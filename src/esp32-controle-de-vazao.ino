// ============================================================================
// Configurações de Hardware e Bibliotecas
// ============================================================================

#define BUTTON_PIN 32 // Pino GPIO do ESP32 que recebe o sinal digital do sensor de vazão

#include <WiFi.h>           // Gerencia a conexão Wi-Fi do microcontrolador
#include <PubSubClient.h>   // Cliente MQTT para publicação e subscrição de tópicos
#include <ArduinoJson.h>    // Facilita a estruturação do payload no formato JSON
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"        // Arquivo local com credenciais sensíveis (IMPORTANTE: deve estar no .gitignore)

// ============================================================================
// Configurações do Display OLED
// ============================================================================
#define SCREEN_WIDTH 128 // Largura do display em pixels
#define SCREEN_HEIGHT 64 // Altura do display em pixels
#define OLED_RESET    -1 // Pino de reset (ou -1 se compartilhar o reset do Arduino)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================================
// Variáveis Globais e Configurações de Rede
// ============================================================================

const char* residencia = "105"; // Identificador único da residência para este dispositivo

// Credenciais importadas do secrets.h
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;

// Instâncias dos clientes de rede
WiFiClient espClient;           // Lida com a camada TCP/IP
PubSubClient client(espClient); // Lida com o protocolo MQTT sobre o TCP/IP

// Documento JSON alocado dinamicamente para montar o payload de envio
JsonDocument doc;
String lastPublishedMsg = "Nenhum envio"; // Armazena o último payload para o display

// ============================================================================
// Variáveis de Interrupção (Volatile)
// ============================================================================
// O modificador 'volatile' avisa o compilador para carregar a variável direto da RAM 
// a cada leitura, pois ela pode ser alterada a qualquer momento pela rotina de interrupção (ISR).

volatile int water = 0; 
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; // Tempo em ms para ignorar ruídos elétricos do sensor (debounce)

// ============================================================================
// Funções Auxiliares
// ============================================================================

// Atualiza a interface do display OLED
void atualizarDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  display.print("WiFi: ");
  display.println(WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
  
  display.print("MQTT: ");
  display.println(client.connected() ? "ON" : "OFF");
  
  display.println("---------------------");
  display.println("Ultimo Payload:");
  display.println(lastPublishedMsg);
  
  display.display();
}

// Inicializa e aguarda a conexão com a rede Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println("\nConectando-se ao Wi-Fi...");

  WiFi.begin(ssid, password);
  
  // Trava a execução enquanto não conectar
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi conectado com sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Mantém ou recupera a conexão com o broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão com Broker MQTT...");
    
    // Gera um ID de cliente único para evitar conflitos no Broker
    String clientId = "ESP32Client-";
    clientId += residencia;
    
    // Tenta conectar usando as credenciais
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conectado!");
      // Publica uma mensagem de status informando que o dispositivo "acordou"
      client.publish("esp32/status", "online");
    } else {
      Serial.print("Falhou. Código de erro (rc) = ");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// ============================================================================
// Rotina de Serviço de Interrupção (ISR)
// ============================================================================
// IRAM_ATTR força a função a ficar na RAM interna do ESP32 (mais rápida),
// evitando atrasos causados pela leitura da memória Flash.
void IRAM_ATTR flow() {
  unsigned long interruptTime = millis();
  
  // Filtro de Debounce: Só contabiliza o pulso se passou o tempo mínimo desde o último
  if (interruptTime - lastInterruptTime > debounceDelay) {
    water++;
    lastInterruptTime = interruptTime;
  }
}

// ============================================================================
// Setup (Executado uma vez na inicialização)
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Inicializa o display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Endereço I2C padrão 0x3C
    Serial.println(F("Falha na alocacao do SSD1306"));
    for(;;); // Trava se der erro no display
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Pré-configura os campos fixos do JSON para economizar processamento no loop
  doc["residencia"] = residencia;
  doc["pulso"] = 0;
  
  // Configura o pino com resistor de pull-up interno (evita flutuação de sinal)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Atrela a função 'flow' ao pino. O gatilho 'FALLING' aciona a interrupção 
  // quando o sinal vai de HIGH para LOW (comum em botões/sensores com pull-up).
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), flow, FALLING);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port); 

}

// ============================================================================
// Loop Principal
// ============================================================================
void loop() {
  // Garante que o MQTT continue conectado
  if (!client.connected()) {
    reconnect();
  }
  // Mantém os processos em background do cliente MQTT rodando (ex: ping)
  client.loop();
  
  // Temporizador não-bloqueante de 1 segundo (1000 ms)
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    lastMsg = millis();
    
    // Só processa e envia se houve leitura de vazão
    if(water > 0){
      
      // Região Crítica: Desabilita interrupções para evitar que a ISR altere 'water' 
      // exatamente no meio da leitura/limpeza (evita "Race Condition")
      noInterrupts();
      doc["pulso"] = water;
      water = 0; // Zera o contador para o próximo ciclo
      interrupts(); // Reabilita as interrupções
      
      // Serializa o JSON e publica no tópico
      String payload;
      serializeJson(doc, payload);
      
      Serial.println("Enviando medição de Pulsos:");
      Serial.println(payload);
      
      client.publish("recanto/vazao", payload.c_str());
      
      atualizarDisplay();
    }
  }
}