// Input only pins
#define VP 36
#define VN 39

void setup() {
  Serial.begin(115200);
}

int valueA;
int valueB;

void loop() {
  // read the value from the sensor:
  valueA = analogRead(VP);
  valueB = analogRead(VN);

  // print to serial
  Serial.print("A:");
  Serial.print(valueA);
  Serial.print(",B:");
  Serial.println(valueB);

  //100 milliseconds
  delay(100);
}
