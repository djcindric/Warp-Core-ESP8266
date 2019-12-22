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
unsigned long time_pulse_delay = 0;

const char* ssid = "Warp Core";
const char* password = "jasmineislovins"; // has to be longer than 7 chars

CRGB leds[NUM_LEDS];
CRGB primaryColor = CRGB(95,255,255);
CRGB secondaryColor = CRGB::White;

ESP8266WebServer  server(80);

int opMode = 0; //Which color mode to use

//How often to update lights
int INTERVAL_LIGHT_UPDATE = 40;
int INTERVAL_LIGHT_UPDATE2 = 500;

//For pulse light mode
int currentRing = -1;
int totalRings = 20;

//For revolving light mode
int currentColumn = 0;

//For filling light mode
int currentRingInverse = 10;

//For drop-in light mode
int currentRingDropping = 0;
int lastRingDropped = 20;
boolean middleDropped = false;
boolean middleFilled = false;
boolean emptyCore = false;

//For spiral light mode(s)
int currentPixel = 139;
boolean inMiddleRing = false;
boolean doneMiddleRing = false;
int curRingPos = 0;

//Cylon
boolean doReverse = false;

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
  server.on("/changeOpMode", changeOpMode);
  server.on("/changeBrightness", changeBrightness);
  
  
  server.begin();

}

void loop(){
  server.handleClient();  //Handle any incoming network traffic

  //Check that the proper amount of time has passed and then update the current lighting based on opMode
  if(millis() > time_light_update + INTERVAL_LIGHT_UPDATE){    
    time_light_update = millis();

    switch (opMode) {
    case 0:
      standardPulse();
      break;
    case 1:
      fillUp();
      break;
    case 2:
      dropIn();
      break;
    case 3:
      solidColor();
      break;
    case 4:
      revolvingLight();
      break;
    case 5:
      spiralFill(false);
      break;
    case 6:
      spiralFill(true);
      break;
    case 7:
      cylon(true);
      break;
    case 8:
      cylon(false);
      break;
    default:
      solidColor();
      break;
    }
  }
}

//Utility function to fade every LED
void fadeall() { 
  for(int i = 0; i < NUM_LEDS; i++) { 
    leds[i].nscale8(250); 
  }
}

//Lighting mode which cycles through colors while moving up/down
void cylon(boolean keepFilled){
  static uint8_t hue = 0;
  // First slide the led in one direction
  if(!doReverse){
    // Set the i'th led to red 
    leds[curRingPos] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    if(!keepFilled){
      leds[curRingPos] = CRGB::Black;
    }
    fadeall();

    //Manipulate the current position to account for the "middle ring" being at the center of the warp core
    curRingPos = curRingPos + 1;
    if(!doneMiddleRing && curRingPos == 70){
      inMiddleRing = true;
      curRingPos = 140; //Jump to middle ring
    }
    else if(curRingPos == 154){
      inMiddleRing = false; //No longer inside the lighting ring
      doneMiddleRing = true; //Lighting in middle ring has been shown, avoid showing again until the next loop
      curRingPos = 70;
    }
    else if(doneMiddleRing && curRingPos == 139){
      doneMiddleRing = false;
      inMiddleRing = false;
      doReverse = true;
    }
  }else{
    // Set the i'th led to red 
    leds[curRingPos] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    if(!keepFilled){
      leds[curRingPos] = CRGB::Black;
    }
    fadeall();

    //Manipulate the current position to account for the "middle ring" being at the center of the warp core
    curRingPos = curRingPos - 1;
    if(!doneMiddleRing && curRingPos == 69){
      inMiddleRing = true;
      curRingPos = 154; //Jump to middle ring
    }
    else if(inMiddleRing && curRingPos == 140){
      inMiddleRing = false; //No longer inside the lighting ring
      doneMiddleRing = true; //Lighting in middle ring has been shown, avoid showing again until the next loop
      curRingPos = 69;
    }
    else if(curRingPos == 0){
      doneMiddleRing = false;
      inMiddleRing = false;
      doReverse = false;
    }
  }  
}

