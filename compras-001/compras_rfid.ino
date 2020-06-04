
#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

void setupRFID() {
  mfrc522.PCD_Init(); 
  // Mensagens iniciais no serial monitor
  Serial.println();
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();  
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
