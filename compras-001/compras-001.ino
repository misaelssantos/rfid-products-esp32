// V01: Leitor de produtos com RFID
// autor: Misael Santos (misael@gmail.com)
//  
// TODO1: Colocar LED indicador do wifi

/****************************************
 * Include Libraries
 ****************************************/
#include <WiFi.h>
#include <PubSubClient.h> // MQTT
#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI

#define WIFISSID "Simony_2G" // Put your WifiSSID here
#define PASSWORD "4F63B89E" // Put your wifi password here
#define TOKEN "BBFF-TXBx9JmZ5yjlpmmxOxzvysbIVirNRC" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "rfid01" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string; 
                                           //it should be a random and unique ascii string and different from all other devices
/****************************************
 * Define Constants
 ****************************************/
// MQTT
#define VARIABLE_LABEL "sensor" // Assing the variable label
#define DEVICE_LABEL "esp32" // Assig the device label
#define TOPIC_DEVICES "/v1.6/devices/"
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

// Indicador de sentido do estoque (0-remover, 1-adicionar)
String stockMode="0";

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

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Conectando cliente MQTT...");
    
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Conectado");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 2 segundos");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

//faz a leitura dos dados do cartão/tag
String leituraDadosRFID() {

  //imprime os detalhes tecnicos do cartão/tag
  Serial.println();
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); 

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

  //faz a leitura dos dados do bloco
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
  if (str == "") {
    str = "novo";
  }
  return (str);
  
}

void productSet(String produto) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  // Registrando valor do Produto
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", produto); // Adds the variable label
  sprintf(payload, "%s {\"value\": %s}}", payload, stockMode); // Adds the value
  
  Serial.println("Enviando dados via MQTT");
  client.publish(topic, payload);
  
  // client.loop();
  delay(1000);
  
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
  client.setCallback(callback);  
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
  String tag = leituraDadosRFID();

  // Verifica se eh produto ou admin
  if (tag != "admin" && tag != ERRO) {

    Serial.print(F("\nProduto: "));
    Serial.println(tag);
    Serial.println();

    productSet(tag);

  } else {

    Serial.print(F("\nCartão Administrador!"));
    //sleep(1000);
    if (stockMode == "0") {
      stockMode = "1";
      Serial.print(F("\nReabastecendo!"));
    } else {
      stockMode = "0";
      Serial.print(F("\nMonitoração de estoque!"));
    }
    productSet("modo");
  }
 
  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  

}
