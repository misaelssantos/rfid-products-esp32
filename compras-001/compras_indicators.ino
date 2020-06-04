
void blinkLed(int led) {
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);
}

void turnLedOn(int led) { 
  digitalWrite(led, HIGH); 
}

void turnLedOff(int led) { 
  digitalWrite(led, LOW); 
}

void beep() {
  ledcWrite(channel, 255);
  ledcWriteTone(channel, NOTE_C);
  delay(300);
  ledcWriteTone(channel, 0);
}

void doubleBeep() {
  ledcWrite(channel, 255);
  ledcWriteTone(channel, NOTE_C); //c
  delay(200);
  ledcWriteTone(channel, NOTE_E); //e
  delay(200);
  ledcWriteTone(channel, 0);  
}
