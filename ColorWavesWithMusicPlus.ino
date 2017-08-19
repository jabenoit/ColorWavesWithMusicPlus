#include "FastLED.h"
#include "IRremote.h"
#include <avr/wdt.h>

// Multiple RGB LED strips code - 
//    Mix of ColorWavesWithPalettes (to handle 3 strips uniquely)
//    SparkFun Music Visualizer Tutorial (process frequency data, find "bump"
//    SparkFun Simple IR Remote (simple control on modes, palettes, bump speed)
//    Adafruit Twinkle (use FastLED library and palettes)
//    my own Fireworks code (adaptation of Twinkle - only does 1 LED at a time, unlike matrix/array versions) 
/*        AFAIK, this will *not* run on Uno's and other devices w/ only 2KB memory -
             you'll need to take out some code (probably either IR remote or Music Vis
             code would be enough) - threshold is somewhere in the 800+ bytes for local
             variables. */
// Rev1.3 - by John Benoit, 19 Aug 2017 */

/* ColorWavesWithPalettes
   Animated shifting color waves, with several cross-fading color palettes.
   by Mark Kriegsman, August 2015
  
   Color palettes courtesy of cpt-city and its contributors:
     http://soliton.vm.bytemark.co.uk/pub/cpt-city/
  
   Color palettes converted for FastLED using "PaletteKnife" v1:
     http://fastled.io/tools/paletteknife/
*/

/* SparkFun Addressable RGB LED Sound and Music Visualizer Tutorial Arduino Code by Michael Bartlett
   https://github.com/bartlettmic/SparkFun-RGB-LED-Music-Sound-Visualizer-Arduino-Code/blob/master/Visualizer_Program/Visualizer_Program.ino
 */

/*  SparkFun Electronics 2013 - Playing with Infrared Remote Control
  
  IR Receiver Breakout (SEN-8554): Supply voltage of 2.5V to 5.5V
    Attach
    OUT: To pin 11 on Arduino
    GND: GND
    VCC: 5V
  
  This is based on Ken Shirriff's code found on GitHub:
  https://github.com/shirriff/Arduino-IRremote/

  This code works with cheap remotes. If you want to look at the individual timing
  of the bits, use this code:
  http://www.arduino.cc/playground/Code/InfraredReceivers
*/

//*********************************************************************************
// Global Debug - set to 0 to remove all serial.print calls
#define DEBUG_MODE 1

//*********************************************************************************
// LED/FastLED globals
#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
#define BRIGHTNESS  255

#define NUM_LED_STRIPS   3
#define STRIP_DATA_PIN0 10
#define STRIP_DATA_PIN1  9
#define STRIP_DATA_PIN2  8
const uint8_t STRIP_NUM_LEDS[]     = { 12,  6,  1 }; // list base / outer ring first, so the spiral pattern will go in and up
const uint8_t STRIP_SPIRAL_START[] = {  8,  4,  0 }; // 1st LED in the Spiral 
const uint8_t STRIP_BAND_MAP[]     = {  0,  3,  6 }; // Strip 2 uses band 1 (Bass), Strip 1 uses band 3 (Tenor), Strip 0 uses band 5 (Soprano)
const uint8_t STRIP_BUMP_SCALE[]   = {  1,  1,  1 }; // increase to get bumps to appear more - 
const uint8_t TOTAL_NUM_LEDS       = STRIP_NUM_LEDS[0] + STRIP_NUM_LEDS[1] + STRIP_NUM_LEDS[2];
const uint8_t STRIP_NUM_LEDS_01    = STRIP_NUM_LEDS[0] + STRIP_NUM_LEDS[1];

#define MAX_LEDS_IN_STRIP 12

CRGB leds[NUM_LED_STRIPS][MAX_LEDS_IN_STRIP];

// ten seconds per color palette makes a good demo, 20-120 is better for deployment
#define SECONDS_PER_PALETTE 120
#define SECONDS_PER_PATTERN 600

bool gRotatePalette = true;  // can toggle by IRRemote (Select)
bool gRotatePattern = true;  // can toggle by IRRemote (Select)
bool gFireworksActive = false;  // set true inside prodedure, so main loop won't send initiate new procedure until this fireworks is done
                                // causes IR remote to never get read, as after 1-2 seconds, we have 20-30 fireworks queued up

uint8_t gTmpIdx        = 0; // for gTtmpIdx numStrip loops - ensure in code doesn't have conflicting usage (and don't keep adding/remove n from heap)
uint8_t gPatternNumber = random8(0,3); // 0=A=colorwaves, 1=B=twinkle, 2=C=fireworks, 3=O=spiralwaves
uint8_t gHue           = random8(255); // used in spiralwaves


//*********************************************************************************
// Spectrum Shield and Music processing globals
// Declare Spectrum Shield pin connections
#define STROBE 4
#define RESET 6
#define DC_ONE A0
#define DC_TWO A1
#define NUM_BANDS 7 

//Define spectrum variables
bool    gBump[NUM_LED_STRIPS] = {};      // Used to pass if there was a "bump" in volume
uint8_t gBumpCnt[NUM_LED_STRIPS] = {};   // Holds how many bumps were triggered since last cleanup
uint8_t gBumpTkn[NUM_LED_STRIPS] = {};  // Holds how many bumps were taken since last cleanup (should be > bumpCnt) - 
uint8_t gLastBump[NUM_LED_STRIPS] = {};   // level of bump

#define NOISE_VALUE 64     // with no input, the MSGEQ will return some numbers - usually in the 48-72 range, so take out some component of noise
uint8_t gBumpVolBase = 3;  // multiplied by BUMP_THRESH and VOL_THRESH as input limits
#define BUMP_THRESH 6      // decrease to make fewer bump flashes, increase for less, see serial log for frequency info to tune
#define VOL_THRESH  4      // for volume calculations, ignore values less than this (skip them)
uint8_t gMusicOn = 0;      // use for choosing random vs. bump for stuff
#define MUSIC_ON_THRESH 32 // how many samples of music do we need before we go into bump mode - + on over VOL_THRESH, - on under

//*********************************************************************************
// IR Remote globals
int RECV_PIN = 12;
IRrecv gIRRecv(RECV_PIN);
decode_results gIRResults;
#define POWER 0x10EFD827 
#define A 0x10EFF807 
#define B 0x10EF7887
#define C 0x10EF58A7
#define UP 0x10EFA05F
#define DOWN 0x10EF00FF
#define LEFT 0x10EF10EF
#define RIGHT 0x10EF807F
#define SELECT 0x10EF20DF

// commented out PhotoCell code to save memory while working on Spiral code
/* //*********************************************************************************
const int LIGHT_PIN = A0; // Pin connected to voltage divider output
// Measure the voltage at 5V and the actual resistance of your
// 47k resistor, and enter them below:
const float VCC = 5.00;     // Measured voltage of Ardunio 5V line
const float R_DIV = 4700.0; // Measured resistance of 4.7k resistor
// Set this to the minimum resistance require to turn an LED on:
const float DARK_THRESHOLD = 10000.0;
// global boolean for dark enough to show
bool ambientDark = false;   // true will enable show, false will disable 
#define SECONDS_PER_PHOTOCELL  5 // how often do we check the photocell */

