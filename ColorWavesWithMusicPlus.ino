#include "FastLED.h"
#include "IRremote.h"

// Mix of ColorWavesWithPalettes (to handle 3 strips uniquely)
//        SparkFun Music Visualizer Tutorial (process frequency data, find "bump"
//        SparkFun Simple IR Remote (simple control on modes, palettes, bump speed)
//        Adafruit Twinkle (use FastLED library and palettes)
//        FastLED Confetti (use palettes)
/*        AFAIK, this will *not* run on Uno's and other devices w/ only 2KB memory -
             you'll need to take out some code (probably either IR remote or Music Vis
             code would be enough) - threshold is somewhere in the 800+ bytes for local
             variables. */
// Rev1.0 - by John Benoit, December 29 2016 */

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
#define BRIGHTNESS  24

#define NUM_LED_STRIPS   3
#define STRIP_DATA_PIN0  8
#define STRIP_DATA_PIN1  9
#define STRIP_DATA_PIN2 10
const uint8_t STRIP_NUM_LEDS[]  = { 7, 12, 24 };
const uint8_t STRIP_BAND_MAP[] = { 5,  3,  1}; // Strip 2 uses band 1 (Bass), Strip 1 uses band 3 (Tenor), Strip 0 uses band 5 (Soprano)

#define MAX_LEDS_IN_STRIP 24
//#define NUM_LEDS          7+12+30 // could do this algorythmically, but only expect a few strips...

CRGB leds[NUM_LED_STRIPS][MAX_LEDS_IN_STRIP];

// ten seconds per color palette makes a good demo, 20-120 is better for deployment
#define SECONDS_PER_PALETTE 20
#define SECONDS_PER_COLORMODE 60
bool ROTATE_PALETTE = true;    // can toggle by IRRemote (Select)
bool ROTATE_COLORMODE = true; // can toggle by IRRemote (Select)

// n for numStrip in for loops - ensure in code doesn't have conflicting usage (and don't keep adding/remove n from heap)
uint8_t n; 

uint8_t colorMode = 0; // 0=A=colorwaves, 1=B=twinkle, 2=C=confetti

//*********************************************************************************
// Spectrum Shield and Music processing globals
// Declare Spectrum Shield pin connections
#define STROBE 4
#define RESET 6
#define DC_One A0
#define DC_Two A1
#define NUM_BANDS 7 

//Define spectrum variables
bool    bump[NUM_LED_STRIPS] = {};      // Used to pass if there was a "bump" in volume
uint8_t bumpCnt[NUM_LED_STRIPS] = {};   // Holds how many bumps were triggered since last cleanup
uint8_t takenCnt[NUM_LED_STRIPS] = {};  // Holds how many bumps were taken since last cleanup (should be > bumpCnt) - 
uint8_t avgVol[NUM_LED_STRIPS]  = {}; //Holds the "average" volume-level to proportionally adjust the visual experience.
uint8_t bumpLvl[NUM_LED_STRIPS] = {};   // level of bump
uint8_t bumpVolBase = 5;  // multiplied by BUMP_THRESH and VOL_THRESH as input limits
#define BUMP_THRESH 12  // decrease to make fewer bump flashes, increase for less, see serial log for avgBump to tune
#define VOL_THRESH  16  // for volume calculations, ignore values less than this (skip them)

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
// Global Variables for Twinkle (from Adafruit NeoPixel) and Confetti
float fadeRate = 0.96;
uint8_t gHue[] = { 0, 0, 0}; // rotating "base color" used by many of the patterns

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

/* Based on Gradient palette "GMT_seis_gp", originally from
   http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_seis.png.index.html
   converted for FastLED with gammas (2.6, 2.2, 2.5)
   Size: 40 bytes of program space. */
DEFINE_GRADIENT_PALETTE( set2_GMT_seis_gp ) {
    0,  88,  0,  0,
   82, 255,  0,  0,
  163, 255, 22,  0,
  255, 255,104,  0};
DEFINE_GRADIENT_PALETTE( set1_GMT_seis_gp ) {
    0, 255,104,  0,
   82, 255,255,  0,
  163, 255,255,  0,
  255,  17,255,  1};
DEFINE_GRADIENT_PALETTE( set0_GMT_seis_gp ) {
    0,  17,255,  1,
   82,   0,223, 31,
  163,   0, 19,255,
  255,   0,  0,147};

