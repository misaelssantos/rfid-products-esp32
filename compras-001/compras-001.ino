// V01: Leitor de produtos com RFID
// autor: Misael Santos (misael@gmail.com)
//  
/****************************************
 * Include Libraries
 ****************************************/
#include <WiFi.h>
#include <PubSubClient.h> // MQTT
#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <ArduinoJson.h>

#define WIFISSID "XXXX" // Put your WifiSSID here
#define PASSWORD "XXXX" // Put your wifi password here
#define TOKEN "XXXX" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "rfid01" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string; 
                                           //it should be a random and unique ascii string and different from all other devices
/****************************************
 * Define Constants
 ****************************************/
// MQTT
#define TOPIC_DEVICES "/v1.6/devices"
#define VARIABLE_LABEL "sensor" // Assing the variable label
#define DEVICE_LABEL "esp32" // Assig the device label
#define STOCK_MODE_LABEL "modo"
char MQTTBROKER[]  = "industrial.api.ubidots.com";
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
#define RED_LED     32

#define ERRO "Erro"
#define ADMIN_TAG "admin"

// Indicador de sentido do estoque (0-remover, 1-adicionar)
int stockMode = 0;
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

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.println("");
  Serial.println("-----------------------");
  Serial.print("Mensagem recebida no topico: ");
  Serial.println(topic);

  // Getting the message payload string...
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);

  // Parsing JSON doc...
  //const int capacity = JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(1);
  StaticJsonDocument<100> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print(F("Falha ao ler JSON: "));
    Serial.println(err.c_str());
    blinkLed(RED_LED);
  }
  int value = doc["value"];
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
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("MQTT Conectado!");
      sprintf(topic, "%s/%s/%s", TOPIC_DEVICES, DEVICE_LABEL, STOCK_MODE_LABEL);  
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

//faz a leitura dos dados do cartão/tag
String readRFIDTag() {
  //imprime os detalhes tecnicos do cartão/tag
  // Serial.println();
  // mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); 

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
    return (ERRO);
  }
  //Faz a leitura dos dados do bloco
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Leitura RFID falhou: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    blinkLed(RED_LED);
    return (ERRO);
  }
  else {
    blinkLed(GREEN_LED);
  }

  String str = (char*)buffer;
  str = str.substring(0, MAX_SIZE_BLOCK);
  str.trim();
  // if (str == "") {
  //   str = "novo";
  // }
  return (str);
  
}

void productSet(String produto) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  // Registrando valor do Produto
  sprintf(topic, "%s/%s", TOPIC_DEVICES, DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", produto); // Adds the variable label
  sprintf(payload, "%s {\"value\": %d}}", payload, stockMode); // Adds the value
  
  Serial.println("Enviando dados via MQTT");
  client.publish(topic, payload);
  
  // client.loop();
  // delay(1000);
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
  productSet(STOCK_MODE_LABEL);
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
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  // Wifi...
  setupWifi();
  // Inicia MQTT Client...
  setupMQTT();   
  // Inicia MFRC522
  setupRFID();

}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

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
