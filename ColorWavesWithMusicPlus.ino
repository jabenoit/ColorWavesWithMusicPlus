#include "FastLED.h"  // developed with FastLED ver 3.1.3 library
#include "IRremote.h" // developed with IRremote ver 2.2.3 library
#include "my_palettes.h" // contains defintions of the color palletes used by ColorWaves, add / remove / etc. there

// Mix of ColorWavesWithPalettes (to handle 3 strips uniquely)
//        SparkFun Music Visualizer Tutorial (process frequency data, find "bump"
//        SparkFun Simple IR Remote (simple control on modes, palettes, bump speed)
//        Adafruit Twinkle (use FastLED library and palettes)
//        Fireworks (written from scratch, as I'm not using a matrix display) 

/*        AFAIK, this will *not* run on Uno's and other devices w/ only 2KB memory -
             you'll need to take out some code (probably either IR remote or Music Vis
             code would be enough) - threshold is somewhere in the 800+ bytes for local
             variables. */

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
#define DEBUG_MODE 0

//*********************************************************************************
// LED/FastLED globals
#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS  255

#define NUM_LED_STRIPS   3
#define STRIP_DATA_PIN0  8
#define STRIP_DATA_PIN1  9
#define STRIP_DATA_PIN2 10
const uint8_t STRIP_NUM_LEDS[]  = { 7, 12, 24 };
const uint8_t STRIP_BAND_MAP[] = { 5,  3,  1}; // Strip 2 uses band 1 (Bass), Strip 1 uses band 3 (Tenor), Strip 0 uses band 5 (Soprano)

#define MAX_LEDS_IN_STRIP 24

CRGB leds[NUM_LED_STRIPS][MAX_LEDS_IN_STRIP];

// ten seconds per color palette makes a good demo, 20-120 is better for deployment
#define SECONDS_PER_PALETTE 20
#define SECONDS_PER_COLORMODE 1800
bool ROTATE_PALETTE = true;   // can toggle by IRRemote (Select)
bool ROTATE_COLORMODE = true; // can toggle by IRRemote (Power)

// n for numStrip in for loops - ensure in code doesn't have conflicting usage (and don't keep adding/remove n from heap)
uint8_t n; 
uint8_t colorMode = 0; // 0=A=colorwaves, 1=B=twinkle, 2=C=fireworks

//*********************************************************************************
// Spectrum Shield and Music processing globals
// Declare Spectrum Shield pin connections
#define STROBE 4
#define RESET 6
#define DC_One A0
#define DC_Two A1
#define NUM_BANDS 7 

// Define spectrum variables
bool    bump[NUM_LED_STRIPS] = {};      // Used to pass if there was a "bump" in volume
uint8_t bumpCnt[NUM_LED_STRIPS] = {};   // Holds how many bumps were triggered since last cleanup
uint8_t bumpTkn[NUM_LED_STRIPS] = {};  // Holds how many bumps were taken since last cleanup (should be > bumpCnt) - 
uint8_t lastBump[NUM_LED_STRIPS] = {};   // level of bump
uint8_t bumpVolBase = 4;  // multiplied by BUMP_THRESH and VOL_THRESH as input limits
#define BUMP_THRESH 12  // decrease to make fewer bump flashes, increase for less, see serial log for frequency info to tune
#define VOL_THRESH  19  // for volume calculations, ignore values less than this (skip them)
uint8_t musicOn = 0;    // use for choosing random vs. bump for stuff
#define MUSICON_THRESH 32 // how many samples of music do we need before we go into bump mode - + on over VOL_THRESH, - on under

//*********************************************************************************
// IR Remote globals
int RECV_PIN = 12;
IRrecv irrecv(RECV_PIN);
decode_results results;
#define POWER 0x10EFD827 
#define A 0x10EFF807 
#define B 0x10EF7887
#define C 0x10EF58A7
#define UP 0x10EFA05F
#define DOWN 0x10EF00FF
#define LEFT 0x10EF10EF
#define RIGHT 0x10EF807F
#define SELECT 0x10EF20DF

//*********************************************************************************
// Global Variables for Twinkle (from Adafruit NeoPixel) and FireWorks (modified Twinkle)
#define SPARKLE_AT 32
bool sparkleList[NUM_LED_STRIPS][MAX_LEDS_IN_STRIP] = {};
uint8_t gHue[] = { 0, 0, 0}; // rotating "base color" used by many of the patterns

