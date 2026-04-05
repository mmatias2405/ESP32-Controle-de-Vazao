# Controle de Vazão com ESP32 via MQTT - Simulação Wokwi

Este repositório contém o firmware para um sistema IoT de monitoramento de vazão de água utilizando o microcontrolador ESP32. 

**Como a aplicação funciona:**
O sistema utiliza interrupções de hardware (no pino GPIO 32) para ler os sinais digitais gerados por um sensor de fluxo (simulado por um botão). O código implementa uma lógica de *debounce* de 200ms para ignorar ruídos elétricos. A cada 1 segundo, caso exista fluxo de água, o ESP32 desabilita temporariamente as interrupções para evitar falhas de leitura, processa os pulsos acumulados e estrutura um payload em formato JSON. Esse payload, contendo o ID da residência e o número de pulsos, é então transmitido via rede Wi-Fi para um Broker MQTT no tópico `recanto/vazao`.

O projeto foi estruturado para ser gerenciado através do **PlatformIO** e simulado visualmente no **Wokwi**.

---

## 🚀 Como Executar o Projeto (Passo a Passo)

Siga rigorosamente as instruções abaixo para configurar o seu ambiente, compilar o código e iniciar a simulação.

### Passo 1: Preparando o VS Code e a Extensão do Wokwi
O projeto foi desenvolvido para rodar de forma integrada dentro do Visual Studio Code.
1. Certifique-se de ter o [Visual Studio Code](https://code.visualstudio.com/) instalado em sua máquina.
2. Abra o VS Code, acesse a aba de Extensões (`Ctrl+Shift+X` ou `Cmd+Shift+X` no Mac).
3. Busque por **Wokwi Simulator** e clique em Instalar.
4. **Importante:** A extensão precisa de uma licença gratuita ativada. Pressione `F1` no teclado, digite `Wokwi: Request a New License` e siga o link para autenticar com a sua conta Wokwi no navegador.

### Passo 2: Configurando Senhas e Credenciais (Segurança)
Para proteger suas senhas de rede e do servidor, elas não ficam expostas no código principal. Você deve configurar seu ambiente local:
1. No explorador de arquivos do VS Code, localize o arquivo `secrets-template.h`.
2. Faça uma cópia exata deste arquivo e renomeie a cópia para **`secrets.h`**.
3. Abra o `secrets.h` recém-criado e insira as suas credenciais reais:
   - Nome da rede (SSID) e Senha do WiFi.
   - Endereço do Broker MQTT, porta, usuário e senha.

### Passo 3: Instalando o PlatformIO e Compilando (PIO)
Este projeto utiliza o PlatformIO (PIO) para gerenciar o framework do ESP32 e baixar as dependências automaticamente (como as bibliotecas `PubSubClient` e `ArduinoJson`).
1. Na aba de Extensões do VS Code (`Ctrl+Shift+X`), busque por **PlatformIO IDE** e instale. *(A primeira instalação pode levar alguns minutos enquanto os pacotes base são baixados).*
2. Após a instalação, reinicie o VS Code, se for solicitado.
3. Abra o terminal integrado clicando em `Terminal > New Terminal` na barra superior.
4. No terminal, execute o comando de compilação:
   ```bash
   pio run
5. Aguarde o processo: O PlatformIO fará o download de todas as bibliotecas listadas no projeto e compilará o seu código. O processo estará concluído quando você visualizar a mensagem verde SUCCESS no terminal.

### Passo 4: Iniciando a Simulação no Wokwi
Com as bibliotecas baixadas e o código devidamente compilado no passo anterior, o simulador está pronto para rodar o arquivo binário:
1. No explorador de arquivos do VS Code, clique no arquivo diagram.json para abri-lo. Este arquivo contém a interface gráfica do circuito.
2. Ao abrir o arquivo, uma aba do simulador Wokwi aparecerá (ou você pode abrir a pré-visualização clicando no ícone do Wokwi no canto superior direito do editor).
3. Na janela do simulador, localize o botão verde de Play (Iniciar Simulação) e clique nele.
4. O ESP32 virtual será ligado, carregará as credenciais do seu secrets.h, se conectará ao Wi-Fi e começará a aguardar os pulsos no GPIO 32 para enviar via MQTT!