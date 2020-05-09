#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 21
#define RST_PIN 22

#define LED_VERMELHO 32
#define LED_VERDE 12

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup(){
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
}

void loop() {
  
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  if (!mfrc522.PICC_ReadCardSerial()){
    return;
  }
  
  long code=0;
  
  for (byte i = 0; i < mfrc522.uid.size; i++){
    code=((code+mfrc522.uid.uidByte[i])*10); // Nun werden wie auch vorher die vier Blöcke ausgelesen und in jedem Durchlauf wird der Code mit dem Faktor 10 „gestreckt“. (Eigentlich müsste man hier den Wert 1000 verwenden, jedoch würde die Zahl dann zu groß werden.
  }

  Serial.print("O número do cartão é:"); // Zum Schluss wird der Zahlencode (Man kann ihn nicht mehr als UID bezeichnen) ausgegeben.
  Serial.println(code);

  // 273390, 1922640
  if (code==273390 || code==1922640) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  } else {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
  }
  
}
