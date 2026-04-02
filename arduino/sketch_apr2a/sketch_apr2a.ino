#ifndef LED_BUILTIN
#define LED_BUILTIN 48
#endif

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Testing LED on GPIO ");
  Serial.println(LED_BUILTIN);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED ON");
  delay(500);

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED OFF");
  delay(500);
}
