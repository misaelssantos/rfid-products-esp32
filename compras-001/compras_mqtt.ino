// MQTT
#define MQTT_CLIENT_NAME "rfid01" // Name for the instance client
#define TOPIC_PREFIX "/compras/devices/esp32/"
#define TOPIC_STOCKMODE "/compras/devices/config/mode"
#define TOPIC_TEMPERATURE "/compras/sensor/temperature"
#define TOPIC_HUMIDITY "/compras/sensor/humidity"

char MQTTBROKER[]  = "192.168.68.113";
char payload[100];
char topic[150];

void setupMQTT() {
  SPI.begin(); // Init SPI bus
  client.setServer(MQTTBROKER, 1883);
  client.setCallback(callbackMQTT);  
}

// MQTT subscription
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.println("");
  Serial.println("-----------------------");
  Serial.print("Mensagem recebida no topico: ");
  Serial.println(topic);

  // Getting the message payload string...
  char buffer[length + 1];
  memcpy(buffer, payload, length);
  buffer[length] = NULL;

  // Convert it to integer
  char *end = nullptr;
  long value = strtol(buffer, &end, 10);
  
  // Check for conversion errors
  if (end == buffer || errno == ERANGE) {
    Serial.print(F("Falha ao ler valor!"));
    blinkLed(RED_LED); // Conversion error occurred
    return;
  } 

  Serial.println();
  Serial.print("Valor: ");
  Serial.println(value);
  stockMode = value;
  blinkLed(GREEN_LED);

}

void reconnectMQTT() {
  int count = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    count = count + 1;
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
      sleep(2);
      blinkLed(RED_LED);
    }

    if (count == 10) {
      Serial.println("");
      Serial.println("Tentativa 10...");
      Serial.println("");
      setupMQTT();
      break;
    }

  }
}

void statusSet(String strTopic, int value) {
  // Montando msg...  
  sprintf(topic, "%s", strTopic);
  sprintf(payload, "%d", value); // Adds the value

  publishMQTT(topic, payload);
}

void publishTemperature(float value) {
  char topic[150];
  // Montando msg...  
  sprintf(topic, "%s", TOPIC_TEMPERATURE);
  char charValue[50];
  String strValue = String(value);
  strValue.toCharArray(charValue, strValue.length() + 1); //packaging up the data to publish to mqtt whoa...

  publishMQTT(topic, charValue);
}

// Registrando valor do Produto
void productSet(String produto) {
  // Montando msg...
  String strtopic = TOPIC_PREFIX + produto;
  strtopic.toCharArray(topic, 100);
  // Valor
  sprintf(payload, "%d", stockMode); // Adds the value

  publishMQTT(topic, payload);  
}

void publishHumidity(float value) {
  char topic[150];
  // Montando msg...  
  sprintf(topic, "%s", TOPIC_HUMIDITY);
  char charValue[50];
  String strValue = String(value);
  strValue.toCharArray(charValue, strValue.length() + 1); //packaging up the data to publish to mqtt whoa...

  publishMQTT(topic, charValue);  
}

void publishMQTT(char topic[], char payload[]) {
  if (!client.connected()) {
    reconnectMQTT();
  }

  Serial.print("\nEnviando dados para o topico: ");
  Serial.print(topic);
  Serial.print(", ");
  Serial.println(payload);
  client.publish(topic, payload);

  // client.loop();
  blinkLed(GREEN_LED);

}

void toggleStockMode() {
  if (stockMode == 0) {
    stockMode = 1;
    Serial.println(F("\nReabastecendo!"));
  } else {
    stockMode = 0;
    Serial.println(F("\nMonitoração de estoque!"));
  }
  statusSet(TOPIC_STOCKMODE, stockMode);
}