// Current palette number from the 'playlist' of color palettes
uint8_t       gCurrentPaletteID;
CRGBPalette16 gCurrentPaletteAry[NUM_LED_STRIPS];
CRGBPalette16 gTargetPaletteAry[NUM_LED_STRIPS];

bool updPalettes = true; // use for setting palette away from RGB on the 1st pass, and then updating on userInput


//*********************************************************************************
void setup() {
  if (DEBUG_MODE)   Serial.begin(57600);  // open the serial port at 57600 bps
  irrecv.enableIRIn(); // Start the receiver
  
  delay(10);
  randomSeed(analogRead(0));
  // setup the built-in LED for toggling on IR code received
  pinMode(LED_BUILTIN, OUTPUT);  

  //Set Spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);
  pinMode(DC_Two, INPUT);  
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  //Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);   delay(1);
  digitalWrite(RESET, HIGH);   delay(1);
  digitalWrite(STROBE, HIGH);  delay(1);
  digitalWrite(STROBE, LOW);   delay(1);
  digitalWrite(RESET, LOW);    delay(1);
  // set defaults and max values for global volume vars
  memset (bump,        false, NUM_LED_STRIPS);
  memset (bumpCnt,         0, NUM_LED_STRIPS);
  memset (bumpTkn,         0, NUM_LED_STRIPS);
  memset (lastBump,        0, NUM_LED_STRIPS);
  memset (sparkleList, false, NUM_LED_STRIPS);
  // tell FastLED about the LED strip configuration - can't use variables in template (<>) section - could wrapper it (google PixelUtil)
  // but haven't figured out how to pass the *leds array thru a template... brute force for now
  n = 0; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN0,COLOR_ORDER>(leds[n], STRIP_NUM_LEDS[n]).setCorrection(TypicalPixelString);
  n = 1; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN1,COLOR_ORDER>(leds[n], STRIP_NUM_LEDS[n]).setCorrection(TypicalPixelString);
  n = 2; FastLED.addLeds<LED_TYPE,STRIP_DATA_PIN2,COLOR_ORDER>(leds[n], STRIP_NUM_LEDS[n]).setCorrection(TypicalPixelString);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(BRIGHTNESS < 255);
  // Initialize our coordinates to some random values for noisepalette

  // 3 second delay for recovery
  delay(3000);
  musicOn = 0;

  // Assign the starting palette
  for (n = 0; n<NUM_LED_STRIPS; n++) {
    uint8_t x = mod8(n,3); // want to map Red, Green and Blue out to strips - fast check on hook-up and LED Style on start-up
    switch (x) {
      case 0:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB(255,159,  0)); // yellow
        break;
      case 1:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB( 95, 95, 95)); // white
        break;
      case 2:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB(  0, 31,  0)); // green
        break;
      default:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Blue);
        break;
    }

    gCurrentPaletteID     = 0;
    gCurrentPaletteAry[n] = gGradientPalettes[n][gCurrentPaletteID];    
    gTargetPaletteAry[n]  = gGradientPalettes[n][gCurrentPaletteID];    
  }
  FastLED.show();
  delay(3000);
}