//*********************************************************************************
// Global Variables for Twinkle (from Adafruit NeoPixel) and FireWorks (modified Twinkle)
#define CWAVE_SPARKLE_1_IN   30 // tune based on your prefs
#define TWINKLE_1_IN         30 // tune based on your prefs, 30 is better for Fiber-Optics, but 2x speed on loop
#define FIREWORKS_1_IN        8 // tune based on your prefs
#define SPARKLE_AT           32 // tune based on your prefs
#define BASE_LOOP_MS         10 // tune based on your prefs
bool gSparkleList[NUM_LED_STRIPS][MAX_LEDS_IN_STRIP] = {};
//uint8_t gHue[] = { 0, 0, 0}; // rotating "base color" used by many of the patterns

void setup() {
  if (DEBUG_MODE)   Serial.begin(57600);  // open the serial port at 57600 bps
  gIRRecv.enableIRIn(); // Start the receiver
  
  delay(10);
  randomSeed(analogRead(0));
  // setup the built-in LED for toggling on IR code received
  pinMode(LED_BUILTIN, OUTPUT);
  // setup the photocell (LTR) for measuring ambient light
//  pinMode(LIGHT_PIN, INPUT);

  //Set Spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_ONE, INPUT);
  pinMode(DC_TWO, INPUT);  
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  //Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);   delay(1);
  digitalWrite(RESET, HIGH);   delay(1);
  digitalWrite(STROBE, HIGH);  delay(1);
  digitalWrite(STROBE, LOW);   delay(1);
  digitalWrite(RESET, LOW);    delay(1);
  // set defaults and max values for global volume vars
  memset (gBump,        false, NUM_LED_STRIPS);
  memset (gBumpCnt,         0, NUM_LED_STRIPS);
  memset (gBumpTkn,         0, NUM_LED_STRIPS);
  memset (gLastBump,        0, NUM_LED_STRIPS);
  memset (gSparkleList, false, NUM_LED_STRIPS);
  // tell FastLED about the LED strip configuration - can't use variables in template (<>) section - could wrapper it (google PixelUtil)
  // but haven't figured out how to pass the *leds array thru a template... brute force for now
  gTmpIdx = 0; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN0,COLOR_ORDER>(leds[gTmpIdx], STRIP_NUM_LEDS[gTmpIdx]).setCorrection(TypicalPixelString);
  gTmpIdx = 1; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN1,COLOR_ORDER>(leds[gTmpIdx], STRIP_NUM_LEDS[gTmpIdx]).setCorrection(TypicalPixelString);
  gTmpIdx = 2; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN2,COLOR_ORDER>(leds[gTmpIdx], STRIP_NUM_LEDS[gTmpIdx]).setCorrection(TypicalPixelString);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(BRIGHTNESS < 255);
  // Initialize our coordinates to some random values for noisepalette

  // 3 second delay for recovery
  delay(3000);

  // Assign the starting palette to RGB to test connections
/*  for (gTmpIdx = 0; gTmpIdx<NUM_LED_STRIPS; gTmpIdx++) {
    uint8_t x = mod8(gTmpIdx,3); // want to map Red, Green and Blue out to strips - fast check on hook-up and LED Style on start-up
    switch (x) {
      case 0:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Blue);
        break;
      case 1:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Green);
        break;
      case 2:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Red);
        break;
      default:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Yellow); // any Cyan or Yellow strips are those with bad hookups, or not 0..2
        break;
    }
  }
  FastLED.show();
  delay(3000); */
  gMusicOn = 0;
}

/* Forward declarations of an array of cpt-city gradient palettes, and 
   a count of how many there are.  The actual color palette definitions
   are at the bottom of this file. */
extern const TProgmemRGBGradientPalettePtr gGradientPalettes_0[];
extern const TProgmemRGBGradientPalettePtr gGradientPalettes_1[];
extern const TProgmemRGBGradientPalettePtr gGradientPalettes_2[];

extern const uint8_t gGradientPaletteCount;

const TProgmemRGBGradientPalettePtr* gGradientPalettes[] = { gGradientPalettes_0, gGradientPalettes_1, gGradientPalettes_2 };

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette[] = { {CRGB::Black}, {CRGB::Black}, {CRGB::Black} };
CRGBPalette16 gTargetPalette[]  = { {gGradientPalettes[0][0]}, {gGradientPalettes[1][0]}, {gGradientPalettes[2][0]} };

bool updPalettes = true; // use for setting palette away from RGB / Black on the 1st pass, and then updating on userInput

void loop() {
  if (updPalettes == true) {
    for (gTmpIdx = 0; gTmpIdx<NUM_LED_STRIPS; gTmpIdx++) {
      gTargetPalette[gTmpIdx]  = gGradientPalettes[gTmpIdx][gCurrentPaletteNumber];    
      palettetest(leds[gTmpIdx], gTmpIdx);
    }
    updPalettes = false;
    // 2s delay so you can see thew new palette
    FastLED.show();
    FastLED.delay(2000); // match this to the fastest values above for palette updates
  }

  while (!gIRRecv.isIdle());  // if not idle, wait till complete

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());

  if (gIRRecv.decode(&gIRResults)) {
    if (gIRResults.value == POWER) {
      gPatternNumber = 3; 
      if (DEBUG_MODE) Serial.println(F("Power - moving to spiralwaves"));
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == A) {
      if (DEBUG_MODE) Serial.println(F("A - moving to colorwaves"));
      gPatternNumber = 0;
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == B) {
      gPatternNumber = 1; 
      if (DEBUG_MODE) Serial.println(F("B - moving to twinkle"));  
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == C) {
      gPatternNumber = 2; 
      if (DEBUG_MODE) Serial.println(F("C - moving to fireworks"));
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == UP) {
      gBumpVolBase = qsub8(gBumpVolBase, 1);
      if (DEBUG_MODE) Serial.print(F("UP - More Bumps - bumps threshold now at "));
      if (DEBUG_MODE) Serial.println(gBumpVolBase * BUMP_THRESH);
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == DOWN) {
      gBumpVolBase = qadd8(gBumpVolBase, 1);
      if (DEBUG_MODE) Serial.print(F("DOWN - Less Bumps - bumps threshold now at "));
      if (DEBUG_MODE) Serial.println(gBumpVolBase * BUMP_THRESH);
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == LEFT) {
      if (DEBUG_MODE) Serial.println(F("LEFT - Previous Palette"));
      gCurrentPaletteNumber = qsub8(gCurrentPaletteNumber, 1); // go back to previous palette
      updPalettes = true;
      if (DEBUG_MODE) printPaletteNames((char*) "Mov:", gCurrentPaletteNumber);
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == RIGHT) {
      if (DEBUG_MODE) Serial.println(F("RIGHT - Next Palette"));
      gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber,1, gGradientPaletteCount-1); // go forward to next palette
      updPalettes = true;
      if (DEBUG_MODE) printPaletteNames((char*) "Mov:", gCurrentPaletteNumber);
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    if (gIRResults.value == SELECT) {
      if (DEBUG_MODE) Serial.println(F("SELECT - toggling gRotatePalette / gRotatePattern"));    
      if (gRotatePalette) {
        gRotatePalette = false;
      } else {
        gRotatePalette = true;        
      }
      if (gRotatePattern) {
        gRotatePattern = false;
      } else {
        gRotatePattern = true;        
      }
      blinkIt(); // flash the built-in LED to show we received an IR cmd
    }
    gIRRecv.resume();
  }

  
