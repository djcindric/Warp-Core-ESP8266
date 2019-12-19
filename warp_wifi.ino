#include <FastLED.h>
#include<ESP8266WiFi.h>
#include <ESP8266WebServer.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    4
#define BUTTON_PIN_1  0
#define BUTTON_PIN_2  2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    155
#define NUM_LEDS_OUTER    140
#define BRIGHTNESS          255
 
unsigned long time_light_update = 0;

const char* ssid = "Warp Core";
const char* password = "agilexwifi"; // has to be longer than 7 chars

CRGB leds[NUM_LEDS];
CRGB whichColor = CRGB::Blue;

ESP8266WebServer  server(80);

int counter = 0;

int opMode = 0; //Which color mode to use

int INTERVAL_LIGHT_UPDATE = 100;
int INTERVAL_LIGHT_UPDATE2 = 500;

int currentRing = 0;
int totalRings = 20;

void setup() {
  
  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  
  Serial.println("Booting warp core...");

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  //Set WIFI mode to AP so we can connect directly and not be tied to a local network
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1, 0); // 1 = Visible, 0 = hidden

  server.on("/changeColor", changeColor);
  server.on("/changeSpeed", changeSpeed);
  
  server.begin();

}

void loop(){
  server.handleClient();  //Handle any incoming network traffic

  if(millis() > time_light_update + INTERVAL_LIGHT_UPDATE){
    Serial.println("Updating current lighting");
    
    time_light_update = millis();

    switch (opMode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    default:
      standardPulse();
      break;
    }
  }
}

void changeColor(){
  Serial.print("Num args: ");
  Serial.println(server.args());

  int red = 255;
  int green = 255;
  int blue = 255;

  if( server.hasArg("Red")) {
    red = server.arg("Red").toInt();
  }
  if( server.hasArg("Green")) {
    green = server.arg("Green").toInt();
  }
  if( server.hasArg("Blue")) {
    blue = server.arg("Blue").toInt();
  }

  Serial.print("Changing color to red: ");
  Serial.print(server.arg("Red"));
  Serial.print(", Green: ");
  Serial.print(server.arg("Green"));
  Serial.print(", Blue: ");
  Serial.print(server.arg("Blue"));
  Serial.println("");

  whichColor = CRGB( red, green, blue);

  server.send(200, "text/plain", "Changed color");
}

void changeSpeed(){
  Serial.print("Num args: ");
  Serial.println(server.args());

  if( server.hasArg("delay")) {
    int delayAmount = server.arg("delay").toInt();
    if(delayAmount > 0){
      Serial.print("Changing delay amount to: ");
      Serial.println(INTERVAL_LIGHT_UPDATE);
  
      INTERVAL_LIGHT_UPDATE = delayAmount;
    }
  }
  
  server.send(200, "text/plain", "Changed speed");
}

void standardPulse(){

  //Set the middle ring solid
  for(int dot = 140; dot < 155; dot++){
    leds[dot] = whichColor;
  }

  //"Ring 11" is a dead state where we only want to pause on black
  if(currentRing < 11){
    //Set the current ring in the top/bottom halves blue
    for(int dot = 0; dot < 7; dot++){
      leds[dot + (currentRing * 7)] = whichColor;
      leds[NUM_LEDS_OUTER - (dot + (currentRing * 7))] = whichColor; 
  
      //Do some fading as "ring 10" is really just a transition into the middle
      if(currentRing == 10){
        leds[dot + (63)].fadeLightBy( 128 );
        leds[NUM_LEDS_OUTER - (dot + 63)].fadeLightBy(190);
      }
    }  
  }

  //Display lights
  FastLED.show();

  //Clear the board for next time
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = CRGB::Black;
  }

  //Increment ring counter for next time
  currentRing = currentRing + 1;

  //Roll back over to ring 1 if we hit the end (10 in each half, 0 based)
  if(currentRing > 11){
    currentRing = 0;
    delay(500);
  }
}
