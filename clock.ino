#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

/*
 * Stable, doesn't use high performance FastLED library
 */

const bool interpolateHour = true;   //uses minutes to give hour hand intermediate values
const bool wideHourHand = true;    //expands hour hand to 3/60ths
bool smoothHands = true;   //smooth transitions for second and minute hands
const bool autoSmoothHands = true;
const bool randomColors = true;    //colors change at 12:00
const byte boldMarkers = 4;   //brighter markers
const byte markers = 12;    //number of markers
const byte tickRate = 20;   //updates per second
const bool upsideDown = true;   //if cable is on the bottom

RTC_DS1307 rtc;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, 9, NEO_GRB + NEO_KHZ800);

byte hourLED = 0;
byte minuteLED = 0;
byte secondLED = 0;
bool second_flag = 0;

byte colors [3][3];  //initialize {hour, minute, second, marker}
byte markerColors[2][3] = {   // bold, regular
  {63, 63, 63},
  {31, 31, 31}
};

const byte RGBColors [][3] = {
  {255, 0, 0},
  {0, 255, 0},
  {0, 0, 255}
  };
bool rainbow = LOW;
bool fluid = LOW;

class LEDClass {
  private:
    byte ID;
    byte tempRed = 0, tempGreen = 0, tempBlue = 0;
  public:
    void setID(byte IDnum) {
      ID = IDnum;
    }
    void addTempColor(byte r, byte g, byte b) {   //method for blending multiple colors, takes highest R, G, B values
      if (r > tempRed) tempRed = r;
      if (g > tempGreen) tempGreen = g;
      if (b > tempBlue) tempBlue = b;
    }
    void clearPixel() {
      tempRed = 0;
      tempBlue = 0;
      tempGreen = 0;
      setPixel();
    }
    void setPixel() {
        strip.setPixelColor(ID, tempRed, tempBlue, tempGreen);
    }
} LED[60];    //initialize 60 instances for 60 LEDs

void setup () {
  pinMode(A0, INPUT_PULLUP);    //ldr
  //pinMode(8, INPUT_PULLUP);   //button
  pinMode(A3, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A3, HIGH);
  digitalWrite(A2, LOW);
  randomSeed(analogRead(A0));

  if (randomColors) changeColors();
  strip.begin();
  strip.show();
  
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  //set time COMMENT THIS
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  for (int i = 0, j = 0; i < 4 && j < 3; i++, j++) colors[i][j] = RGBColors[i][j];    //sets default colors
  
  if (upsideDown) {   //identify every LED instance 0-59
    for (byte i = 0; i < 30; i++) LED[i].setID(i + 30);
    for (byte i = 30; i < 60; i++) LED[i].setID(i - 30);
  }
  else {
    for (byte i = 0; i < 60; i++) LED[i].setID(i);    
  }
}

void changeColors() {     //runs on startup and 12:
  byte x = random(3);     //number of color schemes
  rainbow = LOW;
  fluid = LOW;
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 3; j++) {
      switch(x) {
        case 1:
          rainbow = HIGH;
          break;
        case 2:
          fluid = HIGH;
          break;
        default:    //case 0
          colors[i][j] = RGBColors[i][j];
          break;
      }
    }
  }
  Serial.print("Colors scheme "); Serial.println(x);
  delay(1000);
}

byte getAmbientLight() {
  static int rawLightLevel = 1024;    // 1024: begin at 0 brightness
  static byte hysterisis = 25;        // 25 light levels out of 1024
  if (analogRead(A0) > rawLightLevel + hysterisis) rawLightLevel += 1;
  else if (analogRead(A0) < rawLightLevel - hysterisis) rawLightLevel -= 1;
  int lightLevel = map(rawLightLevel, 250, 1000, 255, 0);
  lightLevel = constrain(lightLevel, 3, 255);
  if (second_flag) {
    Serial.print("Brightness: "); Serial.print(rawLightLevel); Serial.print(" => "); Serial.println(lightLevel);
    Serial.println("======================");
  }
  if (autoSmoothHands) {
    if (lightLevel <= 5) smoothHands = LOW;   //automatically disable smoothing in very low light (avoids flickering)
    else smoothHands = HIGH;
  }
  return lightLevel;
}