//  EVERY_N_SECONDS( SECONDS_PER_PHOTOCELL ) {
//    ReadPhotoCell();
//  }
  
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    // only rotate palette's if we haven't said no, and if the music is paused / between songs
    if (gRotatePalette == true && gMusicOn == 0) {
      gCurrentPaletteNumber    = random8( gGradientPaletteCount);  
      for (gTmpIdx = 0; gTmpIdx<NUM_LED_STRIPS; gTmpIdx++) {
        gTargetPalette[gTmpIdx] = gGradientPalettes[gTmpIdx][gCurrentPaletteNumber];
      }      
    }
  }

  EVERY_N_SECONDS( SECONDS_PER_PATTERN ) {
    if (gPatternNumber == 3 || gPatternNumber == 2) {
      // rotate off of fireworks or spiralwave always - otherwise they get stuck there (IR won't budge out either)
      gPatternNumber = random8(3);
    } else {
      if (gRotatePattern == true && gMusicOn == 0) {
        // only rotate patterns if we haven't said no, and if the music is paused / between songs
        gPatternNumber = random8(3);
      }
    }
  }

  EVERY_N_MILLIS( 1 * BASE_LOOP_MS ) { // sample more often then we need, to catch any bumps just before we display something
    ReadAndProcessBands();
  }
  
  EVERY_N_MILLIS( 2 * BASE_LOOP_MS ) {
    for (gTmpIdx = 0; gTmpIdx<NUM_LED_STRIPS; gTmpIdx++) {
//      gHue[gTmpIdx] = gHue[gTmpIdx]+1; // slowly cycle the "base color" through the rainbow for confetti
      nblendPaletteTowardPalette( gCurrentPalette[gTmpIdx], gTargetPalette[gTmpIdx], 16);
    }
  }


  if (gPatternNumber == 2) {
    // don't do for every strip, as it doesn't allow other things to change
    if (gFireworksActive == false) {
      fireworks(); // operates on all strips at same time
    }
  } else {
    for (gTmpIdx = 0; gTmpIdx < NUM_LED_STRIPS; gTmpIdx++) {
      if (gPatternNumber == 3) {
        spiralwaves(); // operates on all strips at same time
      } else if (gPatternNumber == 1) {
        twinkle(leds[gTmpIdx], gTmpIdx);
      } else {
        colorwaves(leds[gTmpIdx], gTmpIdx);
      }    
    }    
  }

  while (!gIRRecv.isIdle());  // if not idle, wait till complete

  FastLED.show();
  FastLED.delay(30); // match this to the fastest values above for palette updates
}

/*******************Read PhotoCell to detect ambient light********************/
/*void ReadPhotoCell(){
  // Read the ADC, and calculate voltage and resistance from it
  int lightADC = analogRead(LIGHT_PIN);
  if (lightADC > 0) {
    // Use the ADC reading to calculate voltage and resistance
    float lightV = lightADC * VCC / 1023.0;
    float lightR = R_DIV * (VCC / lightV - 1.0);
    if (DEBUG_MODE) Serial.println("Voltage: " + String(lightV) + " V");
    if (DEBUG_MODE) Serial.println("Resistance: " + String(lightR) + " ohms");

    // If resistance of photocell is greater than the dark threshold setting, turn the LED on.
    if (lightR >= DARK_THRESHOLD) {
      ambientDark = true;
      if (DEBUG_MODE) Serial.println("Ambient: *DARK*");
    } else {
      ambientDark = false;
      if (DEBUG_MODE) Serial.println("Ambient: Light");
    }
  }
}*/

/*******************Pull frequencies from Spectrum Shield********************/
void ReadAndProcessBands(){
  int freqL;
  int freqR; 
  uint8_t i, b;
  static uint16_t countSteps[] = {0,0,0};
  uint8_t tmpBump;

  // moved from globals to free up some memory for local allocation / re-use
  static uint8_t volume[NUM_LED_STRIPS]  = { 0,  0,  0}; //Holds the volume level read from the sound detector.
  static uint8_t lastVol[NUM_LED_STRIPS] = { 0,  0,  0}; //Holds the value of volume from the previous loop() pass.
  static uint8_t avgVol[NUM_LED_STRIPS]  = { 0,  0,  0}; //Holds the "average" volume-level to proportionally adjust the visual experience.
  
  //Read frequencies for each band, for both Left and Ride (stereo) music data
  // MSGEQ order is 63Hz, 160Hz, 400Hz, 1kHz, 2.5kHz, 6.25kHz and 16kHz, mapping to freq0..6
  for (i = 0; i<7; i++) {
    freqL = analogRead(DC_ONE);
    freqR = analogRead(DC_TWO);
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);

    if (freqL > freqR) {
      volume[i] = qsub8((uint8_t) freqL, NOISE_VALUE);
    } else {
      volume[i] = qsub8((uint8_t) freqR, NOISE_VALUE);
    }
  }
  
  uint8_t volThresh  = qmul8(gBumpVolBase,VOL_THRESH);
  uint8_t bumpThresh = qmul8(gBumpVolBase,BUMP_THRESH);

  bool musicOnThisPass = false;
  for (i = 0; i < NUM_LED_STRIPS; i++) {
    countSteps[i] = countSteps[i] + 1; // uint16_t - no qadd16
    b = STRIP_BAND_MAP[i]; 
    if (volume[b] > volThresh) {  //    /*Sets a min threshold for volume.
      musicOnThisPass = true;
      // For each band we'll use, go find the volume, avgVol, maxVol, etc. info
      avgVol[i] = scale8(qadd8(qmul8(3,avgVol[i]),volume[b]),64); // weighted average
          
      //if there is a notable change in volume, trigger a "bump"
      tmpBump  = qsub8(volume[b],lastVol[i]); // qsub8 won't go below zero, so no need to check for underflow
      if (tmpBump > bumpThresh) {
        gBump[i] = true;
        if (DEBUG_MODE) gBumpCnt[i] = qadd8(gBumpCnt[i],1);
        gLastBump[i] = tmpBump;
      } else {
        gBump[i] = false;
        gLastBump[i] = 0;
      }
    }

    lastVol[i] = volume[b]; //Records current volume for next pass

    if (musicOnThisPass) {
      gMusicOn = qadd8(gMusicOn, 1);      
    } else {
      gMusicOn = qsub8(gMusicOn, 1);      
    }

    if (countSteps[i] % 200 == 0) { // 250 steps @ 20mS should be 5S, but seems like that's ~6.5s, so do 6.5/5 of it
      if (DEBUG_MODE) printSoundStats(i, "200Steps", gMusicOn, volume[b], gLastBump[i], gBumpCnt[i], gBumpTkn[i]);      
      gBumpCnt[i] = 0; // reset every time we print
      gBumpTkn[i] = 0; // reset every time we print
      if (countSteps[i] > 20000) {
        countSteps[i] = 0;
        avgVol[i] = 64;
        countSteps[i] = 0;
      }
    }
  }
}