/* Made by hand */
DEFINE_GRADIENT_PALETTE( set2_calla_01_gp ) {
   0,   0, 31,  0,
 255,   0,159,  0};
DEFINE_GRADIENT_PALETTE( set1_calla_01_gp ) {
   0,  95, 63, 95,
 255, 159,191,159};
DEFINE_GRADIENT_PALETTE( set0_calla_01_gp ) {
   0, 255,127,  0,
 255, 255,221,  0};

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
  set0_calla_01_gp,
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
  set0_GMT_seis_gp,
  };

// Section1 (middle)
const TProgmemRGBGradientPalettePtr gGradientPalettes_1[] = {
  set1_calla_01_gp,
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
  set1_GMT_seis_gp
  };

// Section2 (bottom / exterior)
const TProgmemRGBGradientPalettePtr gGradientPalettes_2[] = {
  set2_calla_01_gp,
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
  set2_GMT_seis_gp,
  };
  
const TProgmemRGBGradientPalettePtr* gGradientPalettes[] = { gGradientPalettes_0, gGradientPalettes_1, gGradientPalettes_2 };

// Current palette number from the 'playlist' of color palettes
uint8_t       gCurrentPaletteID;
CRGBPalette16 gCurrentPaletteAry[NUM_LED_STRIPS];
CRGBPalette16 gTargetPaletteAry[NUM_LED_STRIPS];

bool updPalettes = true; // use for setting palette away from RGB on the 1st pass, and then updating on userInput

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
  if (DEBUG_MODE) memset (bumpCnt,    0, NUM_LED_STRIPS);
  if (DEBUG_MODE) memset (takenCnt,   0, NUM_LED_STRIPS);
  memset (bumpLvl,    0, NUM_LED_STRIPS);
  memset (avgVol,     0, NUM_LED_STRIPS);
  memset (bump,   false, NUM_LED_STRIPS);
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
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB(  0, 31,  0));
        break;
      default:
        fill_solid( leds[x], STRIP_NUM_LEDS[x], CRGB::Blue);
        break;
    }

    gCurrentPaletteID     = 0;
    gTargetPaletteAry[n]  = gGradientPalettes[n][gCurrentPaletteID];    
  }
  FastLED.show();
  delay(2000);
}

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
      if (DEBUG_MODE) Serial.println(F("POWER - toggling ROTATE_COLORMODE"));    
      if (ROTATE_COLORMODE) {
        ROTATE_COLORMODE = false;
      } else {
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
      if (DEBUG_MODE) Serial.println(F("C - moving to confetti"));
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
      if (DEBUG_MODE) Serial.println(F("SELECT - toggling ROTATE_PALETTE"));    
      if (ROTATE_PALETTE) {
        ROTATE_PALETTE = false;
      } else {
        ROTATE_PALETTE = true;        
      }
      blinkIt(5, 100, 2); // Penta blink twice
    }
    irrecv.resume();
  }
  
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteID    = addmod8( gCurrentPaletteID,1, gGradientPaletteCount-1);     
    for (n = 0; n<NUM_LED_STRIPS; n++) {
      gTargetPaletteAry[n] = gGradientPalettes[n][gCurrentPaletteID];
    }
  }

  EVERY_N_MILLIS( 10 ) { // keep at different rate then blend, so the bumps move around
    ReadAndProcessBands();
  }
  
  EVERY_N_MILLIS( 20 ) {
    for (n = 0; n<NUM_LED_STRIPS; n++) {
      gHue[n] = gHue[n]+1; // slowly cycle the "base color" through the rainbow for confetti
      if (colorMode == 0) { // blend for colorwaves
        nblendPaletteTowardPalette( gCurrentPaletteAry[n], gTargetPaletteAry[n], 16);
      }
    }
  }

  for (n = 0; n<NUM_LED_STRIPS; n++) {
    if (colorMode == 2) {
//      Fire2012WithPalette(leds[n], n);
      confetti(leds[n], n);
    } else if (colorMode == 1) {
      twinkle(leds[n], n);
    } else {
      colorwaves(leds[n], n);
    }
  }

  while (!irrecv.isIdle());  // if not idle, wait till complete

  FastLED.show();
  FastLED.delay(30); // match this to the fastest values above for palette updates
}

