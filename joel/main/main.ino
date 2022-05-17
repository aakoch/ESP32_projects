#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include "WiFiCredentials.h"

#define PIN_LED 2
#define PIN_SPEAKER 4

#define freq 5000
#define blinkFreq 5
#define seizureFreq 5
#define ledChannel 0
#define speakerChannel 1
#define speakerFreq 10000
#define resolution 8
#define NOTE_PAUSE_LENGTH 400
#define EIGTH_NOTE_LENGTH 200

#define DEBUG_ON1 true

bool direction = true;
long startTime = millis();

TaskHandle_t PulseTask;
TaskHandle_t CheckOnLedTask;
TaskHandle_t FlashReadyTask;
TaskHandle_t sweepSoundTask;

WebServer httpServer(80);
HTTPUpdateServer httpUpdater;

void handleRoot() {
//  char temp[400];
//  int sec = millis() / 1000;
//  int min = sec / 60;
//  int hr = min / 60;

  String response = "<html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <link rel=\"icon\" href=\"data:,\">\
    <title>Joel</title>\
    <style>\
      button {\
        background-color: #4CAF50;\
        border: none;\
        color: white;\
        padding: 16px 40px;\
        text-decoration: none;\
        font-size: 30px;\
        margin: 2px;\
        cursor: pointer;\
      }\
    </style>\
  </head>\
  <body>\
    <a href=\"/On\"><button>On</button></a>\
    <a href=\"/Off\"><button>Off</button></a><br>\
    <a href=\"/Pulse\"><button>Pulse</button></a><br>\
    <a href=\"/Blink\"><button>Blink</button></a><br>\
    <a href=\"/twinkle\"><button>Play twinkle twinkle</button></a><br>\
    <a href=\"/sweep\"><button>Sweep sound</button></a><br>\
    Click <a href=\"/update\">here</a> to update the firmware.<br>\
  </body>\
</html>";

  httpServer.send(200, "text/html", response);
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";

  for (uint8_t i = 0; i < httpServer.args(); i++) {
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }

  httpServer.send(404, "text/plain", message);
}

void setup()
{
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(PIN_LED, ledChannel);
    ledcSetup(speakerChannel, speakerFreq, resolution);
    
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

//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    WiFi.begin(ssid, password);
//    Serial.println("WiFi failed, retrying.");
//  }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.println("IP address: ");
    String localIP = WiFi.localIP().toString();
    Serial.println(localIP);

    WiFi.setSleep(false);
    
    if (MDNS.begin("esp32")) {
      Serial.println("mDNS responder started");
    }
  
    httpUpdater.setup(&httpServer);
    httpServer.begin();

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
      if (DEBUG_ON1) Serial.println("CheckOnLedTask created");
      vTaskSuspend(CheckOnLedTask); 
    }
  
    xReturned = xTaskCreate(
                      PulseLed,   /* Task function. */
                      "PulseTask",     /* name of task. */
                      10000,       /* Stack size of task */
                      NULL,        /* parameter of the task */
                      0,           /* priority of the task */
                      &PulseTask);  /* Task handle to keep track of created task */
    if (xReturned == pdPASS)
    {
      if (DEBUG_ON1) Serial.println("PulseTask created");
      vTaskSuspend(PulseTask); 
    }


    MDNS.addService("http", "tcp", 80);
    
    httpServer.on("/", handleRoot);
    httpServer.on("/On", turnOn);
    httpServer.on("/Off", turnOff);
    httpServer.on("/Pulse", startPulse);
    httpServer.on("/Blink", startBlink);
    httpServer.on("/sweep", sweepSound);
    httpServer.on("/sound2", testSound2);
    httpServer.on("/sound3", testSound3);
    httpServer.on("/soundOff", turnSoundOff);
    httpServer.on("/twinkle", playTwinkleTwinkle);
    httpServer.on("/test.svg", drawGraph);
    httpServer.on("/inline", []() {
      httpServer.send(200, "text/plain", "this works as well");
    });
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();
    Serial.println("HTTP server started");

    if (DEBUG_ON1) Serial.printf("HTTPUpdateServer ready! Open http://%s/update in your browser\n", localIP);
  
    FlashReady();
}

void loop() {
  httpServer.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}

