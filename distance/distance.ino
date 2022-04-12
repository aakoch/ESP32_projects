
/*
 *  For the HC-SR04
 */

int inputPin = 16;
int triggerPin = 17;

int duration;
int distance;

void setup() {
  pinMode(triggerPin, OUTPUT);
  Serial.begin(19200);
}

void loop() {

  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  duration = pulseIn(inputPin, HIGH);

  // filter out bad values
  if (duration < 10000) {
    distance = duration * 0.034 / 2;
    Serial.println(distance);                    // Send the Signal value to Serial Plotter.
  }

  delay(100);
}