//*********************************************************************************
void loop() {
  if (updPalettes == true) {
    for (n = 0; n<NUM_LED_STRIPS; n++) {
      palettetest(leds[n], n);
    }
    updPalettes = false;
  }

  while (!irrecv.isIdle());  // if not idle, wait till complete

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());

  if (irrecv.decode(&results)) {
    if (results.value == POWER) {
      if (ROTATE_COLORMODE) {
        if (DEBUG_MODE) Serial.println(F("POWER - toggling ROTATE_COLORMODE off"));    
        ROTATE_COLORMODE = false;
      } else {
        if (DEBUG_MODE) Serial.println(F("POWER - toggling ROTATE_COLORMODE ON"));    
        ROTATE_COLORMODE = true;        
      }
      blinkIt(5, 100, 1); // Penta blink once
    }
    if (results.value == A) {
      if (DEBUG_MODE) Serial.println(F("A - moving to colorwaves"));
      colorMode = 0;
      blinkIt(2, 100, 1); // Double blink once
    }
    if (results.value == B) {
      colorMode = 1; 
      if (DEBUG_MODE) Serial.println(F("B - moving to twinkle"));  
      blinkIt(2, 100, 2); // Double blink twice
    }
    if (results.value == C) {
      colorMode = 2; 
      if (DEBUG_MODE) Serial.println(F("C - moving to fireworks"));
      blinkIt(2, 100, 3); // Double blink thrice
    }
    if (results.value == UP) {
      if (DEBUG_MODE) Serial.print(F("UP - More Bumps - bumps threshold now at "));
      if (DEBUG_MODE) Serial.println(bumpVolBase * BUMP_THRESH);
      bumpVolBase = qsub8(bumpVolBase, 1);
      blinkIt(3, 100, 1); // Triple blink once
    }
    if (results.value == DOWN) {
      if (DEBUG_MODE) Serial.print(F("DOWN - Less Bumps - bumps threshold now at "));
      if (DEBUG_MODE) Serial.println(bumpVolBase * BUMP_THRESH);
      bumpVolBase = qadd8(bumpVolBase, 1);
      blinkIt(3, 100, 2); // Triple blink twice
    }
    if (results.value == LEFT) {
      if (DEBUG_MODE) Serial.println(F("LEFT - Previous Palette"));
      blinkIt(4, 100, 1); // Quatro blink twice
      gCurrentPaletteID = qsub8(gCurrentPaletteID, 1); // go back to previous palette
      for (n = 0; n<NUM_LED_STRIPS; n++) {
        gTargetPaletteAry[n]  = gGradientPalettes[n][gCurrentPaletteID];    
      }
      updPalettes = true;
      if (DEBUG_MODE) printPaletteNames((char*) "Mov:", gCurrentPaletteID);
    }
    if (results.value == RIGHT) {
      if (DEBUG_MODE) Serial.println(F("RIGHT - Next Palette"));
      blinkIt(4, 100, 2); // Quatro blink twice
      gCurrentPaletteID = addmod8( gCurrentPaletteID,1, gGradientPaletteCount-1); // go forward to next palette
      for (n = 0; n<NUM_LED_STRIPS; n++) {
        gTargetPaletteAry[n]  = gGradientPalettes[n][gCurrentPaletteID];    
      }
      updPalettes = true;
      if (DEBUG_MODE) printPaletteNames((char*) "Mov:", gCurrentPaletteID);
    }
    if (results.value == SELECT) {
      if (ROTATE_PALETTE) {
        if (DEBUG_MODE) Serial.println(F("SELECT - toggling ROTATE_PALETTE off"));    
        ROTATE_PALETTE = false;
      } else {
        if (DEBUG_MODE) Serial.println(F("SELECT - toggling ROTATE_PALETTE ON"));    
        ROTATE_PALETTE = true;        
      }
      blinkIt(5, 100, 2); // Penta blink twice
    }
    irrecv.resume();
  }
  
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteID    = random8( gGradientPaletteCount);  
    for (n = 0; n<NUM_LED_STRIPS; n++) {
      gTargetPaletteAry[n] = gGradientPalettes[n][gCurrentPaletteID];
    }
  }

  EVERY_N_SECONDS( SECONDS_PER_COLORMODE ) {
    colorMode = random8(3);
  }

  EVERY_N_MILLIS( 20 ) { // sample more often then we need
    ReadAndProcessBands();
  }
  
  EVERY_N_MILLIS( 40 ) {
    for (n = 0; n<NUM_LED_STRIPS; n++) {
      gHue[n] = gHue[n]+1; // slowly cycle the "base color" through the rainbow for confetti
      nblendPaletteTowardPalette( gCurrentPaletteAry[n], gTargetPaletteAry[n], 16);
    }
  }

  for (n = 0; n<NUM_LED_STRIPS; n++) {
    if (colorMode == 2) {
      fireworks(leds[n], n);
    } else if (colorMode == 1) {
      twinkle(leds[n], n);
    } else {
      colorwaves(leds[n], n);
    }
  }
  while (!irrecv.isIdle());  // if not idle, wait till complete

  FastLED.show();
  FastLED.delay(50); // match this to the fastest values above for palette updates
}

