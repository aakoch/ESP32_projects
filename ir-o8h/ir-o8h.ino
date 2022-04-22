/*
  Analog Input

  Demonstrates analog input by reading an analog sensor on analog pin 0 and
  turning on and off a light emitting diode(LED) connected to digital pin 13.
  The amount of time the LED will be on and off depends on the value obtained
  by analogRead().

  The circuit:
  - potentiometer
    center pin of the potentiometer to the analog input 0
    one side pin (either one) to ground
    the other side pin to +5V
  - LED
    anode (long leg) attached to digital output 13 through 220 ohm resistor
    cathode (short leg) attached to ground

  - Note: because most Arduinos have a built-in LED attached to pin 13 on the
    board, the LED is optional.

  created by David Cuartielles
  modified 30 Aug 2011
  By Tom Igoe

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
*/

int outputPin = 7;    // select the input pin for the potentiometer
int enablePin = 32;      // select the pin for the LED
bool objectDetect = false;  // variable to store the value coming from the sensor

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(enablePin, OUTPUT);
  Serial.begin(19200);
}

void loop() {
  digitalWrite( enablePin, HIGH);     // Enable the internal 38kHz signal.
  delayMicroseconds( 210);                   // Wait 210µs (8 pulses of 38kHz).
  if ( digitalRead( outputPin))       // If detector Output is HIGH,
  {
    objectDetect = false;           // then no object was detected;
  }
  else                                // but if the Output is LOW,
  {
    delayMicroseconds( 395);               // wait for another 15 pulses.
    if ( digitalRead( outputPin))   // If the Output is now HIGH,
    { // then first Read was noise
      objectDetect = false;       // and no object was detected;
    }
    else                            // but if the Output is still LOW,
    {
      objectDetect = true;        // then an object was truly detected.
    }
  }
   Serial.println(objectDetect);
  digitalWrite( enablePin, LOW);      // Disable the internal 38kHz signal.
  delayMicroseconds( 265);
}
