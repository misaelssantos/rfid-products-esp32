// V01: Leitor de produtos com RFID
// autor: Misael Santos (misael@gmail.com)
//  
/****************************************
 * Include Libraries
 ****************************************/
#include "secrets.h"  // WIFISSID, PASSWORD, MQTT_USER, MQTT_PASSWORD
#include <WiFi.h>
#include <PubSubClient.h>  // MQTT
#include <MFRC522.h>       // RFID-RC522
#include <SPI.h>           // RFID: Comunicação do barramento SPI
#include "DHTesp.h"        // Sensor DHT11

/****************************************
 * Define Constants
 ****************************************/

// LED Indicadores
#define GREEN_LED   12
#define YELLOW_LED  13
#define RED_LED     14
// RFID
#define SS_PIN    21
#define RST_PIN   22
//
#define BUZZER      27
#define DHTSENSOR   17

#define ERRO "Erro"
#define ADMIN_TAG "admin"

#define INTERVAL 10000000 // Timer de 30s para a leitura do sensor DHT11.

// Buzzer variables
#define NOTE_C 523
#define NOTE_E 659
int freq = 2000;
int channel = 0;
int resolution = 8;

// Indicador de sentido do estoque (0-remover, 1-adicionar)
int stockMode = 0;
int recordMode = 1;
String cardId;

// Wifi
WiFiClient wifi;
// MQTT
PubSubClient client(wifi);
// Sensor
DHTesp dht;
// RFID
// Esse objeto 'chave' é utilizado para autenticação dos dispositivos RFID
MFRC522::MIFARE_Key key;
// código de status de retorno da autenticação RFID
MFRC522::StatusCode status;
// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

// Preparando timer...
hw_timer_t * timer = NULL;
float temperature;
float humidity;
boolean publishSensors = false;
String statusString;

void setupWifi() {

  WiFi.begin(WIFISSID, PASSWORD);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Aguardando WiFi..."); 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blinkLed(RED_LED);
  }
  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  blinkLed(GREEN_LED);
}

void setup() {
  startTimer();
  // Configuring pins...
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  // Buzzer
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(BUZZER, channel);  

  Serial.begin(115200);
  // Wifi...
  setupWifi();
  // Inicia MQTT Client...
  setupMQTT();   
  // Inicia MFRC522
  setupRFID();

  // Sensor DHT11
  dht.setup(DHTSENSOR, DHTesp::DHT11);

  beep();

}

void ledIndicators() {

  if (WiFi.status() == WL_CONNECTED) {
    turnLedOn(GREEN_LED);
  } else {
    turnLedOff(GREEN_LED);    
  }

  if (stockMode == 0) {
    turnLedOn(YELLOW_LED);
  } else {
    turnLedOff(YELLOW_LED);
  }

}

void callbackTimer() {
  statusString = dht.getStatusString();
  if (statusString == "OK") {
    // Setting flag to publish sensors...
    publishSensors = true;
  }
}

void verifySensors() {
  
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  statusString = dht.getStatusString();

  if (publishSensors and (statusString == "OK")) {
    publishSensors = false;
    publishTemperature(temperature);
    publishHumidity(humidity);
  }
}

void startTimer(){
    /* 0 - seleção do timer a ser usado, de 0 a 3.
      80 - prescaler. O clock principal do ESP32 é 80MHz. Dividimos por 80 para ter 1us por tick.
    true - true para contador progressivo, false para regressivo
    */
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &callbackTimer, true);
    timerAlarmWrite(timer, INTERVAL, true); 
    //ativa o alarme
    timerAlarmEnable(timer);
}

void stopTimer(){
    timerEnd(timer);
    timer = NULL; 
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  ledIndicators();

  verifySensors();

   // Aguarda a aproximacao do cartao
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Seleciona um dos cartoes
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Tentando ler o bloco 1 da tag
  String tag = readRFIDTag();

  // Verifica se eh produto ou admin
  if (tag != ADMIN_TAG && tag != ERRO) {

    if (tag != "") {

      Serial.print(F("\nProduto: "));
      Serial.println(tag);
      Serial.println();
      productSet(tag);

    } else {

      Serial.print(F("\nCartão em Branco: "));
      Serial.println(cardId);

    }

  } else {

    Serial.println(F("\nCartão Administrador!"));
    toggleStockMode();

  }

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  

}
