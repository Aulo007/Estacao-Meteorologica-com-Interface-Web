#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/ssd1306.h"
#include "lib/font.h"
#include <math.h>
#include "lib/matrizRGB.h"
#include "lib/buzzer.h"
#include "lib/leds.h"
#include <stdbool.h>

// ========================================
// CONFIGURAÇÕES BÁSICAS
// ========================================
#define WIFI_SSID "RaGus2.5GHZ"
#define WIFI_PASS "#RaGus2.5GHZ6258"

// ========================================
// BIBLIOTECA DOS SENSORES
// ========================================
#include "lib/aht20.h"
#include "lib/bmp280.h"

// ========================================
// CONFIGURAÇÃO DOS BOTÕES
// ========================================
#include "pico/bootrom.h"
#define BOTAO_B 6
#define BOTAO_A 5
volatile int g_tela_display = 0;
volatile uint32_t g_last_interrupt_time = 0;

void gpio_callback(uint gpio, uint32_t events)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - g_last_interrupt_time < 200)
    {
        return;
    }
    g_last_interrupt_time = now;

    if (gpio == BOTAO_B)
    {
        reset_usb_boot(0, 0);
    }
    else if (gpio == BOTAO_A)
    {
        g_tela_display = 1 - g_tela_display;
    }
}

// ========================================
// CONFIGURAÇÃO PARA OS SENSORES E CALIBRAÇÃO
// ========================================
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

volatile double g_sea_level_pressure = 101325.0;
volatile double g_altura_medida_pelo_google_earth_para_calibrar = 998;
volatile bool g_recalibrar = true;

double calculate_altitude_from_pressure(double local_pressure_pa)
{
    return 44330.0 * (1.0 - pow(local_pressure_pa / g_sea_level_pressure, 0.1903));
}

// ========================================
// HTML EXTERNO (arquivo separado!)
// ========================================
#include "lib/pagina.h"

// ========================================
// VARIÁVEIS GLOBAIS
// ========================================
volatile float g_temp_bmp = 0.0f;
volatile float g_pressao_kpa = 0.0f;
volatile float g_temp_aht = 0.0f;
volatile float g_umidade_aht = 0.0f;
volatile float g_altitude = 0.0f;
volatile float g_temp_media = 0.0f;
volatile bool g_wifi_connected = false;
volatile float g_p_min_kpa = 98.0f;
volatile float g_p_max_kpa = 102.0f;
volatile float g_u_min_perc = 40.0f;
volatile float g_u_max_perc = 70.0f;
volatile float g_t_min_celsius = 18.0f;
volatile float g_t_max_celsius = 28.0f;
#define buzzer_pin 21

