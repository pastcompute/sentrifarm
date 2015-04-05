int led = 13;
int buzzer = 11;

void setup() {
  pinMode(led, OUTPUT);     
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600); 
  Serial.println("Solar voltage monitor");
}

void loop() {
  digitalWrite(led, HIGH);
  tone(buzzer, 990, 120);
  delay(880);  
  digitalWrite(led, LOW);

  int value1 = analogRead(A0);
  int value2 = analogRead(A1);

  Serial.print(value1);
  Serial.print(" ");
  Serial.print(value2);
  Serial.println("");
  
  delay(1000); 
}
