void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 says hello");
}

void loop() {
  static int counter = 0;
  Serial.print("Loop count: ");
  Serial.println(counter++);
  delay(1000);
}