void spiralFill(boolean keepFilled){
  leds[currentPixel] = primaryColor;

  FastLED.show();
  
  if(!keepFilled){
    for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = CRGB::Black;
    }
  }

  if(inMiddleRing || doneMiddleRing){
    currentPixel = currentPixel + 1;
  }else{
    currentPixel = currentPixel - 1;
  }

  if(doneMiddleRing){
    curRingPos = curRingPos + 1;
    if(curRingPos == 7){
      curRingPos = 0;
      currentPixel = currentPixel - 14;
    }
  }

  if(!doneMiddleRing && currentPixel == 69){
    inMiddleRing = true;
    currentPixel = 140; //Jump to middle ring
  }
  else if(inMiddleRing && currentPixel == 154){
    inMiddleRing = false; //No longer inside the lighting ring
    doneMiddleRing = true; //Lighting in middle ring has been shown, avoid showing again until the next loop
    currentPixel = 63;
  }
  else if(currentPixel == -1){ //We've reached the last pixel in the outer rings, and we've already lit the middle, so start over
    inMiddleRing = false;
    doneMiddleRing = false;
    currentPixel = 139;
    curRingPos = 0;

    for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = CRGB::Black;
    }
  }
  
}


//Light the entire core a solid color
void solidColor(){
  //Set the color for every LED
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = primaryColor;
  }

  FastLED.show();
}

//Fill the core by dropping 1 ring at a time from top to bottom, and then emptying in the reverse
void dropIn(){
  if(emptyCore){
    if(currentRingDropping == lastRingDropped){
      if(currentRingDropping == 11 && !middleFilled){
        for(int dot = 140; dot < 155; dot++){
          leds[dot] = CRGB::Black;
        }
      }else{
        for(int dot = 0; dot < 7; dot++){
          leds[dot + ((currentRingDropping-1) * 7)] = CRGB::Black;
        }
      }
      
    }
    else{
      for(int dot = 0; dot < 7; dot++){
        leds[dot + (currentRingDropping * 7)] = primaryColor;
      }
    }
    
    if(currentRingDropping == 20){
      lastRingDropped = lastRingDropped - 1;
      currentRingDropping = lastRingDropped;
    }

    //Finished emptying core, flip back to filling
    if(lastRingDropped == 0){
      emptyCore = false;
      currentRingDropping = 0;
      lastRingDropped = 20;
      middleFilled = false;
      middleDropped = false;
      
      //Clear the board for next time
      for(int dot = 0; dot < NUM_LEDS; dot++) { 
          leds[dot] = CRGB::Black;
      }

      return;
    }

    //Display lights
    FastLED.show();

    //Clear everything below the last dropped ring
    for(int dot = ((lastRingDropped-1)*7); dot < 140; dot++) {
      leds[dot] = CRGB::Black;
    }

    if(lastRingDropped < 11 && middleFilled){
      //Clear everything above the last dropped ring
      for(int dot = 140; dot < 155; dot++) {
        leds[dot] = CRGB::Black;
      }
    }

    if(currentRingDropping == 11 && !middleFilled){
      middleFilled = true;
    }else{
      currentRingDropping = currentRingDropping + 1;
    }
    
  }else{
    //"Drop" rings until we reach the bottom
    if(currentRingDropping == lastRingDropped){
      
      currentRingDropping = 0;
      middleDropped = false;
  
      if(lastRingDropped == 11 && !middleFilled){
        middleFilled = true;
      }else{
        lastRingDropped = lastRingDropped - 1;
      }
    }
  
    //Drop the middle ring
    if(currentRingDropping == 10 && !middleDropped){
      middleDropped = true;
      
      for(int dot = 140; dot < 155; dot++){
        leds[dot] = primaryColor;
      }
    }else{
      for(int dot = 0; dot < 7; dot++){
        leds[dot + (currentRingDropping * 7)] = primaryColor;
      }
  
      currentRingDropping = currentRingDropping + 1;
    }
    
    //Display lights
    FastLED.show();

    //Core is filled, flip to empty mode
    if(lastRingDropped == 0){
        emptyCore = true;
        currentRingDropping = 20;
        lastRingDropped = 20;
        middleFilled = false;
        middleDropped = false;

        return;
    }
  
    //Clear everything above the last dropped ring
    for(int dot = 0; dot < ((lastRingDropped-1)*7); dot++) {
      leds[dot] = CRGB::Black;
    }
  
    if(!middleFilled){
      //Clear everything above the last dropped ring
      for(int dot = 140; dot < 155; dot++) {
        leds[dot] = CRGB::Black;
      }
    }
  }  
}