/* This function draws color waves with an ever-changing, widely-varying set of parameters, using a color palette. */
void colorwaves( CRGB* leds, uint8_t lIdx) {
  uint8_t numleds = STRIP_NUM_LEDS[lIdx];
  uint8_t tmpTaken = 0;
  CRGBPalette16 palette = gCurrentPalette[lIdx];
  static uint16_t sPseudotime[NUM_LED_STRIPS] = { 0, 0, 0};
  static uint16_t sLastMillis[NUM_LED_STRIPS] = { 0, 0, 0};
  static uint16_t sHue16[NUM_LED_STRIPS] = { 0, 0, 0};
 
//  uint8_t sat8 = beatsin88( 87, 220, 250); // not used anywhere
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
//  uint8_t msmultiplier = beatsin88(147, 23, 60);
  uint8_t msmultiplier = beatsin88(32, 7, 30); // trying to slow down the waves - jabenoit

  uint16_t hue16 = sHue16[lIdx];//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis[lIdx] ;
  sLastMillis[lIdx]  = ms;
  sPseudotime[lIdx] += deltams * msmultiplier;
  sHue16[lIdx] += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime[lIdx];

  for( uint8_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    uint8_t bumpVal;
    pixelnumber = (numleds-1) - pixelnumber; 
    nblend( leds[pixelnumber], newcolor, 128);
    // sparkle at random or from bump in music
    if ((gMusicOn == 0 && random8(CWAVE_SPARKLE_1_IN) == 1) || ((gMusicOn >= MUSIC_ON_THRESH) && (gBump[lIdx] == true))) {  // if no music, random, otherwise on bump
      if (random8(numleds) > scale8(qmul8(numleds,3),64)) { // only bump for a random 25% of pixels
        if (gMusicOn) {
          bumpVal = qmul8(gLastBump[lIdx], STRIP_BUMP_SCALE[lIdx]);
        } else {
          bumpVal = qmul8(random8(32, 64), STRIP_BUMP_SCALE[lIdx]);
        }
        leds[pixelnumber] += CRGB( bumpVal, bumpVal, bumpVal);          
        tmpTaken = qadd8(tmpTaken,1);        
      }
    }
  }
  gBumpTkn[lIdx] = qadd8(gBumpTkn[lIdx],tmpTaken);
  gBump[lIdx] = 0;
}

void twinkle( CRGB* leds, uint8_t lIdx) {
  CRGBPalette16 palette = gCurrentPalette[lIdx];

  for(uint8_t l = 0; l < STRIP_NUM_LEDS[lIdx]; l++) {
    if (leds[l]) { // lit to some degree
      if (gSparkleList[lIdx][l]) {
        leds[l].fadeToBlackBy(16);        
      } else if ((qadd8((qadd8(leds[l].red, leds[l].green)), leds[l].blue)) < SPARKLE_AT ) { // faded enough for a sparkle
        // have a 40% chance at a sparkle
        if (random8(5) <= 1) {
           leds[l] += CRGB(32,32,32);
           gSparkleList[lIdx][l] = true;
        }
      } else {
        leds[l].fadeToBlackBy(8);
      }
    } else {
      gSparkleList[lIdx][l] = false;
      // twinkle at random or from bump in music
      if ((gMusicOn == 0 && random8(FIREWORKS_1_IN) == 1) || ((gMusicOn >= MUSIC_ON_THRESH) && (gBump[lIdx] == true))) {  // if no music, random, otherwise on bump
        uint8_t i = random8(STRIP_NUM_LEDS[lIdx]);
        leds[i] = ColorFromPalette(palette, random8(192,255)); // assumes the brighter colors are on the higher end of the palette
        if (gBump[lIdx] == true) gBumpTkn[lIdx] = qadd8(gBumpTkn[lIdx],1);    
      } else {
        leds[l] = CRGB(0, 0, 0);
      }
    }
  }
}

/* Alternate rendering function just scrolls the current palette across the defined LED strip. */
void palettetest( CRGB* leds, uint8_t lIdx) {
  uint8_t numleds = STRIP_NUM_LEDS[lIdx];
  CRGBPalette16 palette = gCurrentPalette[lIdx];
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( leds, numleds, startindex, (256 / STRIP_NUM_LEDS[lIdx]) + 1, palette, 255, LINEARBLEND);
}

