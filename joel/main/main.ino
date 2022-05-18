#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include "WiFiCredentials.h"
#include "esp32-hal-log.h"

#define PIN_LED 2
#define PIN_SPEAKER 4

#define freq 5000
#define BLINK_FREQ_REGULAR 5
#define BLINK_FREQ_SEIZURE 10
#define ledChannel 0
#define speakerChannel 1
#define speakerFreq 10000
#define resolution 8
#define NOTE_PAUSE_LENGTH 150
#define EIGTH_NOTE_LENGTH 200

#define DEBUG_ON1 true

bool pulseDirection = true;
bool play = false;
long startTime = millis();

TaskHandle_t PulseTask;
TaskHandle_t CheckOnLedTask;
TaskHandle_t FlashReadyTask;
TaskHandle_t twinkleTask;
TaskHandle_t sweepSoundTask;
//TaskHandle_t runningTask;

WebServer httpServer(80);
HTTPUpdateServer httpUpdater;

void handleRoot() {
//  char temp[400];
//  int sec = millis() / 1000;
//  int min = sec / 60;
//  int hr = min / 60;

  log_v("Entering handleRoot");
  
  String response = "<html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <link rel=\"icon\" href=\"data:,\">\
    <title>Joel</title>\
    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC\" crossorigin=\"anonymous\">\
    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-MrcW6ZMFYlzcLA8Nl+NtUVF0sA7MsXsP1UyJoMp4YLEuNSfAP+JcXn/tWtIaxVXM\" crossorigin=\"anonymous\"></script>\
  </head>\
  <body>\
    <div class=\"container-fluid px-2\">\
      <div class=\"row m-1\">\
        <div class=\"col\">\
          <a href=\"/On\" class=\"btn btn-lg btn-success\">On</a>\
          <a href=\"/Off\" class=\"btn btn-lg btn-danger\">Off</a>\
        </div>\
      </div>\
      <div class=\"row m-1\">\
        <div class=\"col\">\
        <div class=\"btn-group\" role=\"group\" aria-label=\"Blinking options\">\
          <a href=\"/Pulse\" class=\"btn btn-primary\">Pulse</a>\
          <a href=\"/Blink\" class=\"btn btn-primary\">Blink</a>\
          <a href=\"/Seizure\" class=\"btn btn-primary\">Seizure</a>\
        </div>\
        </div>\
      </div>\
      <div class=\"row m-1\">\
        <div class=\"col\">\
        <a href=\"/twinkle\">Play Twinkle, Twinkle</a><br>\
        </div>\
      </div>\
      <div class=\"row m-1\">\
        <div class=\"col\">\
        Click <a href=\"/update\">here</a> to update the firmware.\
        </div>\
      </div>\
    </div>\
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

String getStatus(wl_status_t val) {
  return getStatus((int) val);
}

String getStatus(int val) {
  if (val == WL_IDLE_STATUS) {
    return "IDLE";
  }
  else if (val == WL_NO_SSID_AVAIL) {
    return "NO_SSID_AVAIL";
  }
  else if (val == WL_SCAN_COMPLETED) {
    return "SCAN_COMPLETED";
  }
  else if (val == WL_CONNECTED) {
    return "CONNECTED";
  }
  else if (val == WL_CONNECT_FAILED) {
    return "CONNECT_FAILED";
  }
  else if (val == WL_CONNECTION_LOST) {
    return "CONNECTION_LOST";
  }
  else if (val == WL_DISCONNECTED) {
    return "DISCONNECTED";
  }
  return "unknown";
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
    Serial.print(ssid);
    Serial.println("...");


    wl_status_t status = WiFi.begin(ssid, password);
    
    log_d("begin status=%s", getStatus(status));
    int waitForConnectResultStatus = WiFi.waitForConnectResult();
    log_d("wait for result status=%s", getStatus(status));
    int reconnectDelay = 10000;
    while (waitForConnectResultStatus == WL_CONNECT_FAILED) {
      Serial.print("WiFi failed, retrying in ");
      Serial.print(reconnectDelay / 1000);
      Serial.println(" seconds...");
      
      delay(reconnectDelay);
      reconnectDelay *= 2;
      WiFi.begin(ssid, password);
      waitForConnectResultStatus = WiFi.waitForConnectResult();
      log_d("wait for result repeat status=%s", getStatus(waitForConnectResultStatus));
    }
    
    status = WiFi.status();
    log_d("wifi status=%s", getStatus(status));
    while (status != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      status = WiFi.status();
      log_d("wifi repeat status=%s", getStatus(status));
    }
    Serial.println("status=" + getStatus(status));

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

// power savings?
//    WiFi.setSleep(false);
    
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
      log_d("CheckOnLedTask created");
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
      log_d("PulseTask created");
      vTaskSuspend(PulseTask); 
    }

    xReturned = xTaskCreatePinnedToCore(
        playTwinkleTwinkleTaskMethod,   /* Task function. */
        "twinkleTask",     /* name of task. */
        10000,       /* Stack size of task */
        NULL,        /* parameter of the task */
        0,           /* priority of the task */
        &twinkleTask,  /* Task handle to keep track of created task */
        0);          /* pin task to core 0 */
    if (xReturned == pdPASS)
    {
      log_d("twinkleTask created");
      vTaskSuspend(twinkleTask); 
    }

    MDNS.addService("http", "tcp", 80);
    
    httpServer.on("/", handleRoot);
    httpServer.on("/On", turnOn);
    httpServer.on("/Off", turnOff);
    httpServer.on("/Pulse", startPulse);
    httpServer.on("/Blink", startBlinkRegular);
    httpServer.on("/Seizure", startBlinkSeizure);
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

    log_d("HTTPUpdateServer ready! Open http://%s/update in your browser\n", localIP);
  
    FlashReady();
}

