#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Serial.print("Found ");
  Serial.print(findDevices(ONE_WIRE_BUS), DEC);
  Serial.println(" device/s");

  // With the sensor on the tiny board, it is always in "normal"/non-parasite mode.
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
}

void loop() {
  sensors.requestTemperatures(); 
  float temperature = sensors.getTempFByIndex(0);
  if (temperature > -50) {
    Serial.println(temperature);
    delay(900);
  }
  delay(100);
  
}

uint8_t findDevices(int pin)
{
  OneWire ow(pin);

  uint8_t address[8];
  uint8_t count = 0;

  if (ow.search(address))
  {
    do {
      count++;
    } while (ow.search(address));
  }
  return count;
}