/*******************Pull frequencies from Spectrum Shield********************/
void ReadAndProcessBands(){
  int Frequencies_One;
  int Frequencies_Two; 
  uint8_t i, b;
  static uint16_t countSteps[] = {0,0,0};
  uint8_t tmpBump;

  // moved from globals to free up some memory for local allocation / re-use
  static uint8_t volume[NUM_LED_STRIPS]  = { 0,  0,  0}; //Holds the volume level read from the sound detector.
  static uint8_t lastVol[NUM_LED_STRIPS] = { 0,  0,  0}; //Holds the value of volume from the previous loop() pass.
  static uint8_t avgVol[NUM_LED_STRIPS]  = { 0,  0,  0}; //Holds the "average" volume-level to proportionally adjust the visual experience.
  //Read frequencies for each band, for both Left and Ride (stereo) music data
  for (i = 0; i<7; i++) {
    Frequencies_One = analogRead(DC_One);
    Frequencies_Two = analogRead(DC_Two); 
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
 
    if (Frequencies_One > Frequencies_Two) {
      volume[i] = (uint8_t) Frequencies_One;
    } else {
      volume[i] = (uint8_t) Frequencies_Two;
    }
  }

  uint8_t volThresh  = qmul8(bumpVolBase,VOL_THRESH);
  uint8_t bumpThresh = qmul8(bumpVolBase,BUMP_THRESH);

  for (i = 0; i < NUM_LED_STRIPS; i++) {
    countSteps[i] = countSteps[i] + 1; // uint16_t - no qadd16
    if (volume[b] > volThresh) {  //    /*Sets a min threshold for volume.
      musicOn = qadd8(musicOn, 1);
      b = STRIP_BAND_MAP[i]; // band
    // For each band we'll use, go find the volume, avgVol, maxVol, etc. info
      avgVol[i] = scale8(qadd8(qmul8(3,avgVol[i]),volume[b]),64); // weighted average
          
      //if there is a notable change in volume, trigger a "bump"
      tmpBump  = qsub8(volume[b],lastVol[i]); // qsub8 won't go below zero, so no need to check for underflow
      if (tmpBump > bumpThresh) {
        bump[i] = true;
        if (DEBUG_MODE) bumpCnt[i] = qadd8(bumpCnt[i],1);
        lastBump[i] = tmpBump;
      } else {
        bump[i] = false;
        lastBump[i] = 0;
      }
    } else {
      musicOn = qsub8(musicOn, 1);
	}
    lastVol[i] = volume[b]; //Records current volume for next pass

    if (countSteps[i] % 40 == 0) { // 250 steps @ 20mS should be 5S, but seems like that's ~6.5s, so do 6.5/5 of it
      if (DEBUG_MODE > 1) printSoundStats(i, "40S", musicOn, volume[b], lastBump[i], bumpCnt[i], bumpTkn[i]);      
      bumpCnt[i] = 0; // reset every time we print
      bumpTkn[i] = 0; // reset every time we print
      if (countSteps[i] > 20000) {
        countSteps[i] = 0;
        avgVol[i] = 64;
        countSteps[i] = 0;
      }
    }
  }
}

/* This function draws color waves with an ever-changing, widely-varying set of parameters, using a color palette. */
void colorwaves( CRGB* ledarray, uint8_t strip) {
  uint8_t numleds = STRIP_NUM_LEDS[strip];
  uint8_t tmpTaken = 0;
  CRGBPalette16 palette = gCurrentPaletteAry[strip];
  static uint16_t sPseudotime[NUM_LED_STRIPS] = { 0, 0, 0};
  static uint16_t sLastMillis[NUM_LED_STRIPS] = { 0, 0, 0};
  static uint16_t sHue16[NUM_LED_STRIPS] = { 0, 0, 0};
 
//  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
//  uint8_t msmultiplier = beatsin88(147, 23, 60);
  uint8_t msmultiplier = beatsin88(32, 7, 30); // trying to slow down the waves - jabenoit

  uint16_t hue16 = sHue16[strip];//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis[strip] ;
  sLastMillis[strip]  = ms;
  sPseudotime[strip] += deltams * msmultiplier;
  sHue16[strip] += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime[strip];

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
    pixelnumber = (numleds-1) - pixelnumber; 
    nblend( ledarray[pixelnumber], newcolor, 128);
    // bump if the volume says so, take a bump on random 25% on this strip  
    if ((musicOn >= MUSICON_THRESH) && (bump[strip] == true)) {
      if (random8(numleds) > scale8(qmul8(numleds,3),64)) { // only bump for a random 25% of pixels
        ledarray[pixelnumber] += CRGB( lastBump[strip], lastBump[strip], lastBump[strip]);
        tmpTaken = qadd8(tmpTaken,1);        
      }
    }
  }
  bumpTkn[strip] = qadd8(bumpTkn[strip],tmpTaken);
  bump[strip] = 0;
}

/* Alternate rendering function just scrolls the current palette across the defined LED strip. */
void palettetest( CRGB* ledarray, uint8_t strip) {
  uint8_t numleds = STRIP_NUM_LEDS[strip];
  CRGBPalette16 palette = gCurrentPaletteAry[strip];
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / STRIP_NUM_LEDS[n]) + 1, palette, 255, LINEARBLEND);
}