void loop() {

  // TODO: look at WiFi events: https://techtutorialsx.com/2019/08/11/esp32-arduino-getting-started-with-wifi-events/
  
  wl_status_t status = WiFi.status();
  log_v("wifi status=%s", getStatus(status));
  if (status != WL_CONNECTED) {
    // reconnect
    Serial.println("Reconnecting...");
    status = WiFi.begin(ssid, password);
  }
  while (status != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    status = WiFi.status();
    log_v("wifi repeat status=%s", getStatus(status));
  }
  log_v("status=", getStatus(status));
    
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
  log_i("testing sound 2");
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  ledcWriteNote(speakerChannel, NOTE_C, 1);
  handleRoot();
}

void testSound3() {
  log_i("testing sound 3");
  ledcAttachPin(PIN_SPEAKER, speakerChannel);
  delay(250);
  ledcWriteNote(speakerChannel, NOTE_C, 2);
  handleRoot();
}

void turnSoundOff() {
  log_i("turning off sound");
  ledcDetachPin(PIN_SPEAKER);
  handleRoot();
}

void playTwinkleTwinkle() {
  log_i("playing twinkle twinkle");
  
  vTaskResume(twinkleTask);
  play = true;
  
  handleRoot();
}
    
void playTwinkleTwinkleTaskMethod(void * pvParameters) {

  for (;;) {
    if (play) {
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
      play = false;
    }
    delay(200);
  }
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
  log_i("turning on");
  stopBlink();
  stopPulse();
  turnOnLed();
  startTime = millis();  
  vTaskResume(CheckOnLedTask);
  handleRoot();
}

void turnOff() {
  log_i("turning off");
  stopBlink();
  stopPulse();
  log_v("About to turn off LED");
  turnOffLed();
  log_v("About to suspend CheckOnLedTask");
  vTaskSuspend(CheckOnLedTask);
  
  // TODO: get twinkleTask state. if running, stop. suspending task doesn't work. add flag to check?
  log_v("About to detach speaker");
  ledcDetachPin(PIN_SPEAKER);
  log_v("About to suspend twinkleTask");
  vTaskSuspend(twinkleTask);
  
  log_v("About to call handleRoot");
  handleRoot();
}

void startBlinkRegular() {
  startBlink(BLINK_FREQ_REGULAR);
}

void startBlinkSeizure() {
  startBlink(BLINK_FREQ_SEIZURE);
}

void startBlink(int blinkFrequency) {
  log_i("turning on blinking at rate of %d", blinkFrequency);
  stopPulse();
  
  double frequency = ledcReadFreq(ledChannel);
  if (frequency != blinkFrequency) {
    log_d("setting frequency to %d", blinkFrequency);
    ledcChangeFrequency(ledChannel, blinkFrequency, resolution);
  }
  
  ledcWrite(ledChannel, 50);
  handleRoot();
}

void stopBlink() {
  log_i("turning off blinking");
  double frequency = ledcReadFreq(ledChannel);
  log_d("frequency=%d", frequency);
  log_d("freq=%d", freq);
  log_d("(int) frequency != freq=%d", (int) frequency != freq);
  if ((int) frequency != freq) {
    log_d("setting frequency to %d", freq);
    ledcChangeFrequency(ledChannel, freq, resolution);
  }
  log_d("exiting stopBlink");
}
    
void startPulse() {
  log_i("turning on pulsing");   
  stopBlink();
  vTaskResume(PulseTask);
  handleRoot();  
}

void stopPulse() {
  log_i("turning off pulsing");   
  vTaskSuspend(PulseTask); 
  log_d("exiting stopPulse");
}

void CheckOnLed( void * pvParameters ){
  for(;;){
    bool ledOn = ledcRead(ledChannel) == 100;
    log_d("ledOn=%b", ledOn);
    long elapsedTime = millis() - startTime;
    log_d("elapsedTime=%d", elapsedTime);
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
    log_v("fadeValue=%d", fadeValue);
    if (pulseDirection) {
      fadeValue++;
      if (fadeValue > 99) {
        pulseDirection = false;
      }
    }
    else {
      fadeValue--;
      if (fadeValue < 1) {
        pulseDirection = true;
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