void fireworks( ) {
  uint8_t l, lTmp, str, m;
  uint8_t active, tIdx;
  const uint8_t targetFloor = scale8(TOTAL_NUM_LEDS, 193);
  CRGB newColor;
  uint8_t tmpNum;

  if (gFireworksActive == true) {
    // already called this function w/o return - exit
      if (DEBUG_MODE) { Serial.println(F("Fireworks repeat call - skipping")); }
    exit;
  } else {
      if (DEBUG_MODE) { Serial.println(F("Fireworks UNIQUE call - starting")); }
  }
  gFireworksActive = true; // tell global loop we're doing fireworks, so don't send another yet
  // my palette's are sometimes too dull for a nice fireworks burst, so use one of the predefined ones
  CRGBPalette16 palette = RainbowColors_p;
  
  // set value for all LEDs to Black (off)
  for(str = 0; str < NUM_LED_STRIPS; str++) { fill_solid( leds[str], STRIP_NUM_LEDS[str], CRGB::Black); }
  
  uint8_t ledTrgt    = random8(targetFloor, TOTAL_NUM_LEDS); // pick a fireworks target in top 25%
  uint8_t burstFloor = qsub8(ledTrgt, random8(5, TOTAL_NUM_LEDS)); // how many LEDs to light up on a fireworks burst

  // fireworks shell spiralling up with a small tail (fadeToBlack)
  for(active = 0; active <= ledTrgt; active++) {
    for(tIdx = 0; tIdx <= ledTrgt; tIdx++) {
      if (tIdx < STRIP_NUM_LEDS[0]) {
        str  = 0;
        lTmp = tIdx;
      } else if (tIdx < STRIP_NUM_LEDS_01) {
        str  = 1;
        lTmp = qsub8(tIdx,STRIP_NUM_LEDS[0]);
      } else {
        str  = 2;
        lTmp = qsub8(tIdx, STRIP_NUM_LEDS_01);
      }
      l = mod8(qadd8(STRIP_SPIRAL_START[str],lTmp),STRIP_NUM_LEDS[str]); // use modulo so that we only address LEDs we have - will wrap if START > 0
      if (tIdx == active) {
        leds[str][l] = CRGB::White;
      } else {
        leds[str][l].fadeToBlackBy(95);
      }
      FastLED.delay(1); // small delay - any FastLed.delay w/ delay > 0 should include a show
    }
  }
  // set everything to black 
  for(str = 0; str < NUM_LED_STRIPS; str++) { fill_solid( leds[str], STRIP_NUM_LEDS[str], CRGB::Black); }
  FastLED.delay(100); // small delay - pause before final height and burst (suspense)

  // fireworks burst of color, from ledTrgt for burstDepth LEDs
  uint8_t ranBursts = random(0, 7); // want a heavier weighting to 1 burst (0,1,2,3), than 2 (4,5), and only rarely 3 (6)
  uint8_t numBursts;
  if (ranBursts <= 3) {
    numBursts = 1;
  } else if ( ranBursts <= 5) {
    numBursts = 2;  
  } else {
    numBursts = 3;
  }
  uint8_t newClrTrgt;
  for(uint8_t bNum = numBursts; bNum > 0; bNum--) {
    if (random8(0, 3) == 0) {
      // set color to pure white 25% of the time
      newColor = CRGB::White;      
      if (DEBUG_MODE) { Serial.print(F("Burst# ")); Serial.println(bNum); Serial.println(F("Burst Color WHITE")); }
    } else {
      newClrTrgt = mul8(random8(0,15),16);
      newColor = ColorFromPalette( palette, newClrTrgt);
      if (DEBUG_MODE) { Serial.print(F("Burst# ")); Serial.println(bNum); Serial.print(F("Burst Color ")); Serial.println(newClrTrgt);}
    }
    if (bNum != numBursts) {
      // pick a new target
      ledTrgt    = random8(targetFloor, TOTAL_NUM_LEDS); // pick a fireworks target in top 25%
      burstFloor = qsub8(ledTrgt, random8(5, TOTAL_NUM_LEDS)); // how many LEDs to light up on a fireworks burst
    }
    for(tIdx = TOTAL_NUM_LEDS-1; tIdx >0; tIdx--) {
      if (tIdx < STRIP_NUM_LEDS[0]) {
        str  = 0;
        lTmp = tIdx;
      } else if (tIdx < STRIP_NUM_LEDS_01) {
        str  = 1;
        lTmp = qsub8(tIdx,STRIP_NUM_LEDS[0]);
      } else {
        str  = 2;
        lTmp = qsub8(tIdx, STRIP_NUM_LEDS_01);
      }
      l = mod8(qadd8(STRIP_SPIRAL_START[str],lTmp),STRIP_NUM_LEDS[str]); // use modulo so that we only address LEDs we have - will wrap if START > 0
      gSparkleList[str][l] = false; // set all sparkles off before we get there
      if (tIdx <= ledTrgt && tIdx >= burstFloor) {
        if (leds[str][l]) {         // blend
          nblend(leds[str][l], newColor, 128);
        } else {                    // no color set yet, nothing to blend
          leds[str][l] = newColor;
        }
//        if (DEBUG_MODE) { Serial.print(F("Burst tIdx ")); Serial.println(tIdx);}
        FastLED.delay(1);
        FastLED.show();
      }
    }
    FastLED.delay(random8(31,95)); // small delay
  }
  FastLED.delay(random8(95,159)); // medium delay before we fade

  // fade and sparkle
  for(uint8_t tmpLoop = 0; tmpLoop < 36; tmpLoop++) {
//    if (DEBUG_MODE) { Serial.print(F("Fade loop ")); Serial.println(tmpLoop); }
    for(tIdx = TOTAL_NUM_LEDS-1; tIdx >0; tIdx--) {
      if (tIdx < STRIP_NUM_LEDS[0]) {
        str  = 0;
        lTmp = tIdx;
      } else if (tIdx < STRIP_NUM_LEDS_01) {
        str  = 1;
        lTmp = qsub8(tIdx,STRIP_NUM_LEDS[0]);
      } else {
        str  = 2;
        lTmp = qsub8(tIdx, STRIP_NUM_LEDS_01);
      }
      l = mod8(qadd8(STRIP_SPIRAL_START[str],lTmp),STRIP_NUM_LEDS[str]); // use modulo so that we only address LEDs we have - will wrap if START > 0
      uint8_t ledVal = qadd8(qadd8(leds[str][l].red, leds[str][l].green), leds[str][l].blue);
      if (ledVal) { // 5% chance at a sparkle
        if ((gSparkleList[str][l] == false) && (random8(20) < 1) && (ledVal < SPARKLE_AT )) { // faded enough for a sparkle
          leds[str][l] += CRGB(32,32,32);
          gSparkleList[str][l] = true;
        } else {
          leds[str][l].fadeToBlackBy(24);
        }
      }
    }
    FastLED.show();   // show changes
    FastLED.delay(random8(63,127)); // small delay
  }
  
  // make sure everytyhing is off
  for(str = 0; str < NUM_LED_STRIPS; str++) {
    fill_solid( leds[str], STRIP_NUM_LEDS[str], CRGB::Black);
  }
  FastLED.delay(random8(127, 255)); // another medium delay
  gFireworksActive = false; // tell global loop OK to send another
}

/* Gradient palette "GMT_seis_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_seis.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 40 bytes of program space. */
// used for fireworks
DEFINE_GRADIENT_PALETTE( GMT_seis2_gp ) {
    0,  88,  0,  0,
   28, 255,  0,  0,
   56, 255, 22,  0,
   85, 255,104,  0,
  113, 255,255,  0,
  141, 255,255,  0,
  169,  17,255,  1,
  198,   0,223, 31,
  226,   0, 19,255,
  255,   0,  0,147,
  226,   0, 19,255,
  198,   0,223, 31,
  169,  17,255,  1,
  141, 255,255,  0,
  113, 255,255,  0,
   85, 255,104,  0,
   56, 255, 22,  0,
   28, 255,  0,  0,
    0,  88,  0,  0,
    };

void spiralwaves( ) {
  uint8_t l, lTmp, str, m;
  uint8_t active, tIdx, lHue;
  CRGBPalette16 palette = GMT_seis2_gp;

  // make sure everytyhing is off
//  for(str = 0; str < NUM_LED_STRIPS; str++) { fill_solid( leds[str], STRIP_NUM_LEDS[str], CRGB::Black); }

  // spiralling up with a small tail (fadeToBlack)
  for(active = 0; active <= TOTAL_NUM_LEDS; active++) {
    lHue = gHue;
    for(tIdx = 0; tIdx <= TOTAL_NUM_LEDS; tIdx++) {
      if (tIdx < STRIP_NUM_LEDS[0]) {
        str  = 0;
        lTmp = tIdx;
      } else if (tIdx < STRIP_NUM_LEDS_01) {
        str  = 1;
        lTmp = qsub8(tIdx,STRIP_NUM_LEDS[0]);
      } else {
        str  = 2;
        lTmp = qsub8(tIdx, STRIP_NUM_LEDS_01);
      }
      l = mod8(qadd8(STRIP_SPIRAL_START[str],lTmp),STRIP_NUM_LEDS[str]); // use modulo so that we only address LEDs we have - will wrap if START > 0
      if (tIdx == active) {
        leds[str][l] = ColorFromPalette( palette, gHue);
//      } else {
//        leds[str][l].fadeToBlackBy(1);
      }
      FastLED.show();
//      FastLED.delay(1); // tiny delay, as it's in a loop
      lHue += 4; // increment lHue every time thru the loop
    }
  }  
  gHue += 4; // update global hue (shift starting color by 1) only on big loop
}

/* Blink the built-in Arduino LED ("L", next to Tx/Rx typically) to show we received an IR code */
void blinkIt () {
  for( int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    FastLED.delay(100);               // wait a bit
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW 
    FastLED.delay(100);               // wait a bit
  } 
}