/* More LEDs off with some random twinkles */
void twinkle( CRGB* leds, uint8_t strip) {
  CRGBPalette16 palette = gCurrentPaletteAry[strip];

  for(uint8_t l = 0; l < STRIP_NUM_LEDS[strip]; l++) {
    if (leds[l]) { // lit to some degree
      leds[l].fadeToBlackBy(32);
    } else {
      // pick if we're doing it random or on music bumps
      if ((musicOn == 0 && random8(20) == 1) || ((musicOn >= MUSICON_THRESH) && (bump[strip] == true))) {  // if no music, random, otherwise on bump
        uint8_t i = random(STRIP_NUM_LEDS[strip]);
        if ((qadd8((qadd8(leds[l].red, leds[l].green)), leds[l].blue)) < SPARKLE_AT ) { // faded enough for another twinkle
          leds[i] = ColorFromPalette( palette, random(255), random(159,255));
          if (bump[strip] == true) bumpTkn[strip] = qadd8(bumpTkn[strip],1);    
        }
      } else {  
        leds[l] = CRGB(0, 0, 0);
      }
    }
  }
}

void fireworks( CRGB* leds, uint8_t strip) {
  CRGBPalette16 palette = gCurrentPaletteAry[strip];

  for(uint8_t l = 0; l < STRIP_NUM_LEDS[strip]; l++) {
    if (leds[l]) { // lit to some degree
      if (sparkleList[strip][l]) {
        leds[l].fadeToBlackBy(16);        
      } else if ((qadd8((qadd8(leds[l].red, leds[l].green)), leds[l].blue)) < SPARKLE_AT ) { // faded enough for a sparkle
        // have a 40% chance at a sparkle
        if (random8(5) <= 1) {
           leds[l] += CRGB(64,64,64);
           sparkleList[strip][l] = true;
        }
      } else {
        leds[l].fadeToBlackBy(8);
      }
    } else {
      sparkleList[strip][l] = false;
      // pick if we're doing it random or on music bumps
      if ((musicOn == 0 && random8(7) == 1) || ((musicOn >= MUSICON_THRESH) && (bump[strip] == true))) {  // if no music, random, otherwise on bump
        uint8_t i = random8(STRIP_NUM_LEDS[strip]);
        leds[i] = ColorFromPalette(palette, random8(192,255)); // assumes the brighter colors are on the higher end of the palette
        if (bump[strip] == true) bumpTkn[strip] = qadd8(bumpTkn[strip],1);    
      } else {
        leds[l] = CRGB(0, 0, 0);
      }
    }
  }
}

/* Blink the built-in Arduino LED ("L", next to Tx/Rx typically) to show we received an IR code */
void blinkIt (uint8_t numBlinks, uint16_t msDelay, uint8_t numRepeats) {
  for( int j = 0; j < numRepeats; j++) {
    for( int i = 0; i < numBlinks; i++) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      FastLED.delay(msDelay);            // wait a bit
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW 
      FastLED.delay(msDelay);            // wait a bit
    } 
    FastLED.delay(msDelay*2);            // wait a double bit
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
void printSoundStats ( uint8_t i, String msg, uint8_t tmusicon, uint8_t tvol, uint8_t tlastbump, uint8_t tbumpcnt, uint8_t tbumptkn  ) {
  uint16_t ms = millis();

  if (DEBUG_MODE) Serial.print(msg);
  if (DEBUG_MODE) Serial.print(" - ms ");
  if (DEBUG_MODE) Serial.print(ms);
  if (DEBUG_MODE) Serial.print(" - music ");
  if (DEBUG_MODE) Serial.print(tmusicon);
  if (DEBUG_MODE) Serial.print(" - str ");
  if (DEBUG_MODE) Serial.print(i);
  if (DEBUG_MODE) Serial.print(" - vol/bmp Thresh ");
  if (DEBUG_MODE) Serial.print(bumpVolBase * VOL_THRESH);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.print(bumpVolBase * BUMP_THRESH);
  if (DEBUG_MODE) Serial.print(" - vol/bmp ");
  if (DEBUG_MODE) Serial.print(tvol);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.print(tlastbump);
  if (DEBUG_MODE) Serial.print(" - #bmp/#tkn ");
  if (DEBUG_MODE) Serial.print(tbumpcnt);
  if (DEBUG_MODE) Serial.print("/");
  if (DEBUG_MODE) Serial.println(tbumptkn);
}
