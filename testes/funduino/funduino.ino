#include <SPI.h> // SPI-Bibiothek hinzufügen
#include <MFRC522.h> // RFID-Bibiothek hinzufügen
#define SS_PIN 21 // SDA an Pin 10 (bei MEGA anders)
#define RST_PIN 22 // RST an Pin 9 (bei MEGA anders)

MFRC522 mfrc522(SS_PIN, RST_PIN); // RFID-Empfänger benennen

void setup() {

  Serial.begin(9600); // Serielle Verbindung starten (Monitor)
  SPI.begin(); // Estabelecer conexão SPI
  mfrc522.PCD_Init(); // Inicialização do receptor RFID

}


void loop() {

  // Se nenhum cartão estiver no intervalo ...
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Se nenhum transmissor RFID foi selecionado
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
   
  Serial.print("O ID dos RFID TAGS é:");
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {  
    Serial.print(mfrc522.uid.uidByte[i], HEX); // Então o UID, que consiste em quatro blocos individuais, é lido e enviado ao monitor serial em sequência. O hexadecimal final significa que os quatro blocos do UID são impressos como um número HEX (também com letras)
    Serial.print(" "); // O comando "Serial.print (" ");" garante que haja um espaço entre os blocos individuais lidos.
  }
  
  Serial.println();

}
