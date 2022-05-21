// 3144 Hall Sensor
// Detects a magnetic field and toggles like a switch
// Uses 3.3V
#define PIN_IN 36
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
void setup() {
  Serial.begin(115200);
}

void loop() {
  int value = analogRead(PIN_IN);

  Serial.print("value:");
  Serial.println(value);

  delay(500);
}