/*********************************************************************************/
/* PROGMEM string stuff for palette names - for debug / finding which palettes you like
   How to store table of strings from:
   http://www.nongnu.org/avr-libc/user-manual/pgmspace.html
*/
const char palette_00[] PROGMEM = "set0_calla_01_gp";
const char palette_01[] PROGMEM = "set0_es_emerald_dragon_01_gp";
const char palette_02[] PROGMEM = "set0_blue_purple_gp";
const char palette_03[] PROGMEM = "set0_es_seadreams_08_gp";
const char palette_04[] PROGMEM = "set0_rgi_03_gp";
const char palette_05[] PROGMEM = "set0_rgi_01_gp";
const char palette_06[] PROGMEM = "set0_GMT_hot_gp";
const char palette_07[] PROGMEM = "set0_cw1_029_gp";
const char palette_08[] PROGMEM = "set0_cw1_029g_gp";
const char palette_09[] PROGMEM = "set0_bhw3_61_gp";
const char palette_10[] PROGMEM = "set0_es_landscape_21_gp";
const char palette_11[] PROGMEM = "set0_es_ocean_breeze_002_gp";
const char palette_12[] PROGMEM = "set0_bhw3_21_gp";
const char palette_13[] PROGMEM = "set0_63undercom_gp";
const char palette_14[] PROGMEM = "set0_Analogous_12_gp";
const char palette_15[] PROGMEM = "set0_realpeach_gp";
const char palette_16[] PROGMEM = "set0_passionfruit_gp";
const char palette_17[] PROGMEM = "set0_GMT_seis_gp";
const char palette_18[] PROGMEM = "set0_rainbow_gp";

// Then set up a table to refer to your strings.
const char * const paletteNameTable[] PROGMEM = {   
  palette_00,
  palette_01,
  palette_02,
  palette_03,
  palette_04,
  palette_05,
  palette_06,
  palette_07,
  palette_08,
  palette_09,
  palette_10,
  palette_11,
  palette_12,
  palette_13,
  palette_14,
  palette_15,
  palette_16,
  palette_17,
  palette_18,
  };

/* For debug - print out palette we're using */
void printPaletteNames(const char prfx[4], uint8_t paletteID) {
  char buffer[32];    // make sure this is large enough for the largest string it must hold

  // get the palette name from PROGMEM table
  if (DEBUG_MODE) strcpy_P(buffer, (char*)pgm_read_word(&(paletteNameTable[paletteID]))); // Necessary casts and dereferencing, just copy. 
  if (DEBUG_MODE) Serial.print(prfx);
  if (DEBUG_MODE) Serial.print(F(" "));
  if (DEBUG_MODE) Serial.print(paletteID);
  if (DEBUG_MODE) Serial.print(F(" "));
  if (DEBUG_MODE) Serial.println(buffer);
}

/* For debug - print out music info (why no bumps or two many bumps) */
void printSoundStats ( uint8_t i, String msg, uint8_t tgmusicon, uint8_t tvol, uint8_t tlastbump, uint8_t tbumpcnt, uint8_t tbumptkn  ) {
  uint16_t ms = millis();

  if (DEBUG_MODE) Serial.print(msg);
  if (DEBUG_MODE) Serial.print(" - ms ");
  if (DEBUG_MODE) Serial.print(ms);
  if (DEBUG_MODE) Serial.print(" - music ");
  if (DEBUG_MODE) Serial.print(tgmusicon);
  if (DEBUG_MODE) Serial.print(" - str ");
  if (DEBUG_MODE) Serial.print(i);
  if (DEBUG_MODE) Serial.print(" - vol/bmp Thresh ");
  if (DEBUG_MODE) Serial.print(gBumpVolBase * VOL_THRESH);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.print(gBumpVolBase * BUMP_THRESH);
  if (DEBUG_MODE) Serial.print(" - vol/bmp ");
  if (DEBUG_MODE) Serial.print(tvol);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.print(tlastbump);
  if (DEBUG_MODE) Serial.print(" - #bmp/#tkn ");
  if (DEBUG_MODE) Serial.print(tbumpcnt);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.println(tbumptkn);
}

/* For debug - print out music info (why no bumps or two many bumps) */
void printNoiseStats ( uint8_t i, String msg, uint16_t tgmusicon, uint16_t tvol, uint16_t tlastbump, uint16_t tbumpcnt, uint16_t tbumptkn  ) {
  uint16_t ms = millis();

  if (DEBUG_MODE) Serial.print(msg);
  if (DEBUG_MODE) Serial.print(" - ms ");
  if (DEBUG_MODE) Serial.print(ms);
  if (DEBUG_MODE) Serial.print(" - music ");
  if (DEBUG_MODE) Serial.print(tgmusicon);
  if (DEBUG_MODE) Serial.print(" - bnd ");
  if (DEBUG_MODE) Serial.print(i);
//  if (DEBUG_MODE) Serial.print(" - vol/bmp Thresh ");
//  if (DEBUG_MODE) Serial.print(gBumpVolBase * VOL_THRESH);
//  if (DEBUG_MODE) Serial.print("/");
//  if (DEBUG_MODE) Serial.print(gBumpVolBase * BUMP_THRESH);
  if (DEBUG_MODE) Serial.print(" - vol/bmp ");
  if (DEBUG_MODE) Serial.print(tvol);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.print(tlastbump);
  if (DEBUG_MODE) Serial.print(" - #bmp/#tkn ");
  if (DEBUG_MODE) Serial.print(tbumpcnt);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.println(tbumptkn);
}



//*********************************************************************************
/* Gradient Color Palette definitions for ~20 different cpt-city (or other) color palettes.
   In theory, this could all move to the bottom of the file with some forward declares,
   but I'm not conversant enough in Arduino/C to adapt the code to an array of arrays */

/* FastLED Palette specification, from:
   https://github.com/FastLED/FastLED/wiki/Gradient-color-palettes

  DEFINE_GRADIENT_PALETTE( heatmap_gp ) {
        0,     0,  0,  0,   //black
      127,   255,  0,  0,   //red
      224,   255,255,  0,   //bright yellow
      255,   255,255,255 }; //full white

    The first column specifies where along the palette's indicies (0-255), 
    the gradient should be anchored, as shown in the table above. 
    In this case, the first gradient runs from black to red, in index positions 0-128. 
    The next gradient from red to bright yellow runs in index positions 128-224, 
    and the final gradient from bright yellow to full white runs in index positions 224-255. 
    Note that you can specify unequal 'widths' of the gradient segments to stretch or squeeze 
    parts of the palette as desired. */

/* Based on Gradient palette "bhw1_07_gp", originally from
 http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_07.png.index.html
 converted for FastLED with gammas (2.6, 2.2, 2.5)
 Size: 8 bytes of program space. */

DEFINE_GRADIENT_PALETTE( set2_bhw1_07_gp ) {
    0, 232, 65,  1,
  255, 229,227,  1};
DEFINE_GRADIENT_PALETTE( set1_bhw1_07_gp ) {
    0, 232, 65,  1,
  255, 229,227,  1};
DEFINE_GRADIENT_PALETTE( set0_bhw1_07_gp ) {
    0, 232, 65,  1,
  255, 229,227,  1};