// ========================================
// CÓDIGO DE REDE (Sem alterações)
// ========================================
struct conexao_http
{
    char *resposta;
    size_t tamanho;
    size_t enviado;
};
static err_t quando_enviou(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct conexao_http *conn = (struct conexao_http *)arg;
    conn->enviado += len;
    if (conn->enviado >= conn->tamanho)
    {
        tcp_close(tpcb);
        if (conn->resposta)
            free(conn->resposta);
        free(conn);
    }
    return ERR_OK;
}
static void enviar_resposta_http(struct conexao_http *conn, struct tcp_pcb *tpcb, const char *http_status, const char *body)
{
    conn->tamanho = snprintf(conn->resposta, 1024, "%s\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n%s", http_status, (int)strlen(body), body);
    tcp_arg(tpcb, conn);
    tcp_sent(tpcb, quando_enviou);
    tcp_write(tpcb, conn->resposta, conn->tamanho, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
}
static err_t quando_recebeu(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *requisicao = (char *)p->payload;
    struct conexao_http *conn = malloc(sizeof(struct conexao_http));
    conn->resposta = malloc(1024);
    conn->enviado = 0;
    if (strncmp(requisicao, "GET / ", 6) == 0)
    {
        size_t html_len = strlen(HTML_PAGINA);
        conn->tamanho = snprintf(conn->resposta, 1024 + html_len, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", (int)html_len, HTML_PAGINA);
        tcp_arg(tpcb, conn);
        tcp_sent(tpcb, quando_enviou);
        tcp_write(tpcb, conn->resposta, conn->tamanho, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }
    else if (strncmp(requisicao, "GET /api/dados", 14) == 0)
    {
        char json[512];
        snprintf(json, sizeof(json), "{\"temp_bmp\":%.1f,\"pressao_kpa\":%.2f,\"temp_aht\":%.1f,\"umidade_aht\":%.1f,\"altitude\":%.1f,\"offset_pa\":%.2f,\"temp_media\":%.1f,\"p_min\":%.1f,\"p_max\":%.1f,\"u_min\":%.1f,\"u_max\":%.1f,\"t_min\":%.1f,\"t_max\":%.1f,\"status\":\"ok\"}", g_temp_bmp, g_pressao_kpa, g_temp_aht, g_umidade_aht, g_altitude, g_sea_level_pressure, g_temp_media, g_p_min_kpa, g_p_max_kpa, g_u_min_perc, g_u_max_perc, g_t_min_celsius, g_t_max_celsius);
        enviar_resposta_http(conn, tpcb, "HTTP/1.1 200 OK", json);
    }
    else if (strncmp(requisicao, "POST /api/calibrate", 19) == 0)
    {
        char *body = strstr(requisicao, "\r\n\r\n");
        if (body)
        {
            char *value_ptr = strstr(body, ":");
            if (value_ptr)
            {
                g_altura_medida_pelo_google_earth_para_calibrar = strtod(value_ptr + 1, NULL);
                g_recalibrar = true;
                enviar_resposta_http(conn, tpcb, "HTTP/1.1 200 OK", "{\"status\":\"ok\"}");
            }
        }
    }
    else if (strncmp(requisicao, "POST /api/limits", 16) == 0)
    {
        char *body = strstr(requisicao, "\r\n\r\n");
        if (body)
        {
            char *pmin_str = strstr(body, "\"pmin\":");
            char *pmax_str = strstr(body, "\"pmax\":");
            char *umin_str = strstr(body, "\"umin\":");
            char *umax_str = strstr(body, "\"umax\":");
            char *tmin_str = strstr(body, "\"tmin\":");
            char *tmax_str = strstr(body, "\"tmax\":");
            float pmin_new = pmin_str ? atof(pmin_str + 7) : g_p_min_kpa;
            float pmax_new = pmax_str ? atof(pmax_str + 7) : g_p_max_kpa;
            float umin_new = umin_str ? atof(umin_str + 7) : g_u_min_perc;
            float umax_new = umax_str ? atof(umax_str + 7) : g_u_max_perc;
            float tmin_new = tmin_str ? atof(tmin_str + 7) : g_t_min_celsius;
            float tmax_new = tmax_str ? atof(tmax_str + 7) : g_t_max_celsius;
            if (pmax_new < pmin_new || umax_new < umin_new || tmax_new < tmin_new)
            {
                enviar_resposta_http(conn, tpcb, "HTTP/1.1 400 Bad Request", "{\"status\":\"erro\", \"msg\":\"Máximo não pode ser menor que o mínimo.\"}");
            }
            else
            {
                g_p_min_kpa = pmin_new;
                g_p_max_kpa = pmax_new;
                g_u_min_perc = umin_new;
                g_u_max_perc = umax_new;
                g_t_min_celsius = tmin_new;
                g_t_max_celsius = tmax_new;
                enviar_resposta_http(conn, tpcb, "HTTP/1.1 200 OK", "{\"status\":\"ok\"}");
            }
        }
    }
    else
    {
        enviar_resposta_http(conn, tpcb, "HTTP/1.1 404 Not Found", "{\"status\":\"erro\", \"msg\":\"Rota não encontrada\"}");
    }
    pbuf_free(p);
    return ERR_OK;
}
static err_t nova_conexao(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, quando_recebeu);
    return ERR_OK;
}
static void iniciar_servidor_http(void)
{
    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, nova_conexao);
}

// ========================================
// FUNÇÃO PRINCIPAL
// ========================================
int main()
{
    stdio_init_all();
    sleep_ms(2000);
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000);

    iniciar_servidor_http();

    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    bmp280_init(I2C_PORT);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT, &params);
    aht20_init(I2C_PORT);

    printf("Sistema pronto. IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    inicializar_buzzer(buzzer_pin);
    npInit(7);
    led_init();

    while (true)
    {
        cyw43_arch_poll();

        int32_t raw_temp_bmp, raw_pressure_pa_int;
        bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure_pa_int);
        int32_t temp_bmp_c100 = bmp280_convert_temp(raw_temp_bmp, &params);
        uint32_t pressure_pa = bmp280_convert_pressure(raw_pressure_pa_int, raw_temp_bmp, &params);
        if (g_recalibrar)
        {
            g_sea_level_pressure = pressure_pa / pow(1.0 - (g_altura_medida_pelo_google_earth_para_calibrar / 44330.0), 5.255);
            g_recalibrar = false;
        }
        g_pressao_kpa = pressure_pa / 1000.0;
        g_temp_bmp = temp_bmp_c100 / 100.0;
        g_altitude = calculate_altitude_from_pressure(pressure_pa);
        AHT20_Data data_aht;
        if (aht20_read(I2C_PORT, &data_aht))
        {
            g_temp_aht = data_aht.temperature;
            g_umidade_aht = data_aht.humidity;
        }
        g_temp_media = (g_temp_bmp + g_temp_aht) / 2.0f;

        // --- CORREÇÃO DA VERIFICAÇÃO DE STATUS DO WI-FI ---
        // A função precisa de 2 argumentos: o estado do driver e a interface.
        g_wifi_connected = (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP);

        ssd1306_fill(&ssd, false);
        char wifi_status_str[12];
        sprintf(wifi_status_str, "Wifi: %s", g_wifi_connected ? "ON" : "OFF");
        ssd1306_draw_string(&ssd, wifi_status_str, 28, 4);

        if (g_tela_display == 0)
        {
            char str_tmp_bmp[10], str_press[10], str_tmp_aht[10], str_umi[10];
            sprintf(str_tmp_bmp, "%.1fC", g_temp_bmp);
            sprintf(str_press, "%.0fhPa", g_pressao_kpa * 10.0);
            sprintf(str_tmp_aht, "%.1fC", g_temp_aht);
            sprintf(str_umi, "%.1f%%", g_umidade_aht);
            ssd1306_line(&ssd, 3, 18, 123, 18, true);
            ssd1306_draw_string(&ssd, "BMP280", 12, 22);
            ssd1306_draw_string(&ssd, "AHT20", 76, 22);
            ssd1306_line(&ssd, 63, 18, 63, 61, true);
            ssd1306_draw_string(&ssd, str_tmp_bmp, 12, 36);
            ssd1306_draw_string(&ssd, str_press, 12, 48);
            ssd1306_draw_string(&ssd, str_tmp_aht, 76, 36);
            ssd1306_draw_string(&ssd, str_umi, 76, 48);
        }
        else
        {
            ssd1306_draw_string(&ssd, "Limites", 32, 16);
            char str_t_lim[20], str_p_lim[20], str_u_lim[20];
            sprintf(str_t_lim, "T:%.1f-%.1fC", g_t_min_celsius, g_t_max_celsius);
            sprintf(str_p_lim, "P:%.0f-%.0fkPa", g_p_min_kpa, g_p_max_kpa);
            sprintf(str_u_lim, "U:%.0f-%.0f%%", g_u_min_perc, g_u_max_perc);
            ssd1306_draw_string(&ssd, str_t_lim, 4, 30);
            ssd1306_draw_string(&ssd, str_p_lim, 4, 42);
            ssd1306_draw_string(&ssd, str_u_lim, 4, 54);
        }
        ssd1306_send_data(&ssd);

        bool temp_fora_limite = g_temp_media < g_t_min_celsius || g_temp_media > g_t_max_celsius;
        bool press_fora_limite = g_pressao_kpa < g_p_min_kpa || g_pressao_kpa > g_p_max_kpa;
        bool umid_fora_limite = g_umidade_aht < g_u_min_perc || g_umidade_aht > g_u_max_perc;

        npSetColumn(0, temp_fora_limite ? COLOR_RED : COLOR_GREEN);
        npSetColumn(2, press_fora_limite ? COLOR_RED : COLOR_GREEN);
        npSetColumn(4, umid_fora_limite ? COLOR_RED : COLOR_GREEN);

        if (temp_fora_limite || press_fora_limite || umid_fora_limite)
        {
            ativar_buzzer(buzzer_pin);
            acender_led_rgb(255, 0, 0);
        }
        else
        {
            desativar_buzzer(buzzer_pin);
            acender_led_rgb(0, 255, 0);
        }

        sleep_ms(2000);
    }
    return 0;
}