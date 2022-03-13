/* Swan Samba Drum Lights receiver
 *
 * by Phil Dye <phil@phildye.org>
 * for Swan Samba Band <https://www.swansamba.co.uk>
 *
 * Works with the Swan Samba Drum Lights Transmitter
 * to receive desired colour/pattern commands from a web UI
 * by nRF24L01 radio module, and outputs to a WS2812B LED strip
 * 
 */

#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <FastLED.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// Setup the nRF24L01 radio
#define NRF24L01_PIN_CE	4
#define NRF24L01_PIN_CS	15
struct RF24 radio(NRF24L01_PIN_CE, NRF24L01_PIN_CS);
const byte address[6] = "00001";

// Setup the LEDs
#define NUM_LEDS    60      // TODO Change this for each drum size
#define DATA_PIN    2
#define VOLTS       5
#define MAX_MA      2000
#define FRAMES_PER_SECOND 20
uint8_t max_bright = 200;     // Overall brightness definition, could be changed on the fly

// The array of leds, one for each led in the strip
struct CRGB leds[NUM_LEDS];

uint8_t gHue = 0;     // rotating "base color" used by many of the patterns
uint8_t ledMode = 0;   // the currently active pattern

typedef void (*SimplePatternList[])();    // List of patterns to cycle through.  Each is defined as a separate function below.

void allOff() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}
void showReady() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fill_solid(leds, 1, CRGB::Green);
}
void allGreen() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);
}
void allYellow() {
  fill_solid(leds, NUM_LEDS, CRGB::Yellow);
}
void allBlue() {
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
}
void allRed() {
  fill_solid(leds, NUM_LEDS, CRGB::Red);
}

void chase(CRGB color) {

  static uint8_t i;
  const uint8_t SEGMENTS = 6;

  static uint8_t segmentSize = (uint8_t) NUM_LEDS / SEGMENTS;

  fadeToBlackBy(leds, NUM_LEDS, 100);

  for (uint8_t s = 0; s < SEGMENTS; s++)
  {
    uint8_t target = i + (segmentSize * s);
    if (target > NUM_LEDS) target = NUM_LEDS - 1;
    leds[target] = color;
  }
  
  i++;
  // restart once we reach the end of each segment
  if (i == segmentSize) i = 0;
}
void greenChase() {
  chase(CRGB::Green);
}
void yellowChase() {
  chase(CRGB::Yellow);
}
void blueChase() {
  chase(CRGB::Blue);
}
void redChase() {
  chase(CRGB::Red);
}

void rioSpin() {}
void rioChase() {}
void rioFade() {}
void rioFlag() {}


SimplePatternList gPatterns = {showReady, greenChase, yellowChase, blueChase, redChase, rioSpin, rioChase, rioFlag };

void setup() {
  Serial.begin(115200);
   while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
  delay(1000);

  Serial.println("Setup...");

  Serial.print("Setting up LEDs... ");
  delay(1000);          // Soft startup to ease the flow of electrons
  LEDS.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness( max_bright );
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS,MAX_MA); 
  Serial.println("Done");

  Serial.print("Setting up radio...");
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    fill_solid(leds, 1, CRGB::Red);
    FastLED.show();
    while (1) {} // hold in infinite loop
  }

  radio.openReadingPipe(1, address);
  radio.startListening();         // put radio in TX mode
  // radio.printPrettyDetails(); // (larger) function that prints human readable data
  Serial.println("Done");

  Serial.print("Ready...");
  // show we're ready
  showReady();  
  Serial.println("Done");

}

void readRadio() {
  uint8_t pipe;
  if (radio.available(&pipe)) {             // is there a payload?
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&ledMode, bytes);            // fetch payload from FIFO
    Serial.print("Radio RX data: ");
    Serial.println(ledMode);

    if (ledMode > 47) {
      ledMode = ledMode-48;    // seems to be an offset?!
    }

    Serial.print("ledMode: ");
    Serial.println(ledMode);

    FastLED.clear();
    if (ledMode > sizeof(gPatterns))
    {
      Serial.println("ERROR - greater than defined gPatterns!!");
      fill_solid(leds, 1, CRGB::HotPink);

    }
  }
}

void loop() {

  readRadio();

  EVERY_N_MILLISECONDS(1000 / FRAMES_PER_SECOND) {
    gPatterns[ledMode]();       // Call the current pattern function, updating the 'leds' array    
  }

  FastLED.show();

  // insert a delay to keep the framerate modest
  // FastLED.delay(1000 / FRAMES_PER_SECOND);

}