/* Based on Gradient palette "bhw3_61_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw3/tn/bhw3_61.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 24 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_bhw3_61_gp ) {
   0,  14,  1, 27,
 127,  17,  1, 88,
 255,   1, 88,156};
DEFINE_GRADIENT_PALETTE( set1_bhw3_61_gp ) {
   0,   1, 88,156,
 127,   1, 54, 42,
 255,   9,235, 52};
DEFINE_GRADIENT_PALETTE( set0_bhw3_61_gp ) {
   0,   1, 54, 42,
 127,   9,235, 52,
 255, 139,235,233};

/* Based on Gradient palette "bhw3_21_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw3/tn/bhw3_21.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 36 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_bhw3_21_gp ) {
    0,   1, 40, 98,
   85,   1, 65, 68,
  170,   2,161, 96,
  255,   0, 81, 25};
DEFINE_GRADIENT_PALETTE( set1_bhw3_21_gp ) {
    0,   0, 81, 25,
   85,  65,182, 82,
  170,   0, 86,170,
  255,  17,207,182};
DEFINE_GRADIENT_PALETTE( set0_bhw3_21_gp ) {
    0,   0, 86,170,
   85,  17,207,182,
  170,  17,207,182,
  255,   1, 23, 46};

/* Based on Gradient palette "es_ocean_breeze_002_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_002.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 12 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set0_es_ocean_breeze_002_gp ) {
  0, 139, 152, 142,
 32, 112, 141, 152,
 63,  84, 130, 161,
 95,  57, 119, 171};
DEFINE_GRADIENT_PALETTE( set1_es_ocean_breeze_002_gp ) {
 63,  84, 130, 161,
 95,  57, 119, 171,
127,  29, 108, 180,
159,  22,  82, 136};
DEFINE_GRADIENT_PALETTE( set2_es_ocean_breeze_002_gp ) {
159,  22,  82, 136,
191,  15,  56,  93,
223,   8,  30,  49,
255,   1,   4,   5};

/* Based on Gradient palette "es_seadreams_08_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/sea_dreams/tn/es_seadreams_08.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 20 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_es_seadreams_08_gp ) {
    0,   0,  0,255,
  127,  31, 45,233,
  255, 213,169,247};
DEFINE_GRADIENT_PALETTE( set1_es_seadreams_08_gp ) {
    0,  31, 45,233,
  127, 213,169,247,
  255,   4, 81,219};
DEFINE_GRADIENT_PALETTE( set0_es_seadreams_08_gp ) {
    0, 213,169,247,
  127,   4, 81,219,
  255,   4, 81,219};

/* Based on Gradient palette "es_landscape_21_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_21.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 20 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_es_landscape_21_gp ) {
    0,   3, 63,  5,
  127,  31, 95, 15,
  255,  63,127, 63};
DEFINE_GRADIENT_PALETTE( set1_es_landscape_21_gp ) {
    0,  63,127, 63,
  127,   6, 80,158,
  255,   0, 24,119};
DEFINE_GRADIENT_PALETTE( set0_es_landscape_21_gp ) {
    0,  63,127, 63,
  127, 153,152,120,
  255,   6, 80,158};

/* Based on Gradient palette "63undercom_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/colo/evad/tn/63undercom.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 40 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_63undercom_gp ) {
    0,  31, 12,  0,
  127,  95, 27,  1,
  255, 159, 72,  1};
DEFINE_GRADIENT_PALETTE( set1_63undercom_gp ) {
    0, 159, 72,  1,
  127, 177, 82,  1,
  255, 195, 93,  1};
DEFINE_GRADIENT_PALETTE( set0_63undercom_gp ) {
    0, 195, 93,  1,
  127, 255,117,  1,
  255, 252,193,  1};

/* Based on Gradient palette "cw1_029_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/cw/1/tn/cw1-029.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 12 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_cw1_029_gp ) {
  0,  12,  80, 210,
 82,  73, 119, 162,
164, 134, 158, 115,
255, 194, 196,  67};
DEFINE_GRADIENT_PALETTE( set1_cw1_029_gp ) {
  0, 194, 196,  67,
 82, 255, 235,  19,
164, 247, 193,  18,
255, 240, 151,  17};
DEFINE_GRADIENT_PALETTE( set0_cw1_029_gp ) {
  0, 247, 193,  18,
 82, 240, 151,  17,
164, 232, 108,  16,
255, 224,  66,  15};

/* Based on Gradient palette "cw1_029_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/cw/1/tn/cw1-029.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 12 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_cw1_029g_gp ) {
  0,  12, 210,  80,
 82,  73, 162, 119, 
164, 134, 115, 158,
255, 194,  67, 196};
DEFINE_GRADIENT_PALETTE( set1_cw1_029g_gp ) {
  0, 194, 196,  67,
 82, 255, 235,  19,
164, 247, 193,  18,
255, 240, 151,  17};
DEFINE_GRADIENT_PALETTE( set0_cw1_029g_gp ) {
  0, 247, 193,  18,
 82, 240, 151,  17,
164, 232, 108,  16,
255, 224,  66,  15};

/* Based on Gradient palette "GMT_hot_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_hot.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 16 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_GMT_hot_gp ) {
    0,  31,   0,   0,
  127,  95,   0,   0,
  255, 159,   0,   0};
DEFINE_GRADIENT_PALETTE( set1_GMT_hot_gp ) {
    0, 127,  31,  15,
   63, 143,  63,  15,
  127, 159, 127,  15,
  191, 175, 191,  15,
  255, 191, 255,  15};
DEFINE_GRADIENT_PALETTE( set0_GMT_hot_gp ) {
    0, 207, 255,  15,
   63, 223, 255,  63,
  127, 239, 255, 127,
  191, 255, 255, 191,
  255, 255, 255, 255};

/* Based on Gradient palette "rgi_03_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/rgi/tn/rgi_03.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 20 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set0_rgi_03_gp ) {
    0, 247, 79, 17,
  127,  80, 27, 32,
  255,  11,  3, 52};
DEFINE_GRADIENT_PALETTE( set1_rgi_03_gp ) {
    0,  80, 27, 32,
  127,  11,  3, 52,
  255,  16,  4, 45};
DEFINE_GRADIENT_PALETTE( set2_rgi_03_gp ) {
    0,  11,  3, 52,
  127,  16,  4, 45,
  255,  22,  6, 38};

/* Based on Gradient palette "blue_purple_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/face/hair/tn/blue-purple.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 36 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_blue_purple_gp ) {
    0,   1,  0, 11,
  127,   5,  5, 34,
  255,  24, 23, 74};
DEFINE_GRADIENT_PALETTE( set1_blue_purple_gp ) {
    0,  24, 23, 74,
  127,  10, 71,149,
  255,   2,149,255};
DEFINE_GRADIENT_PALETTE( set0_blue_purple_gp ) {
    0,   2,149,255,
  127,  63,199,255,
  255, 255,255,255};

/* Based on Gradient palette "rgi_01_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/rgi/tn/rgi_01.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 36 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set0_rgi_01_gp ) {
    0, 224,  4,  9,
   82, 118, 16,  6,
  163,  51, 36,  5,
  255, 126, 55, 10};
DEFINE_GRADIENT_PALETTE( set1_rgi_01_gp ) {
    0, 126, 55, 10,
   82, 247, 79, 17,
  163,  80, 27, 32,
  255,  11,  3, 52};
DEFINE_GRADIENT_PALETTE( set2_rgi_01_gp ) {
    0,  80, 27, 32,
   82,  11,  3, 52,
  163,  16,  4, 45,
  255,  22,  6, 38};

/* Based on Gradient palette "realpeach_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/flor/tn/realpeach.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 28 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_realpeach_gp ) {
    0,   4,  9,  0,
  127,   7, 16,  1,
  255,  10, 26,  1};
DEFINE_GRADIENT_PALETTE( set1_realpeach_gp ) {
    0,  10, 26,  1,
  127,  41, 37,  1,
  255, 103, 50,  0};
DEFINE_GRADIENT_PALETTE( set0_realpeach_gp ) {
    0, 103, 50,  0,
  127, 123, 17,  1,
  255, 146,  2,  1};

/* Based on Gradient palette "passionfruit_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/flor/tn/passionfruit.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 128 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_passionfruit_gp ) {
    0,   4, 23,  0,
   42,   5, 25,  0,
   85,   7, 26,  0,
  127,   8, 27,  0,
  170,  10, 29,  0,
  212,  13, 31,  0,
  255,  16, 33,  0};
DEFINE_GRADIENT_PALETTE( set1_passionfruit_gp ) {
    0,  16, 33,  0,
   16,  20, 34,  0,
   31,  24, 36,  0,
   50,  27, 36,  0,
   67,  29, 34,  0,
   84,  30, 32,  0,
  101,  32, 30,  0,
  118,  34, 28,  0,
  135,  35, 26,  0,
  152,  38, 23,  0,
  169,  39, 21,  0,
  186,  42, 19,  0,
  203,  44, 16,  0,
  220,  46, 14,  0,
  237,  48, 12,  0,
  255,  50, 10,  0};
DEFINE_GRADIENT_PALETTE( set0_passionfruit_gp ) {
    0,  50, 10,  0,
  127, 127, 88,  0,
  255, 255,255,  0};

/* Based on Gradient palette "Analogous_12_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/terra/tn/Analogous_12.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 36 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_Analogous_12_gp ) {
    0,  53,  1,  1,
  127, 127, 27,  2,
  255, 255, 55,  4};
DEFINE_GRADIENT_PALETTE( set1_Analogous_12_gp ) {
    0, 127, 27,  2,
  127, 255, 55,  4,
  255, 154, 37,  2};
DEFINE_GRADIENT_PALETTE( set0_Analogous_12_gp ) {
    0, 255, 55,  4,
  127, 154, 37,  2,
  255,  53, 22,  0};

/* Based on Gradient palette "es_emerald_dragon_01_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/emerald_dragon/tn/es_emerald_dragon_01.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 20 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_es_emerald_dragon_01_gp ) {
    0,   1, 19,  7,
  255,   1, 59, 25};
DEFINE_GRADIENT_PALETTE( set1_es_emerald_dragon_01_gp ) {
    0,   1, 59, 25,
  255,  14,127,127};
DEFINE_GRADIENT_PALETTE( set0_es_emerald_dragon_01_gp ) {
    0,  14,127,127,
  255,  28,255,255};

/* Based on Gradient palette "temperature_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/arendal/tn/temperature.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 144 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set0_rainbow_gp ) {
    0,   1, 27,105,
   17,   1, 92,197,
   34,   1,119,221,
   51,   3,130,151,
   68,  23,156,149,
   85,  67,182,112,
  102, 121,201, 52,
  119, 142,203, 11,
  136, 224,223,  1,
  153, 252,187,  2,
  170, 247,147,  1,
  187, 237, 87,  1,
  204, 229, 43,  1,
  221, 220, 15,  1,
  238, 171,  2,  2,
  255,  80,  3,  3};
DEFINE_GRADIENT_PALETTE( set1_rainbow_gp ) {
    0,   1, 27,105,
   17,   1, 92,197,
   34,   1,119,221,
   51,   3,130,151,
   68,  23,156,149,
   85,  67,182,112,
  102, 121,201, 52,
  119, 142,203, 11,
  136, 224,223,  1,
  153, 252,187,  2,
  170, 247,147,  1,
  187, 237, 87,  1,
  204, 229, 43,  1,
  221, 220, 15,  1,
  238, 171,  2,  2,
  255,  80,  3,  3};
DEFINE_GRADIENT_PALETTE( set2_rainbow_gp ) {
    0,   1, 27,105,
   17,   1, 92,197,
   34,   1,119,221,
   51,   3,130,151,
   68,  23,156,149,
   85,  67,182,112,
  102, 121,201, 52,
  119, 142,203, 11,
  136, 224,223,  1,
  153, 252,187,  2,
  170, 247,147,  1,
  187, 237, 87,  1,
  204, 229, 43,  1,
  221, 220, 15,  1,
  238, 171,  2,  2,
  255,  80,  3,  3};

const uint8_t gGradientPaletteCount = 18;

/* Three arrays color palettes, where any index in 0 has a matching palette in 1 and 2
 This will let us programmatically choose one based on
 a number, rather than having to activate each explicitly 
 by name every time.
 Since it is const, this array could also be moved 
 into PROGMEM to save SRAM, but for simplicity of illustration
 we'll keep it in a regular SRAM array.

 This list of color palettes acts as a "playlist"; you can
 add or delete, or re-arrange as you wish. */