void sweepSound() {
  Serial.println("sweeping sound");
  
  xTaskCreatePinnedToCore(
    sweepSoundTaskMethod,   /* Task function. */
    "sweepSountTask",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    0,           /* priority of the task */
    &sweepSoundTask,  /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
      
  handleRoot();
}
    

void sweepSoundTaskMethod(void * pvParameters) {
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  for (int i = 1; i < 2000; i++) {
    ledcWriteTone(speakerChannel, i);
    delay(3);
  }
  ledcDetachPin(PIN_SPEAKER);
}

void testSound2() {
  Serial.println("testing sound 2");
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  ledcWriteNote(speakerChannel, NOTE_C, 1);
  handleRoot();
}

void testSound3() {
  Serial.println("testing sound 2");
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  ledcWriteNote(speakerChannel, NOTE_C, 2);
  handleRoot();
}

void turnSoundOff() {
  Serial.println("turning off sound");
  ledcDetachPin(PIN_SPEAKER);
  handleRoot();
}

void playTwinkleTwinkle() {
  Serial.println("playing twinkle twinkle");
  
  xTaskCreatePinnedToCore(
    playTwinkleTwinkleTaskMethod,   /* Task function. */
    "playTwinkleTwinkleTask",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    0,           /* priority of the task */
    NULL,  /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
      
  handleRoot();
}
    
void playTwinkleTwinkleTaskMethod(void * pvParameters) {
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  vTaskResume(PulseTask);
  delay(250);
  playQuarter(NOTE_C);
  playQuarter(NOTE_C);
  playQuarter(NOTE_G);
  playQuarter(NOTE_G);
  playQuarter(NOTE_A);
  playQuarter(NOTE_A);
  playHalf(NOTE_G);
  delay(NOTE_PAUSE_LENGTH);
  
  playQuarter(NOTE_F);
  playQuarter(NOTE_F);
  playQuarter(NOTE_E);
  playQuarter(NOTE_E);
  playQuarter(NOTE_D);
  playQuarter(NOTE_D);
  playHalf(NOTE_C);
  delay(NOTE_PAUSE_LENGTH);
  
  playQuarter(NOTE_G);
  playQuarter(NOTE_G);
  playQuarter(NOTE_F);
  playQuarter(NOTE_F);
  playQuarter(NOTE_E);
  playQuarter(NOTE_E);
  playHalf(NOTE_D);
  delay(NOTE_PAUSE_LENGTH);
  
  playQuarter(NOTE_G);
  playQuarter(NOTE_G);
  playQuarter(NOTE_F);
  playQuarter(NOTE_F);
  playQuarter(NOTE_E);
  playQuarter(NOTE_E);
  playHalf(NOTE_D);
  delay(NOTE_PAUSE_LENGTH);
  
  playQuarter(NOTE_C);
  playQuarter(NOTE_C);
  playQuarter(NOTE_G);
  playQuarter(NOTE_G);
  playQuarter(NOTE_A);
  playQuarter(NOTE_A);
  playHalf(NOTE_G);
  delay(NOTE_PAUSE_LENGTH);
  
  playQuarter(NOTE_F);
  playQuarter(NOTE_F);
  playQuarter(NOTE_E);
  playQuarter(NOTE_E);
  playQuarter(NOTE_D);
  playQuarter(NOTE_D);
  playHalf(NOTE_C);
  
  ledcDetachPin(PIN_SPEAKER);
  vTaskSuspend(PulseTask); 
  turnOffLed();
  
  handleRoot();
}

void playQuarter(note_t note) {
  playNote(note, 2);
}

void playHalf(note_t note) {
  playNote(note, 4);
}

void playNote(note_t note, int len) {
  ledcWriteNote(speakerChannel, note, 4);
  delay(len * EIGTH_NOTE_LENGTH);
  ledcWrite(speakerChannel, 0);
  delay(NOTE_PAUSE_LENGTH);
}

void turnOn() {
  Serial.println("turning on");
  stopBlink();
  stopPulse();
  turnOnLed();
  startTime = millis();  
  vTaskResume(CheckOnLedTask);
  handleRoot();
}

void turnOff() {
  Serial.println("turning off");
  stopBlink();
  stopPulse();
  turnOffLed();
  vTaskSuspend(CheckOnLedTask);
  handleRoot();
}

void startBlink() {
  Serial.println("turning on blinking");
  stopPulse();
  
  double frequency = ledcReadFreq(ledChannel);
  if (frequency != blinkFreq) {
    if (DEBUG_ON1) Serial.println("setting frequency to " + blinkFreq);
    ledcChangeFrequency(ledChannel, blinkFreq, resolution);
  }
  
  ledcWrite(ledChannel, 50);
  handleRoot();
}

void stopBlink() {
  Serial.println("turning off blinking");
  double frequency = ledcReadFreq(ledChannel);
  if (frequency != (double) freq) {
    if (DEBUG_ON1) Serial.println("setting frequency to " + freq);
    ledcChangeFrequency(ledChannel, freq, resolution);
  }
}
    
void startPulse() {
  Serial.println("turning on pulsing");   
  stopBlink();
  vTaskResume(PulseTask);
  handleRoot();  
}

void stopPulse() {
  Serial.println("turning off pulsing");   
  vTaskSuspend(PulseTask); 
}

void CheckOnLed( void * pvParameters ){
  for(;;){
    bool ledOn = ledcRead(ledChannel) == 100;
    if (DEBUG_ON1) Serial.println("ledOn = " + String(ledOn));
    long elapsedTime = millis() - startTime;
    if (DEBUG_ON1) Serial.println("elapsedTime = " + String(elapsedTime));
    if (ledOn && elapsedTime > 60000) {
      turnOffLed();
      vTaskSuspend(CheckOnLedTask);
    }
   
    delay(1000);
  } 
}

void PulseLed( void * pvParameters ){
  int fadeValue = 0;

  for(;;){
    if (DEBUG_ON1) Serial.println("fadeValue = " + String(fadeValue));
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
      turnOnLed();
      delay(90);
      turnOffLed();
      delay(90);
    }
    if (i < 2) {
      delay(500);
    }
  }
}

void turnOnLed() {
  ledcWrite(ledChannel, 100);
}

void turnOffLed() {
  ledcWrite(ledChannel, 0);
}


void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  httpServer.send(200, "image/svg+xml", out);
}