//Light the center ring and slowly fill the top/bottom halves from the center out
void fillUp(){

  //Fill center for the first iteration
  if(currentRingInverse == 10){
    //Set the middle ring solid
    for(int dot = 140; dot < 155; dot++){
      leds[dot] = primaryColor;
    }
  }else{
    //Set the current ring in the top/bottom halves blue
    for(int dot = 0; dot < 7; dot++){
      leds[dot + (currentRingInverse * 7)] = primaryColor;
      leds[NUM_LEDS_OUTER - (dot + (currentRingInverse * 7))] = primaryColor; 
    }
  }
  
  //Display lights
  FastLED.show();

  //Increment ring counter for next time
  currentRingInverse = currentRingInverse - 1;

  //Roll back over once we hit the last ring
  if(currentRingInverse < 0){
    currentRingInverse = 10;

    //Clear the board for next time
    for(int dot = 0; dot < NUM_LEDS; dot++) { 
        leds[dot] = CRGB::Black;
    }
    
    delay(400);
  }
}

//Light mode which lights a column from top to bottom and rotates clockwise
void revolvingLight(){
  int bottomColumn = (currentColumn + 5) % 7;
  int midColumn = (currentColumn + 2) % 7;
  
  //Light a column down the 20 main rings
  for(int row = 0; row < 10; row++){
    leds[(row*7) + currentColumn] = primaryColor;
  }

  for(int row = 10; row < 20; row++){
    leds[(row*7 + 7) - bottomColumn] = primaryColor;
  }

  //Light 2 LEDs in the middle as it is double length
  leds[140 + midColumn*2] = primaryColor;
  leds[141 + midColumn*2] = primaryColor;

  //Display lights
  FastLED.show();

  //Clear the board for next time
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot] = CRGB::Black;
  }

  //Increment ring counter for next time
  currentColumn = currentColumn + 1;

  //Reset the counter once we perform a full rotation
  if(currentColumn == 7){
    currentColumn = 0;
  }
}

