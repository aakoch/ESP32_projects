#include "WiFi.h"

#define PIN_LED 2

#define ssid "JoelCantDecide"
#define password "3ttt333ttt3"

#define freq 5000
#define blinkFreq 5
#define seizureFreq 5
#define ledChannel 0
#define resolution 8

bool direction = true;
long startTime = millis();

TaskHandle_t PulseTask;
TaskHandle_t CheckOnLedTask;
TaskHandle_t FlashReadyTask;

WiFiServer server(80);

void setup()
{
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(PIN_LED, ledChannel);
    
    Serial.begin(115200);
    
    while (!Serial) {
      delay(100);
    }

    delay(200);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

     WiFi.setSleep(false);
    
    server.begin();

    BaseType_t xReturned;
    xReturned = xTaskCreate(
                      CheckOnLed,   /* Task function. */
                      "CheckOnLedTask",     /* name of task. */
                      10000,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      0,           /* priority of the task */
                      &CheckOnLedTask);  /* Task handle to keep track of created task */
                     
    if( xReturned == pdPASS )
    {
      Serial.println("CheckOnLedTask created");
      vTaskSuspend(CheckOnLedTask); 
    }
  
    xReturned = xTaskCreate(
                      PulseLed,   /* Task function. */
                      "PulseTask",     /* name of task. */
                      10000,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      0,           /* priority of the task */
                      &PulseTask);  /* Task handle to keep track of created task */
    if( xReturned == pdPASS )
    {
      Serial.println("PulseTask created");            
      vTaskSuspend(PulseTask); 
    }

    FlashReady();
}

void loop(){
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String task = "";
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/On\">here</a> to turn the LED on.<br>");
            client.print("Click <a href=\"/Off\">here</a> to turn the LED off.<br>");
            client.print("Click <a href=\"/Pulse\">here</a> to pulse the LED.<br>");
            client.print("Click <a href=\"/Blink\">here</a> to blink the LED.<br>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        
        if (currentLine.endsWith("GET /On")) {
          task = "On";
        }
        
        else if (currentLine.endsWith("GET /Off")) {
          task = "Off";
        }
        
        else if (currentLine.endsWith("GET /Blink")) {    
          task = "Blink";
        }
        
        else if (currentLine.endsWith("GET /Pulse")) {  
          task = "Pulse";
          
        }
      }
    }
        
    if (task == "On") {
      Serial.println("turning on");
      ledcWrite(ledChannel, 100);
      
      startTime = millis();  
      vTaskResume(CheckOnLedTask);
    }
    else if (task != "") {
      vTaskSuspend(CheckOnLedTask);
    }
    
    if (task == "Off") {
      Serial.println("turning off");
      ledcWrite(ledChannel, 0);
      vTaskSuspend(CheckOnLedTask);
    }
    
    if (task == "Blink") {
      Serial.println("turning on blinking");
      ledcChangeFrequency(ledChannel, blinkFreq, resolution);
      ledcWrite(ledChannel, 50);
    }
    else if (task != "") {
      Serial.println("turning off blinking");
      ledcChangeFrequency(ledChannel, freq, resolution);
    }
    
    if (task == "Pulse") {
      Serial.println("turning on pulsing");   
      vTaskResume(PulseTask);       
    }
    else if (task != "") {
      Serial.println("turning off pulsing");   
      vTaskSuspend(PulseTask); 
    }
      
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void CheckOnLed( void * pvParameters ){
  for(;;){
    bool ledOn = ledcRead(ledChannel) == 100;
    Serial.println("ledOn = " + String(ledOn));
    long elapsedTime = millis() - startTime;
    Serial.println("elapsedTime = " + String(elapsedTime));
    if (ledOn && elapsedTime > 60000) {
      ledcWrite(ledChannel, 0);
      vTaskSuspend(CheckOnLedTask);
    }
   
    delay(1000);
  } 
}

void PulseLed( void * pvParameters ){
  int fadeValue = 0;

  for(;;){
    Serial.println("fadeValue = " + String(fadeValue));
    if (direction) {
      fadeValue++;
      if (fadeValue > 99) {
        direction = false;
      }
    }
    else {
      fadeValue--;
      if (fadeValue < 1) {
        direction = true;
      }
    }
    
    ledcWrite(ledChannel, fadeValue);
    delay(9);
  } 
}

void FlashReady() {
  for(int i = 0; i < 3; i++) {
    for(int i = 0; i < 3; i++) {
      ledcWrite(ledChannel, 100);
      delay(90);
      ledcWrite(ledChannel, 0);
      delay(90);
    }
    if (i < 2) {
      delay(500);
    }
  }
}
