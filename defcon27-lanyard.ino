/*
  fht_adc.pde
  guest openmusiclabs.com 9.5.12
  example sketch for testing the fht library.
  it takes in data on ADC0 (Analog0) and processes them
  with the fht. the data is sent out over the serial
  port at 115.2kb.  there is a pure data patch for
  visualizing the data.
*/


#include "Adafruit_ZeroFFT.h"
#include <FastLED.h>

#define TOTAL_LEDS 132
#define STARTING_BRIGHTNESS 100
#define BRIGHTNESS_REDUCTION 10

#define PIXEL_OFFSET 0


//set this to 0 to disable autoranging and scale to FFT_MAX
#define AUTOSCALE 1

#if AUTOSCALE == 0
#define FFT_MAX 512
#endif

#define SOUND_INPUT A0

#define DATA_SIZE 1024

//the sample rate.
#define FS 2360

#define NUM_REFERENCE_LINES 6

int16_t data[DATA_SIZE];

const int num_leds = TOTAL_LEDS;
const double COLOR_PER_LED = (255 / num_leds) / 2;
const int bins = 32;//(int)(log(FHT_N) / log(2));
const int leds_per_bin = (int)(num_leds / bins / 2);
const int threshold_depth = 10;

int threshold_over_count[32];
int threshold_under_count[32];
int learning_curve = 300;

int local_millis = 0;
float local_average  = 0;
int local_pixel_offset = 0;


int offset_value = 0;
CRGB leds[num_leds];
float threshold[bins];
int bin_brightness[bins];
int color_shift = 0;
float averages[bins][threshold_depth];


void setup() {

  FastLED.addLeds<APA102,11,13>(leds, num_leds);

  threshold[0] = 190;
  threshold[1] = 170;
  threshold[2] = 40;
  threshold[3] = 40;
  threshold[4] = 40;
  threshold[5] = 40;
  threshold[6] = 40;
  threshold[7] = 40;
  threshold[8] = 40;
  threshold[9] = 40;
  threshold[10] = 40;
  threshold[11] = 40;
  threshold[12] = 40;
  threshold[13] = 40;
  threshold[14] = 40;
  threshold[15] = 40;
  threshold[16] = 40;
  threshold[17] = 40;
  threshold[18] = 40;
  threshold[19] = 40;
  threshold[20] = 40;
  threshold[21] = 40;
  threshold[22] = 40;
  threshold[23] = 40;
  threshold[24] = 40;
  threshold[25] = 40;
  threshold[26] = 40;
  threshold[27] = 40;
  threshold[28] = 40;
  threshold[29] = 40;
  threshold[30] = 40;
  threshold[31] = 40;

  // Set the threshold counts to 0 so we can track the number of times the input value is over or under the threshold by 20%
  for(int i = 0; i<32;i++){
    threshold_over_count[i]=0;
    threshold_under_count[i]=0;
  }

  for (int q = 0; q < bins ; q++) {
    for (int t = 0; t < threshold_depth; t++) {
      averages[q][t] = 100;
    }
  }
  
}
void loop() {
    int32_t avg = 0;
  for (int i = 0; i < DATA_SIZE; i++) {
    int16_t val = analogRead(SOUND_INPUT);
    avg += val;
    data[i] = val;
  }

  //remove DC offset and gain up to 16 bits
  avg = avg / DATA_SIZE;
  for (int i = 0; i < DATA_SIZE; i++) {
    data[i] = (data[i] - avg) * 64;
  }

  //run the FFT
  ZeroFFT(data, DATA_SIZE);



  for (int i = 0; i < bins; i++) {
    // Code section to turn on the LEDs that have 
    if (data[i] > threshold[i]) {
      bin_brightness[i] = STARTING_BRIGHTNESS;
      threshold_under_count[i] = 0;
    } else {
      if (bin_brightness[i] <= BRIGHTNESS_REDUCTION) {
        bin_brightness[i] = 0;
      } else {
        bin_brightness[i] -= BRIGHTNESS_REDUCTION;
      }
    }

    //This is the code section dedicated to the threshold auto-leveling. I am not looking forward to this because it is always the part that breaks.
    // Start by checking the overs
    if(data[i] > threshold[i] *1.2){
      threshold_over_count[i] += 1;
      if(threshold_over_count[i] > learning_curve){
        threshold[i] = threshold[i] * 1.1;
        threshold_over_count[i] = 0;
      }
    }
    // and now the unders
    if(data[i] < threshold[i] * 0.8){
      threshold_under_count[i]++;
      if(threshold_under_count[i] > learning_curve * 2){
        threshold[i] = threshold[i] * 0.9;
        threshold_under_count[i] = 0;
      }
    }
    
    
    for (int j = 0; j < leds_per_bin; j++) {
      leds[abs(i * leds_per_bin + j - PIXEL_OFFSET - local_pixel_offset) % TOTAL_LEDS] = CHSV((abs(i * leds_per_bin*4 + j)), 255, bin_brightness[i]);
      leds[(TOTAL_LEDS - abs(i * leds_per_bin + j - PIXEL_OFFSET - local_pixel_offset)) % TOTAL_LEDS] = CHSV((abs(i * leds_per_bin*4 + j)), 255, bin_brightness[i]);
      //leds[(TOTAL_LEDS - abs(i * leds_per_bin + j - PIXEL_OFFSET - local_pixel_offset)) % TOTAL_LEDS] = CHSV(((i * leds_per_bin + j) * COLOR_PER_LED), 255, bin_brightness[i]);
      //This is a pretty cool Color shifting mode using color as the brightness as well
      //leds[i * leds_per_bin + j] = CHSV(bin_brightness[i]*2, 255, bin_brightness[i]);
      //leds[i * leds_per_bin + j] = CHSV((bin_brightness[i] + color_shift)%256, 255, bin_brightness[i]);
      /*
        // This is an interesting crawly effect.
        if ((4 - j) <= bin_brightness[i] / 25) {
        leds[i * leds_per_bin + j] = CHSV(((i * leds_per_bin + j) * COLOR_PER_LED), 255, 100 - bin_brightness[i] / (4 - j));
        } else {
        leds[i * leds_per_bin + j] = CHSV(((i * leds_per_bin + j) * COLOR_PER_LED), 255, 0);
        }//*/

      //DO NOT COLOR SHIFT HERE BAD IDEA WILL SEIZURE
      //color_shift++;
    }
    // Putting Color Shift out here makes it do a really nifty color rotationbut is less obvious what is showing up
    //color_shift++;
  }
/*  
 *  Old Crazy stuff, pretty but WAAY too fast and "loud"
    for (int i=0; i< num_leds; i++){
      leds[i] = CHSV((i*2+ offset_value)%255,255,200);
    }
  offset_value++;
*/
  FastLED.show();
}

