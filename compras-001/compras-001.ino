// V01: Leitor de produtos com RFID
// autor: Misael Santos (misael@gmail.com)
//  
/****************************************
 * Include Libraries
 ****************************************/
#include "secrets.h" // WIFISSID, PASSWORD, TOKEN_MQTT
#include <WiFi.h>
#include <PubSubClient.h> // MQTT
#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <ArduinoJson.h>

/****************************************
 * Define Constants
 ****************************************/
// MQTT
#define MQTT_CLIENT_NAME "rfid01" // Name for the instance client
#define TOPIC_PREFIX "/compras/devices"
#define TOPIC_STOCKMODE "/compras/devices/config/mode"
#define VARIABLE_LABEL "sensor" // Assing the variable label
#define VARIABLE_TAGS "tags"
#define DEVICE_LABEL "esp32" // Assig the device label
#define STOCK_MODE_LABEL "modo"
#define NEW_INFO "new"

// char MQTTBROKER[]  = "industrial.api.ubidots.com";
char MQTTBROKER[]  = "192.168.0.17";
char payload[100];
char topic[150];
// Space to store values to send
char str_sensor[10];

// RFID
#define SS_PIN    21
#define RST_PIN   22
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

// LED Indicadores
#define GREEN_LED   12
#define YELLOW_LED  13
#define RED_LED     14

#define BUZZER      27

#define ERRO "Erro"
#define ADMIN_TAG "admin"

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

//Esse objeto 'chave' é utilizado para autenticação dos dispositivos RFID
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação RFID
MFRC522::StatusCode status;
// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

WiFiClient wifi;
PubSubClient client(wifi);

/****************************************
 * Auxiliar Functions
 ****************************************/
void blinkLed(int led) {
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);
}
void turnLedOn(int led) { digitalWrite(led, HIGH); }
void turnLedOff(int led) { digitalWrite(led, LOW); }
void beep() {
  ledcWrite(channel, 100);
  ledcWriteTone(channel, NOTE_C);
  delay(300);
  ledcWriteTone(channel, 0);
}
void doubleBeep() {
  ledcWrite(channel, 100);
  ledcWriteTone(channel, NOTE_C); //c
  delay(200);
  ledcWriteTone(channel, NOTE_E); //e
  delay(200);
  ledcWriteTone(channel, 0);  
}


void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.println("");
  Serial.println("-----------------------");
  Serial.print("Mensagem recebida no topico: ");
  Serial.println(topic);

  // Getting the message payload string...
  char buffer[length + 1];
  memcpy(buffer, payload, length);
  buffer[length] = NULL;
  String message(buffer);
  Serial.print("Valor (byte): ");
  Serial.write(payload, length);

  // Convert it to integer
  char *end = nullptr;
  long value = strtol(buffer, &end, 10);
  
  // Check for conversion errors
  if (end == buffer || errno == ERANGE) {
    Serial.print(F("Falha ao ler valor!"));
    blinkLed(RED_LED); // Conversion error occurred
  } else {
    Serial.println("CONVERTEU!!!!");
    Serial.println(value);
  }

  // Parsing JSON doc...
  //const int capacity = JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(1);
  // StaticJsonDocument<100> doc;
  // DeserializationError err = deserializeJson(doc, payload);
  // if (err) {
  //  Serial.print(F("Falha ao ler JSON: "));
  //  Serial.println(err.c_str());
  //  blinkLed(RED_LED);
  // }
  

  // JsonObject documentRoot = doc.as<JsonObject>();
  // JsonObject& object = doc.createObject();
  // documentRoot.printTo(Serial);

  // int value = doc["value"];
  
  Serial.println();
  Serial.print("Valor: ");
  Serial.println(value);
  stockMode = value;
  blinkLed(GREEN_LED);

}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Conectando cliente MQTT...");
    
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("MQTT Conectado!");
      sprintf(topic, "%s", TOPIC_STOCKMODE);
      Serial.print("Assinando topico: ");
      Serial.println(topic);
      client.subscribe(topic);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 2 segundos");
      // Wait 2 seconds before retrying
      blinkLed(RED_LED);
    }
  }
}