/*******************Pull frequencies from Spectrum Shield********************/
void ReadAndProcessBands(){
  uint8_t freq_amp;
  int Frequencies_One;
  int Frequencies_Two; 
  uint8_t i, b;
  static uint16_t countSteps[] = {0,0,0};
  uint8_t tmpBump, lastBump;

  // moved from globals to free up some memory for local allocation / re-use
  uint8_t volume[NUM_LED_STRIPS]  = { 0,  0,  0}; //Holds the volume level read from the sound detector.
  uint8_t last[NUM_LED_STRIPS]    = { 0,  0,  0}; //Holds the value of volume from the previous loop() pass.
  uint8_t maxVol[NUM_LED_STRIPS]  = {72, 72, 72}; //Holds the largest volume recorded thus far to proportionally adjust the visual's responsiveness.
  uint8_t avgBump[NUM_LED_STRIPS] = { 0,  0,  0}; //Holds the "average" volume-change to trigger a "bump."
  uint8_t maxBump[NUM_LED_STRIPS] = {16, 16, 16}; //Holds the max bump found


  //Read frequencies for each band, for both Left and Ride (stereo) music data
  for (freq_amp = 0; freq_amp<7; freq_amp++) {
    Frequencies_One = analogRead(DC_One);
    Frequencies_Two = analogRead(DC_Two); 
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
 
    if (Frequencies_One > Frequencies_Two) {
      volume[freq_amp] = (uint8_t) Frequencies_One;
    } else {
      volume[freq_amp] = (uint8_t) Frequencies_Two;
    }
  }

  for (i = 0; i < NUM_LED_STRIPS; i++) {
    countSteps[i] = countSteps[i] + 1; // uint16_t - no qadd16
    if (volume[b] > qmul8(bumpVolBase,VOL_THRESH)) {  //    /*Sets a min threshold for volume.
      b = STRIP_BAND_MAP[i]; // band
    // For each band we'll use, go find the volume, avgVol, maxVol, etc. info
      avgVol[i] = scale8(qadd8(qmul8(3,avgVol[i]),volume[b]),64); // weighted average
              
      //If the current volume is larger than the loudest value recorded, overwrite
      if (volume[b] > maxVol[i]) maxVol[i] = volume[b];
  
      //If there is a decent change in volume since the last pass, average it into "avgBump"
      tmpBump  = qsub8(volume[b],last[i]); // qsub8 won't go below zero, so no need to check for underflow
      lastBump = qsub8(avgVol[b],last[i]); // qsub8 won't go below zero, so no need to check for underflow
      if ((tmpBump > (lastBump)) && (lastBump > 0)) avgBump[i] = scale8(qadd8(qmul8(avgBump[i],3),tmpBump),64); // weighted average
      if (tmpBump > maxBump[i]) maxBump[i] = tmpBump;
    
      //if there is a notable change in volume, trigger a "bump"
      if ((tmpBump > qmul8(bumpVolBase,BUMP_THRESH)) && (tmpBump > scale8(qmul8(3,avgBump[i]),2))) {  // comparing tmpBump to 1.5 * avgBump using fastled math
        bump[i] = true;
        if (DEBUG_MODE) bumpCnt[i] = qadd8(bumpCnt[i],1);
        bumpLvl[i] = tmpBump;
      } else {
        bumpLvl[i] = 0;
      }
      last[i] = volume[b]; //Records current volume for next pass
    }
    if (countSteps[i] % 40 == 0) { // 250 steps @ 20mS should be 5S, but seems like that's ~6.5s, so do 6.5/5 of it
      if (DEBUG_MODE) printSoundStats(i, "40Steps", volume[b], maxVol[i], avgVol[i], bump[i], maxBump[i], bumpCnt[i], takenCnt[i]);      
      if (DEBUG_MODE) bumpCnt[i]  = 0; // reset every time we print
      if (DEBUG_MODE) takenCnt[i] = 0; // reset every time we print
      if (countSteps[i] > 20000) {
        countSteps[i] = 0;
        maxVol[i] = 64;
        avgVol[i] = 64;
        maxBump[i] = 16; // reset every time we print
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
  uint8_t msmultiplier = beatsin88(147, 23, 60);

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
    // bump if the volume says so, and then for every other (2nd) led - I tried other modulos there, but any variables caused false bump (takeCnt >0, bumpCnt = 0)  
    if ((bump[strip] == true) && (mod8(pixelnumber,2) == 0)) {
      if (random8(numleds) > scale8(qmul8(numleds,3),64)) { // only bump for a random 25% of pixels
        ledarray[pixelnumber] += CRGB( bumpLvl[strip], bumpLvl[strip], bumpLvl[strip]);
        if (DEBUG_MODE) tmpTaken = qadd8(tmpTaken,1);        
      }
    }
  }
  if (DEBUG_MODE) takenCnt[strip] = qadd8(takenCnt[strip],tmpTaken);
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

// pick if we're doing it random or on musix bumps
 if (((avgVol[strip] < (bumpVolBase*VOL_THRESH)) && random8(7) == 1) || bump[strip]) {  // if no music, random, otherwise on bump
    uint8_t i = random(STRIP_NUM_LEDS[strip]);
    if (leds[i].red < 1 && leds[i].green < 1 && leds[i].blue < 1) {
      leds[i] = ColorFromPalette( palette, random(256), random(127,255));
      takenCnt[strip] = qadd8(takenCnt[strip],1);    
    }
  }
  
  for(uint8_t l = 0; l < STRIP_NUM_LEDS[strip]; l++) {
    if (leds[l]) { // lit to some degree
      if (leds[l].red) {
        leds[l].red = leds[l].red * fadeRate;
      } else {
        leds[l].red = 0;
      }
      if (leds[l].green > 1) {
        leds[l].green = leds[l].green * fadeRate;
      } else {
        leds[l].green = 0;
      }
      if (leds[l].blue > 1) {
        leds[l].blue = leds[l].blue * fadeRate;
      } else {
        leds[l].blue = 0;
      }
    } else {
      leds[l] = CRGB(0, 0, 0);
    }
  }
}

/* More LEDs on random changes and fading */
void confetti(CRGB* leds, uint8_t strip) {
  // random colored speckles that blink in and fade smoothly
  CRGBPalette16 palette = gCurrentPaletteAry[strip];
  fadeToBlackBy( leds, STRIP_NUM_LEDS[strip], 10);
  uint8_t i = random8(STRIP_NUM_LEDS[strip]);
  if (((avgVol[strip] < (bumpVolBase*VOL_THRESH)) && random8(3) == 1) || bump[strip]) { // if no music, random, otherwise on bump
    leds[i] = ColorFromPalette( palette, random(256), random(127,255));
  } else {
    leds[i] = ColorFromPalette( palette, random(256), 128);
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
void printSoundStats ( uint8_t i, String msg, uint8_t tvol, uint8_t tmaxvol, uint8_t tavgvol, bool tbump, uint8_t tmaxbump, uint8_t tbumpcnt, uint8_t ttakencnt  ) {
  uint8_t b = STRIP_BAND_MAP[i]; // band
  uint16_t ms = millis();

  if (DEBUG_MODE) Serial.print(msg);
  if (DEBUG_MODE) Serial.print(" - ms ");
  if (DEBUG_MODE) Serial.print(ms);
  if (DEBUG_MODE) Serial.print(" - str ");
  if (DEBUG_MODE) Serial.print(i);
  if (DEBUG_MODE) Serial.print(" - bnd ");
  if (DEBUG_MODE) Serial.print(b);
  if (DEBUG_MODE) Serial.print(" - vol ");
  if (DEBUG_MODE) Serial.print(tvol);
  if (DEBUG_MODE) Serial.print(" - max ");
  if (DEBUG_MODE) Serial.print(tmaxvol);
//  if (DEBUG_MODE) Serial.print(" - avg ");
//  if (DEBUG_MODE) Serial.print(tavgvol);
//  if (DEBUG_MODE) Serial.print(" - bump ");
//  if (DEBUG_MODE) Serial.print(tbump);
  if (DEBUG_MODE) Serial.print(" - maxBump ");
  if (DEBUG_MODE) Serial.print(tmaxbump);
  if (DEBUG_MODE) Serial.print(" - #bmp ");
  if (DEBUG_MODE) Serial.print(tbumpcnt);
  if (DEBUG_MODE) Serial.print(" - #tkn ");
  if (DEBUG_MODE) Serial.println(ttakencnt);
}