//Default light mode which lights the center and "pulses" from the top/bottom inward
void standardPulse(){

  //Set the middle ring solid
  for(int dot = 140; dot < 155; dot++){
    leds[dot] = secondaryColor;
  }

  //"Ring 11" is a dead state where we only want to pause on black
  if(currentRing < 11){
    //Set the current ring in the top/bottom halves blue
    for(int dot = 0; dot < 7; dot++){

      //Light entire ring if we're not on ring -1
      if(currentRing >= 0){
        leds[dot + (currentRing * 7)] = primaryColor;
        leds[NUM_LEDS_OUTER - (dot + (currentRing * 7))] = primaryColor; 
      }

      //Light outer most ring partially for "-1" to fake an intro ring
      if(currentRing < 0){
        leds[dot] = primaryColor;
        leds[NUM_LEDS_OUTER - dot] = primaryColor; 
        leds[dot].fadeLightBy( 220 );
        leds[NUM_LEDS_OUTER - dot].fadeLightBy( 220 );
      }

      //Fade ring above for ring 2+
      if(currentRing > 0){
        leds[dot + ((currentRing-1) * 7)] = primaryColor;
        leds[dot + ((currentRing-1) * 7)].fadeLightBy( 190 );

        leds[NUM_LEDS_OUTER - (dot + ((currentRing-1) * 7))] = primaryColor;
        leds[NUM_LEDS_OUTER - (dot + ((currentRing-1) * 7))].fadeLightBy( 190 ); 

        //Fade ring below up to the last
        if(currentRing < 10){
          leds[dot + ((currentRing+1) * 7)] = primaryColor;
          leds[dot + ((currentRing+1) * 7)].fadeLightBy( 190 );

          leds[NUM_LEDS_OUTER - (dot + ((currentRing+1) * 7))] = primaryColor;
          leds[NUM_LEDS_OUTER - (dot + ((currentRing+1) * 7))].fadeLightBy( 190 );
        }
      }
  
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
    if(millis() > time_pulse_delay + (20*INTERVAL_LIGHT_UPDATE)){    
      time_pulse_delay = millis();

      currentRing = 0;
    }
  }
}

//Checks HTTP request and changes the primary color for lighting
//ARGS: Red, Green, Blue (RGB integer values), Which (which color to update, 0 for primary, 1 secondary etc)
void changeColor(){
  resetEverything();
  
  int red = 255;
  int green = 255;
  int blue = 255;
  int whichLight = 0;

  if(server.hasArg("red")) {
    red = server.arg("red").toInt();
  }
  if(server.hasArg("green")) {
    green = server.arg("green").toInt();
  }
  if(server.hasArg("blue")) {
    blue = server.arg("blue").toInt();
  }
  if(server.hasArg("whichLight")) {
    whichLight = server.arg("whichLight").toInt();
  }

  Serial.print("Changing color to red: ");
  Serial.print(red);
  Serial.print(", Green: ");
  Serial.print(green);
  Serial.print(", Blue: ");
  Serial.print(blue);
  Serial.println("");

  switch (whichLight) {
    case 0:
      primaryColor = CRGB( red, green, blue);
      break;
    case 1:
    default:
      secondaryColor = CRGB( red, green, blue);
      break;
   }

  server.send(200, "text/plain", "Changed Color Successfully");
}

//Checks HTTP request and changes the delay between lighting updates
void changeSpeed(){
  resetEverything();
  
  //Check HTTP args for delay amount and set
  if( server.hasArg("delay")) {
    int delayAmount = server.arg("delay").toInt();
    if(delayAmount >= 0){
      Serial.print("Changing delay amount to: ");
      Serial.println(delayAmount);
  
      INTERVAL_LIGHT_UPDATE = delayAmount;

      server.send(200, "text/plain", "Changed Delay Successfully");
      return;
    }
  }
  
  server.send(200, "text/plain", "Failed to change delay");
}

//Checks HTTP request and changes the brightness of the warp core
void changeBrightness(){
  resetEverything();
  
  //Check HTTP args for the amount and set brightness
  if( server.hasArg("amount")) {
    int newBrightness = server.arg("amount").toInt();
    if(newBrightness >= 0 && newBrightness <= 255){
      Serial.print("Changing brightness to: ");
      Serial.println(newBrightness);

      FastLED.setBrightness(newBrightness);

      //Send HTTP 200 response
      server.send(200, "text/plain", "Changed Brightness Succesfully");
      return;
    }
  }

  //Send HTTP 200 response
  server.send(200, "text/plain", "Failed to change brightness");
}

//Checks HTTP request and updates the lighting mode
void changeOpMode(){
  resetEverything();

  //Check HTTP args for the op mode and set
  if( server.hasArg("opMode")) {
    int newOpMode = server.arg("opMode").toInt();
    opMode = newOpMode;

    Serial.print("Changing opMode to: ");
    Serial.println(newOpMode);

    //Also change delay to the "optimal" for each mode
    switch (opMode) {
      case 0: //Pulse
        INTERVAL_LIGHT_UPDATE = 40;
        break;
      case 1: //Fill
        INTERVAL_LIGHT_UPDATE = 50;
        break;
      case 2: //Drop-in
        INTERVAL_LIGHT_UPDATE = 30;
        break;
      case 3: //Solid
        break;
      case 4: //Revolving
        INTERVAL_LIGHT_UPDATE = 70;
        break;
      case 5: //Spiral 1
        INTERVAL_LIGHT_UPDATE = 25;
        break;
      case 6: //Spiral 2
        INTERVAL_LIGHT_UPDATE = 35;
        break;
      case 7: //Cylon 1
        INTERVAL_LIGHT_UPDATE = 10;
        break;
      case 8: //Cylon 2
        INTERVAL_LIGHT_UPDATE = 20;
        break;
      default:
        INTERVAL_LIGHT_UPDATE = 40;
        break;
      
    }

    //Send HTTP 200 response
    server.send(200, "text/plain", "Changed lighting mode successfully");
    return;
  }

  //Send HTTP 200 response
  server.send(200, "text/plain", "Failed to change lighting mode");
}

void resetEverything(){
  //Clean slate for all variables that affect lighting
  emptyCore = false;
  currentRingDropping = 0;
  lastRingDropped = 20;
  middleFilled = false;
  middleDropped = false;
  currentRing = -1;
  currentColumn = 0;
  currentRingInverse = 10;
  time_pulse_delay = millis();
  currentPixel = 139;
  inMiddleRing = false;
  doneMiddleRing = false;
  curRingPos = 0;
  doReverse = false;

  //Clear all lights
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
    leds[dot] = CRGB::Black;
  }
}
