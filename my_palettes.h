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

const uint8_t gGradientPaletteCount = 19;

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
  set1_rainbow_gp,
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
  set2_rainbow_gp,
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
