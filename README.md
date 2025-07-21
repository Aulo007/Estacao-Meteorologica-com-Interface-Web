# PicoClima: Estação Meteorológica com Interface Web e Alertas

![Linguagem](https://img.shields.io/badge/Linguagem-C/C++-blue.svg)
![Hardware](https://img.shields.io/badge/Hardware-Raspberry%20Pi%20Pico%20W-purple.svg)
![Framework](https://img.shields.io/badge/SDK-Pico%20SDK-red.svg)

[cite_start]Este repositório contém o código-fonte do projeto "PicoClima", uma estação meteorológica completa desenvolvida como parte da **Residência Tecnológica em Sistemas Embarcados** (Embarca Tech / Softex)[cite: 2, 9]. O sistema utiliza um Raspberry Pi Pico W para coletar dados de sensores, exibi-los localmente, servir uma interface web e acionar alertas.

![Foto do Projeto](https://user-images.githubusercontent.com/SEU_USUARIO/SEU_REPO/SUA_IMAGEM.jpg)
*(Sugestão: Tire uma foto do seu projeto montado e funcionando e substitua o link acima para exibi-la aqui!)*

---

## 🚀 Funcionalidades

O projeto integra diversas funcionalidades de hardware e software para criar uma solução de monitoramento completa:

* **Monitoramento de Sensores:**
    * [cite_start]Leitura de temperatura e umidade relativa com o sensor **AHT20**[cite: 19].
    * [cite_start]Leitura de pressão barométrica e temperatura com o sensor **BMP280**[cite: 19].
    * Cálculo de temperatura média e altitude com base na pressão.

* **Servidor Web Embarcado:**
    * [cite_start]Interface web responsiva, acessível via Wi-Fi em computadores e celulares[cite: 25, 41].
    * [cite_start]Exibição de dados em tempo real através de requisições AJAX/JSON[cite: 30].
    * [cite_start]Gráficos dinâmicos para temperatura, pressão e umidade[cite: 26].
    * Formulários para calibração de altitude e configuração de limites de alerta.

* **Display OLED Interativo:**
    * [cite_start]Exibição local do status da conexão Wi-Fi e dos dados dos sensores[cite: 23, 33].
    * Interface com duas telas, alternáveis através de um botão físico, para visualização de dados ou dos limites configurados.

* **Sistema de Alertas:**
    * [cite_start]**Sonoro:** Um buzzer é acionado caso qualquer parâmetro ultrapasse os limites definidos[cite: 27, 36].
    * [cite_start]**Visual (LED RGB):** O LED RGB integrado atua como um indicador de status geral, ficando verde em condições normais e vermelho durante um alerta[cite: 27, 35].
    * [cite_start]**Visual (Matriz de LEDs):** A matriz oferece um feedback granular, indicando com colunas vermelhas ou verdes qual(is) sensor(es) está(ão) fora do limite[cite: 27, 34].

* **Interação Física:**
    * [cite_start]Uso de botões com tratamento de interrupção e debounce para uma interação confiável e sem falhas[cite: 37, 38].

---

## 🛠️ Hardware Necessário

* Placa BitDogLab (ou Raspberry Pi Pico W com periféricos equivalentes)
* Sensor de Temperatura e Umidade AHT20
* Sensor de Pressão e Temperatura BMP280
* Display OLED SSD1306
* Buzzer
* Matriz de LEDs RGB (Neopixel)
* LED RGB
* Botões (Push Buttons)

---

## ⚙️ Instalação e Compilação

Este projeto foi desenvolvido utilizando o SDK oficial do Raspberry Pi Pico com VS Code.

### Pré-requisitos

* Ambiente de desenvolvimento para o Raspberry Pi Pico configurado (Pico SDK, ARM GCC Compiler, CMake).
* Visual Studio Code com as extensões **CMake Tools** e **Cortex-Debug**.

(Para um guia detalhado de configuração do ambiente, consulte a [documentação oficial](https://github.com/raspberrypi/pico-setup-windows)).

### Passos para Execução

1.  **Clonar o Repositório**
    ```bash
    git clone [https://github.com/Aulo007/Estacao-Meteorologica-com-Interface-Web.git](https://github.com/Aulo007/Estacao-Meteorologica-com-Interface-Web.git)
    ```

2.  **Configurar Credenciais de Wi-Fi**
    Abra o arquivo `main.c` e edite as seguintes linhas com os dados da sua rede Wi-Fi:
    ```c
    #define WIFI_SSID "SUA_REDE_WIFI"
    #define WIFI_PASS "SUA_SENHA"
    ```

3.  **Compilar o Projeto**
    * Abra a pasta do projeto no Visual Studio Code.
    * Aguarde a extensão CMake Tools configurar o projeto.
    * Selecione o compilador GCC Arm na barra de status inferior.
    * Clique no botão **run**  na barra de status.

4.  **Gravar na Placa (Flashing)**
    * Com a placa desconectada, pressione e segure o botão **BOOTSEL**.
    * Conecte a placa ao computador via cabo USB. Ela será montada como um disco removível.
    * Arraste e solte o arquivo `seu_projeto.uf2` (gerado na pasta `build`) para dentro do disco da placa.
    * A placa irá reiniciar automaticamente e começar a executar o código.

---

## 🎮 Como Usar

1.  **Monitore a Saída Serial:** Após a gravação, abra um monitor serial (ex: o Serial Monitor do VS Code ou PuTTY). A placa imprimirá o endereço IP quando se conectar à rede Wi-Fi.
2.  **Acesse a Interface Web:** Digite o endereço IP obtido em um navegador no seu computador ou celular.
3.  **Interaja com a Placa:**
    * Use o **Botão A** (pino 5) para alternar a visualização do display OLED entre os dados dos sensores e os limites de alerta.
    * Teste o sistema de alertas configurando limites na interface web que sejam facilmente ultrapassados (ex: uma temperatura mínima acima da atual).
