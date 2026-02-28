// Pino que recebe o sinal digital do sensor de vazão
#define BUTTON_PIN 32 

// Biblioteca para conectar ao Wi-Fi
#include <WiFi.h>

//Biblioteca para gerenciar a conexão com servidor MQTT
#include <PubSubClient.h>

//Biblioteca para estruturar a mensagem enviada como JSON
#include <ArduinoJson.h>

//Arquivo para armazenar informações sensiveis
#include "secrets.h"

//Informação da residencia
const char* residencia = "105";

//Informações do Wi-Fi
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

//Informações de servidor MQTT
const char* mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;

WiFiClient espClient;
PubSubClient client(espClient);

JsonDocument doc;

int state = LOW;

volatile int water = 0; 

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; 


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se ao Wi-FI");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP32Client-";
    clientId += residencia;
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado!");
      client.publish("esp32/status", "online");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void IRAM_ATTR flow() {
  unsigned long interruptTime = millis();
  
  if (interruptTime - lastInterruptTime > debounceDelay) {
    water++;
    lastInterruptTime = interruptTime;
  }
}

void setup() {
  Serial.begin(115200);

  doc["residencia"] = residencia;
  doc["pulso"] = 0;
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), flow, FALLING);
  
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    lastMsg = millis();
    
    
    if(water > 0){
      noInterrupts();
      doc["pulso"] = water;
      water = 0;
      interrupts();
      String payload;
      serializeJson(doc, payload);
      Serial.println("Enviando Pulsos");
      Serial.println(payload);
      client.publish("recanto/vazao", payload.c_str());
    }
    
  }
}