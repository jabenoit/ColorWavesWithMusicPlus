# ColorWavesWithMusicPlus
Combination of FastLED ColorWave code, SparkFun Spectrum Analyzer, IR Remote and a few other items.
I had problems w/ dynamic memory on Arduino Uno R3 (or comparable) with 2K dynamic memory, but works
OK on my Arduino Leonardo. If you want to run it on a Uno (or other Arduino w/ 2K dynamic memory), you'll 
need to remove either the music 'ReadAndProcessFreq' code or the IRRemote code.

Based on 2016-2017 libraries - FASTLed 3.1.3 and IRremote 2.2.3
