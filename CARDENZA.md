# Cardenza runtime support

Fork: https://github.com/203Null/M5Unified, branch `codex/cardenza-autodetect`.
Based on upstream tag 0.2.24. MIT; original M5Stack attribution is retained.
Requires M5GFX >=0.2.31. App builds pin the exact fork commit.

`M5.begin()` identifies Cardenza only on a Cardputer-family display/fallback,
using GPIO2 SDA / GPIO1 SCL, address 0x08, ES8156 IDs FD=81, FE=55,
revision FF upper nibble=1. ACK alone does not identify the board. Probing
uses upstream software I2C and restores pins, leaving hardware buses alone.
A failed probe follows upstream board detection, without ES8156 writes or
GPIO21 hold. This does not turn a physically faulty Cardenza bus into a
working board; probe faults need hardware diagnosis.

On identified Cardenza: active-low keyboard LED enable GPIO21 is held high,
M5 RGB driver and battery ADC setup are skipped, IMU setup is disabled,
GPIO46 remains PDM data and GPIO43 its clock. `getBoard()` remains
`board_M5Cardputer`, so M5Cardputer 1.1.1 uses its matrix keyboard.
Original Cardputer and ADV keep upstream ES8311/keyboard/power behavior.
This targets the current Cardenza revision; a future reversed LED DAT/EN
revision needs its own identification/wiring policy before use.

Audio output: Philips I2S, GPIO41 BCLK / GPIO43 LRCK / GPIO42 data,
16-bit slots, BCLK/LRCK=32. `Speaker.begin()` reconfigures ES8156 after PDM
recording, using stereo physical slots even with mono PCM input.
`M5.isCardenza()` and `M5.cardenzaCodecReady()` expose detected identity
and codec setup status. `M5.configureCardenzaAudio(frame_bits,sample_bits)`
lets raw-I2S apps choose 32/16 or 64/(16,24,32); stop Speaker first.
Never keep app render tasks calling playRaw while Mic owns GPIO43.

App-owned FastLED/NeoPixel, hardcoded battery ADC, custom IMU setup,
GPIO1/2 MIDI and raw audio drivers must still dispatch at runtime. Merely
replacing a dependency cannot fix these paths. Launcher remains separate;
this library neither changes bootloader nor repartitions/erases flash.

Host fault injection (no hardware claims):
`g++ -std=c++11 tests/cardenza_codec.cpp -o cardenza_codec_test`
then run the executable. App compilation, flash hash and physical
sound/keyboard validation are separate verification steps.