// Section0 (top / interior)
const TProgmemRGBGradientPalettePtr gGradientPalettes_0[] = {
  set0_rainbow_gp,
  set0_es_emerald_dragon_01_gp,
  set0_blue_purple_gp,
  set0_es_seadreams_08_gp,
  set0_rgi_03_gp,
  set0_rgi_01_gp,
  set0_GMT_hot_gp,
  set0_cw1_029_gp,
  set0_cw1_029g_gp,
  set0_bhw3_61_gp,
  set0_es_landscape_21_gp,
  set0_es_ocean_breeze_002_gp,
  set0_bhw3_21_gp,
  set0_63undercom_gp,
  set0_Analogous_12_gp,
  set0_realpeach_gp,
  set0_passionfruit_gp,
  set0_bhw1_07_gp,
  };

// Section1 (middle)
const TProgmemRGBGradientPalettePtr gGradientPalettes_1[] = {
  set1_rainbow_gp,
  set1_es_emerald_dragon_01_gp,
  set1_blue_purple_gp,
  set1_es_seadreams_08_gp,
  set1_rgi_03_gp,
  set1_rgi_01_gp,
  set1_GMT_hot_gp,
  set1_cw1_029_gp,
  set1_cw1_029g_gp,
  set1_bhw3_61_gp,
  set1_es_landscape_21_gp,
  set1_es_ocean_breeze_002_gp,
  set1_bhw3_21_gp,
  set1_63undercom_gp,
  set1_Analogous_12_gp,
  set1_realpeach_gp,
  set1_passionfruit_gp,
  set1_bhw1_07_gp
  };

// Section2 (bottom / exterior)
const TProgmemRGBGradientPalettePtr gGradientPalettes_2[] = {
  set2_rainbow_gp,
  set2_es_emerald_dragon_01_gp,
  set2_blue_purple_gp,
  set2_es_seadreams_08_gp,
  set2_rgi_03_gp,
  set2_rgi_01_gp,
  set2_GMT_hot_gp,
  set2_cw1_029_gp,
  set2_cw1_029g_gp,
  set2_bhw3_61_gp,
  set2_es_landscape_21_gp,
  set2_es_ocean_breeze_002_gp,
  set2_bhw3_21_gp,
  set2_63undercom_gp,
  set2_Analogous_12_gp,
  set2_realpeach_gp,
  set2_passionfruit_gp,
  set2_bhw1_07_gp,
  };
  