//Faz a leitura dos dados do cartão/tag
String readRFIDTag() {
  // Reading Card ID...
  long code=0; 
  for (byte i = 0; i < mfrc522.uid.size; i++){
    code=((code+mfrc522.uid.uidByte[i])*10);
  }
  char bufferId[mfrc522.uid.size];
  sprintf(bufferId, "%d", code);
  Serial.println();
  Serial.print(F("ID: "));
  Serial.println(bufferId);
  cardId = bufferId;

  //Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  //buffer para colocar os dados ligos
  byte buffer[SIZE_BUFFER] = {0};
  //bloco que faremos a operação
  byte bloco = 1;
  byte tamanho = SIZE_BUFFER;
  //faz a autenticação do bloco que vamos operar
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Autenticacao RFID falhou: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkLed(RED_LED);
    doubleBeep();
    return (ERRO);
  }
  //Faz a leitura dos dados do bloco
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Leitura RFID falhou: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkLed(RED_LED);
    doubleBeep();
    return (ERRO);
  }
  else {
    blinkLed(GREEN_LED);
    beep();    
  }

  String str = (char*)buffer;
  str = str.substring(0, MAX_SIZE_BLOCK);
  str.trim();
  return (str);
  
}

// Registrando valor do Produto
void productSet(String produto) {
  // Montando msg...  
  sprintf(topic, "%s/%s/%s", TOPIC_PREFIX, DEVICE_LABEL, produto);
  // Usando string nessa concatenacao pelo problema com strings grandes no sprintf
  // String strpayload = "{\"" + produto + "\": ";
  // strpayload.toCharArray(payload, 100);
  //sprintf(payload, "%s {\"value\": %d}}", payload, stockMode); // Adds the value  
  sprintf(payload, "%d", stockMode); // Adds the value

  publishMQTT(topic, payload);
  
}

void modetSet(int value) {
  // Montando msg...  
  // sprintf(topic, "%s", TOPIC_STOCKMODE);
  // sprintf(payload, "%s", ""); // Cleans the payload
  // sprintf(payload, "{\"%s\":", STOCK_MODE_LABEL); // Adds the variable label
  // sprintf(payload, "%s {\"value\": %d}}", payload, value); // Adds the value
  sprintf(payload, "%d", value); // Adds the value

  publishMQTT(topic, payload);
  
}

void publishMQTT(char topic[], char payload[]) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  
  Serial.print("\nEnviando dados para o topico: ");
  Serial.print(topic);
  Serial.print(",");
  Serial.println(payload);
  client.publish(topic, payload);
  
  // client.loop();
  blinkLed(GREEN_LED);

}

void toggleStockMode() {
  if (stockMode == 0) {
    stockMode = 1;
    Serial.print(F("\nReabastecendo!"));
  } else {
    stockMode = 0;
    Serial.print(F("\nMonitoração de estoque!"));
  }
  modetSet(stockMode);
}

void setupWifi() {
  Serial.begin(115200);
  WiFi.begin(WIFISSID, PASSWORD);
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

void setupMQTT() {
  SPI.begin(); // Init SPI bus
  client.setServer(MQTTBROKER, 1883);
  client.setCallback(callbackMQTT);  
}

void setupRFID() {
  mfrc522.PCD_Init(); 
  // Mensagens iniciais no serial monitor
  Serial.println();
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();  
}

void setup() {
  // Configuring pins...
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Buzzer
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(BUZZER, channel);  

  // Wifi...
  setupWifi();
  // Inicia MQTT Client...
  setupMQTT();   
  // Inicia MFRC522
  setupRFID();

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

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  ledIndicators();

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

    Serial.print(F("\nCartão Administrador!"));
    toggleStockMode();

  }
 
  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  

}