void setRainbowColors() {   //counterpart of changeColors() function,
  static byte oldSecond;
  if (secondLED != oldSecond) {
    oldSecond = secondLED;
    byte rainbowColors[][3] = {{rainbowR(hourLED), rainbowG(hourLED), rainbowB(hourLED)},
    {rainbowR(minuteLED), rainbowG(minuteLED), rainbowB(minuteLED)},
    {rainbowR(secondLED), rainbowG(secondLED), rainbowB(secondLED)}};
    for (byte i = 0; i < 3; i++) {
      for (byte j = 0; j < 3; j++) {
        colors[i][j] = rainbowColors[i][j];
      }
    }
  }
}

void setFluidColors() {
  static byte oldSecond;
  static byte i = random(60);
  if (secondLED != oldSecond) {
    oldSecond = secondLED;
    byte x = random(2);
    switch (x) {
      case 0:
        if (i < 59) i += 1;
        else i = 0;
      case 1:
        if (i > 0) i -= 1;
        else i = 59;
    }
    byte h = i, m = i + 20, s = i + 40;   //hour, minute and second colors are evenly spaced
    if (m > 59) m -= 60;
    if (s > 59) s -= 60;
    byte fluidColors[][3] = {{rainbowR(h), rainbowG(h), rainbowB(h)},
      {rainbowR(m), rainbowG(m), rainbowB(m)},
      {rainbowR(s), rainbowG(s), rainbowB(s)}};
    for (byte i = 0; i < 3; i++) {
      for (byte j = 0; j < 3; j++) {
        colors[i][j] = fluidColors[i][j];
      }
    }
  }
}

byte rainbowR(byte i) {
  byte pos = i * 255 / 59;
  if (pos < 85) return 255 - pos * 3;
  else if (pos < 170) return 0;
  else return (pos - 170) * 3;
}
byte rainbowG(byte i) {
  byte pos = i * 255 / 59;
  if (pos < 85) return 0;
  else if (pos < 170) return (pos - 85) * 3;
  else return 255 - (pos - 170) * 3;
}
byte rainbowB(byte i) {
  byte pos = i * 255 / 59;
  if (pos < 85) return pos * 3;
  else if (pos < 170) return 255 - (pos - 85) * 3;
  else return 0;
}

void updateClock() {
  static int oldSecond = 0;
  DateTime now = rtc.now();
//  hourLED = now.hour();
//  hourLED = (now.hour() * 5 % 60) + interpolateHour ? minuteLED/12 : 0;
  secondLED = now.second();
  minuteLED = now.minute();
  hourLED = now.hour() % 12 * 5 + (interpolateHour ? now.minute()/12 : 0);
  if (oldSecond != secondLED) {
    Serial.print("The time is "); Serial.print(hourLED/5); Serial.print(":"); Serial.print(now.minute()); Serial.print(":"); Serial.println(now.second());
    oldSecond = secondLED;
    second_flag = 1;   //tell getAmbientLight to print
  }
  else second_flag = 0;
}

void setHands() {
  if (rainbow) setRainbowColors();    //configures rainbow in real time
  if (fluid) setFluidColors();
  for (byte i = 0; i < 60; i++) {
    if (i % (60/boldMarkers) == 0)
      LED[i].addTempColor(markerColors[0][0], markerColors[0][1], markerColors[0][2]);    //set bold markers
    if (i % (60/markers) == 0)
      LED[i].addTempColor(markerColors[1][0], markerColors[1][1], markerColors[1][2]);    //set markers
    if ((i == secondLED) && !smoothHands)    //skip if smooth hands enabled
      LED[i].addTempColor(colors[2][0], colors[2][1], colors[2][2]);
    if ((i == minuteLED) && !smoothHands)
      LED[i].addTempColor(colors[1][0], colors[1][1], colors[1][2]);
    if (i == hourLED)
      LED[i].addTempColor(colors[0][0], colors[0][1], colors[0][2]);
  }
  if (wideHourHand) setWideHourHand();
  if (smoothHands) {
    setSmoothMinuteHand();
    setSmoothSecondHand();
  }
}

