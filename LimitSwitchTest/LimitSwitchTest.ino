void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  Serial.begin(115200);
}

void loop() {
  bool limit1 = !digitalRead(2);
  bool limit2 = !digitalRead(4);
  Serial.print(limit1);
  Serial.print(" ");
  Serial.println(limit2);
  delay(100);
}