void setWideHourHand() {    //sets pixels on either side of the hour
  for (byte i = 0; i < 60; i++) {
    if ((abs(i - hourLED) == 1) || (abs(i - hourLED) == 59))
      LED[i].addTempColor(colors[0][0], colors[0][1], colors[0][2]);
  }
}

void setSmoothMinuteHand() {
  static byte currentTick, oldMinute;
  if (currentTick < tickRate) currentTick += 1;   //constrains currentTick to tickRate
  if (oldMinute != minuteLED) {
    oldMinute = minuteLED;
    currentTick = 0;
  }
  byte tempColorsTrailing[] = {
    (colors[1][0] * (tickRate - currentTick)) / tickRate,
    (colors[1][1] * (tickRate - currentTick)) / tickRate,
    (colors[1][2] * (tickRate - currentTick)) / tickRate
    };
  byte tempColorsLeading[] = {
    colors[1][0] * currentTick / tickRate,
    colors[1][1] * currentTick / tickRate,
    colors[1][2] * currentTick / tickRate
    };
  for (byte i = 0; i < 60; i++) {
    if ((i-1) == minuteLED || (i+59) == minuteLED)            //leading pixel
      LED[i].addTempColor(tempColorsLeading[0], tempColorsLeading[1], tempColorsLeading[2]);
    if (i == minuteLED)
//    if (i == minuteLED)                                       //actual pixel
      LED[i].addTempColor(colors[1][0], colors[1][1], colors[1][2]);
//      LED[i].addTempColor(tempColorsLeading[0], tempColorsLeading[1], tempColorsLeading[2]);
    if (((i + 1) == minuteLED) || ((i - 59) == minuteLED))    //trailing pixel
      LED[i].addTempColor(tempColorsTrailing[0], tempColorsTrailing[1], tempColorsTrailing[2]);
  }
}

void setSmoothSecondHand() {
  static byte oldSecond;
  static unsigned long oldMillis = millis();
  int millisCount = millis() - oldMillis;
  if (oldSecond != secondLED) {
    oldSecond = secondLED;
    oldMillis = millis();
    millisCount = 0;
  }
  byte currentTick = tickRate * millisCount / 1000;   //this isn't the ACTUAL tick, just what the tick should be.
  byte tempColorsTrailing[] = {
    (colors[2][0] * (tickRate - currentTick)) / tickRate,
    (colors[2][1] * (tickRate - currentTick)) / tickRate,
    (colors[2][2] * (tickRate - currentTick)) / tickRate
    };
  byte tempColorsLeading[] = {
    colors[2][0] * currentTick / tickRate,
    colors[2][1] * currentTick / tickRate,
    colors[2][2] * currentTick / tickRate
    };
  for (byte i = 0; i < 60; i++) {
    if (i == secondLED)    //leading pixel
      LED[i].addTempColor(tempColorsLeading[0], tempColorsLeading[1], tempColorsLeading[2]);
    if (((i + 1) == secondLED) || ((i - 59) == secondLED))   //trailing pixel
      LED[i].addTempColor(tempColorsTrailing[0], tempColorsTrailing[1], tempColorsTrailing[2]);
  }
}

void loop () {
  strip.setBrightness(getAmbientLight());
  if (randomColors && hourLED == 0 && minuteLED == 0 && secondLED == 0) changeColors();
  updateClock();
  for (byte i = 0; i < 60; i++) LED[i].clearPixel();    //reset pixels before beginning
  setHands();
  for (byte i = 0; i < 60; i++) LED[i].setPixel();
  strip.show();
  delay(1000 / tickRate);
}
