#include <T3PWM.h>
#include <T1PWM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <SD.h>
#include <DS3232RTC.h>
#include <TimeLib.h>

// Oled/SD card pins
#define sclk 21
#define mosi 22
#define miso 23
#define dc   40
#define ocs  42
#define rst  41
#define sd_cs 43

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define MINT            0x07F0

#define BUFFPIXEL 20

File bmpFile;
int bmpWidth, bmpHeight;
uint8_t bmpDepth, bmpImageoffset;
bool tftini = true;
Adafruit_SSD1351 tft = Adafruit_SSD1351(ocs, dc, rst);

unsigned long ctr;


/* hold output state */
unsigned long out1, out2, out3;

/* hold state info */
unsigned int state_chan1, state_chan2, state_chan3;
byte state_fan;

/*
 * Sunrise and Sunset start times per CH
 * all times declared in seconds
 */
unsigned long C1TurnOn = 36000; // time CH1 dawn begins > violet/cool blue
unsigned long C1TurnOff = 75600; // time CH1 sunset begins > violet/cool blue
unsigned long C2TurnOn = 36000; // time CH2 dawn begins > cyan
unsigned long C2TurnOff = 75600; // time CH2 sunset begins > cyan
unsigned long C3TurnOn = 36000; // time CH3 dawn begins > white
unsigned long C3TurnOff = 75600; // time CH3 sunset begins > white

/*
 * Light "state" represents the PWM duty cycle for each channel This normally
 * dictates light intensity. Possible values for duty cycle are 0 - 16383.
 */
unsigned int C1DayState = 6000; // CH1 daytime LED intensity > violet/cool blue
unsigned int C1NightState = 0; // CH1 night time LED intensity > violet/cool blue
unsigned int C2DayState = 2348; // CH2 daytime LED intensity > cyan
unsigned int C2NightState = 0; // CH2 night time LED intensity > cyan
unsigned int C3DayState = 4696; // CH3 daytime LED intensity > white
unsigned int C3NightState = 0; // CH3 night time LED intensity > white

/*
 * Fade Duration
 * duration (in seconds) of fade per CH.
 */
unsigned long C1FadeDuration = 3000; // 50 min > ch1 violet/cool blue
unsigned long C2FadeDuration = 3000; // 50 min > ch2 cyan
unsigned long C3FadeDuration = 3600; // 1hrs > ch3 white

/*
 * fader -- Determine output state for a given time to provide smooth fade from
 * one state to another.
 *     Args:
 *     start_time  -- time (in seconds) of start of fade
 *     start_state -- beginning state
 *     end_state   -- ending state
 */
void fader1(long start_time1, const int start_state1, const int end_state1) {
  float per_second_delta_1 = (float) (end_state1-start_state1)/C1FadeDuration;
  long elapsed1 = ctr-start_time1;
  out1 = start_state1 + per_second_delta_1 * elapsed1;
}

void fader2(long start_time2, const int start_state2, const int end_state2) {
  float per_second_delta_2 = (float) (end_state2-start_state2)/C2FadeDuration;
  long elapsed2 = ctr-start_time2;
  out2 = start_state2 + per_second_delta_2 * elapsed2;
}

void fader3(long start_time3, const int start_state3, const int end_state3) {
  float per_second_delta_3 = (float) (end_state3-start_state3)/C3FadeDuration;
  long elapsed3 = ctr-start_time3;
  out3 = start_state3 + per_second_delta_3 * elapsed3;
}

/* Return seconds elapsed since midnight */
long seconds_since_midnight() {
  time_t t = now();
  long hr = hour(t);
  long min = minute(t);
  long sec = second(t);
  long total = hr * 3600 + min * 60 + sec;
  return total;
}

/*
 * Set Output
 * Sets current CH PWM output on appropriate pin
 * and updates State info per CH
 */
void set_state1(const int state1) {
  if (state1 >= 0 && state1 <= 16383) {
      SetPWM3A(state1);
    state_chan1 = state1; }
}

void set_state2(const int state2) {
  if (state2 >= 0 && state2 <= 16383) {
       SetPWM3B(state2);
       state_chan2 = state2; }
}

void set_state3(const int state3) {
  if (state3 >= 0 && state3 <= 16383) {
       SetPWM3C(state3);
       state_chan3 = state3; }
}

/*
 * determine states -- This is where the actual timing logic resides.  We
 * examine ctr (seconds since midnight) and then set output state accordingly.
 * Variable ctr rolls back to 0 at midnight so stages that cross midnight (ie:
 * nighttime) are broken up into two stages.
 */
void determine_state1() {
  if ( ctr >= 0 && ctr < C1TurnOn ) { // night
    set_state1(C1NightState);
    bmpDraw("moon.bmp", 37, 10);
  } else if ( ctr >= C1TurnOn && ctr <= (C1TurnOn+C1FadeDuration) ) { // sunrise
    fader1(C1TurnOn, C1NightState, C1DayState);
    set_state1(out1);
    bmpDraw("sunrise.bmp", 37, 10);
  } else if ( ctr > (C1TurnOn+C1FadeDuration) && ctr < C1TurnOff ) { // day
    set_state1(C1DayState);
    bmpDraw("sun.bmp", 37, 10);
  } else if ( ctr >= C1TurnOff && ctr <= (C1TurnOff+C1FadeDuration) ) { // sunset
    fader1(C1TurnOff, C1DayState, C1NightState);
    set_state1(out1);
    bmpDraw("sunset.bmp", 37, 10);
  } else if ( ctr > (C1TurnOff+C1FadeDuration) && ctr < 86400 ) { // night
    set_state1(C1NightState);
    bmpDraw("moon.bmp", 37, 10);
  }
}

void determine_state2() {
  if ( ctr >= 0 && ctr < C2TurnOn ) { // night
    set_state2(C2NightState);
  } else if ( ctr >= C2TurnOn && ctr <= (C2TurnOn+C2FadeDuration) ) { // sunrise
    fader2(C2TurnOn, C2NightState, C2DayState);
    set_state2(out2);
  } else if ( ctr > (C2TurnOn+C2FadeDuration) && ctr < C2TurnOff ) { // day
    set_state2(C2DayState);
  } else if ( ctr >= C2TurnOff && ctr <= (C2TurnOff+C2FadeDuration) ) { // sunset
    fader2(C2TurnOff, C2DayState, C2NightState);
    set_state2(out2);
  } else if ( ctr > (C2TurnOff+C2FadeDuration) && ctr < 86400 ) { // night
    set_state2(C2NightState);
  }
}

void determine_state3() {
  if ( ctr >= 0 && ctr < C3TurnOn ) { // night
    set_state3(C3NightState);
  } else if ( ctr >= C3TurnOn && ctr <= (C3TurnOn+C3FadeDuration) ) { // sunrise
    fader3(C3TurnOn, C3NightState, C3DayState);
    set_state3(out3);
  } else if ( ctr > (C3TurnOn+C3FadeDuration) && ctr < C3TurnOff ) { // day
    set_state3(C3DayState);
  } else if ( ctr >= C3TurnOff && ctr <= (C3TurnOff+C3FadeDuration) ) { // sunset
    fader3(C3TurnOff, C3DayState, C3NightState);
    set_state3(out3);
  } else if ( ctr > (C3TurnOff+C3FadeDuration) && ctr < 86400 ) { // night
    set_state3(C3NightState);
  }
}

/*
 * Utility function for digital clock, adds leading zero to value and prints to oled.
 */
void printLeadingZero(int digits) {
  if (digits < 10) {
    tft.print('0');
    tft.print(digits);
}
  else {
    tft.print(digits);
  }
}


/*
 * Utility function to convert 16bit and 8bit pwm value to percent.
 */
void pwmPercent16 (float stateinfo) {
  int output = (stateinfo/16383)*100;
  tft.print(output);
  tft.print(F("%"));
}

void pwmPercent16s (float stateinfo) {
  int output = (stateinfo/16383)*100;
  Serial1.print(output);
  Serial1.print(F("%"));
}

void pwmPercent8 (float stateinfo) {
  int output = (stateinfo/256)*100;
  Serial1.print(output);
  Serial1.println(F("%"));
}

/*
 * Utility prints .bmp assets from sd card to oled.
 */
void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;

  if((x >= tft.width()) || (y >= tft.height())) return;


  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
   // Serial1.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    read32(bmpFile);
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    // Read DIB header
    read32(bmpFile);
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      bmpDepth;
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        for (row=0; row<h; row++) { // For each scanline...
          tft.goTo(x, y+row);

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          // optimize by setting pins now
          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];

            tft.drawPixel(x+col, y+row, tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial1.println(F("BMP format not recognized."));
}

/* These read 16- and 32-bit types from the SD card file.
 * BMP data is stored little-endian, Arduino is little-endian too.
 * May need to reverse subscript order if porting elsewhere.
 */
uint16_t read16(File& f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File& f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}



/*
 * Prints led channel info (%) to OLED.
 */
void PrimaryInfoS() {
  int temp = ((RTC.temperature() / 4) * 9 / 5 + 32);
  time_t t = now(); 
  
  tft.setTextColor(BLACK,MINT);
  tft.setTextSize(1);
  tft.setCursor(100,3);
  tft.print(temp);
  tft.print(" F");
  tft.setCursor(40,80);
  tft.print(F("ch1  ch2  ch3"));
  tft.setCursor(40,95);
  pwmPercent16(state_chan1);
  tft.print(F(" "));
  tft.setCursor(70,95);
  pwmPercent16(state_chan2);
  tft.print(F(" "));
  tft.setCursor(100,95);
  pwmPercent16(state_chan3);
  tft.print(F(" "));
  
  if (state_fan >= 127) {
  bmpDraw("fan2.bmp", 8, 78);  
  }
  else {
  tft.fillRect(8, 78, 24, 24, MINT);
  }
  tft.setCursor(3,115);
  if (hour(t) > 12) {
  int hourpm = hour(t)-12;
  tft.print(hourpm);
  }
  else if (hour(t) == 0) {
  tft.print("12");
  }
  else {
  tft.print(hour(t));
  }
  tft.print(F(":"));
  printLeadingZero(minute(t));
  //display.print(":");
  //printLeadingZero(second(t));
  if (hour(t) >= 0 && hour(t) <= 11) {
  tft.print(F("am "));
  }
  else {
  tft.print(F("pm "));
  }
  tft.setCursor(66,115);
  tft.printf("%02d/%02d/%02d", month(t), day(t), year(t));
}

/*
 * Fan Control
 * Sets an 8bit PWM output on fan
 * when any LED channel is  >= 1% output
 */
void FanCtrl() {
  if (state_chan1 >= 164 || state_chan2 >= 164 || state_chan3 >= 164) {
  state_fan = 127;
}
 else {
  state_fan = 0;
  }
  SetPWM1A(state_fan);
}

/*
 * Function accepts serial commands via esp8266 for remote control/data acquisition 

void parseCommand(String com)
{
  String part1;
  String part2;
  time_t t = now(); // store the current time in time variable t
  int i = RTC.temperature();
  float celsius = i / 4.0;
  float fahrenheit = celsius * 9.0 / 5.0 + 32.0;

  part1 = com.substring(0, com.indexOf(" "));
  part2 = com.substring(com.indexOf(" ") + 1);

  if(part1.equalsIgnoreCase("1on")) {
    C1TurnOn = part2.toInt();
    Serial1.print(F("Channel 1 start time set: "));
    Serial1.println(part2.toInt());
  }
  else if (part1.equalsIgnoreCase("1off")) {
    C1TurnOff = part2.toInt();
    Serial1.print(F("Channel 1 end time set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("2on")) {
    C2TurnOn = part2.toInt();
    Serial1.print(F("Channel 2 start time set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("2off")) {
    C2TurnOff = part2.toInt();
    Serial1.print(F("Channel 2 end time set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("3on")) {
    C3TurnOn = part2.toInt();
    Serial1.print(F("Channel 3 start time set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("3off")) {
    C3TurnOff = part2.toInt();
    Serial1.print(F("Channel 3 end time set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("1day")) {
    C1DayState = part2.toInt();
    Serial1.print(F("Channel 1 max intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("1night")) {
    C1NightState = part2.toInt();
    Serial1.print(F("Channel 1 min intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("2day")) {
    C2DayState = part2.toInt();
    Serial1.print(F("Channel 2 max intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("2night")) {
    C2NightState = part2.toInt();
    Serial1.print(F("Channel 2 min intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("3day")) {
    C3DayState = part2.toInt();
    Serial1.print(F("Channel 3 max intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("3night")) {
    C3NightState = part2.toInt();
    Serial1.print(F("Channel 3 min intensity set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("1fade")) {
    C1FadeDuration = part2.toInt();
    Serial1.print(F("Channel 1 fade in/out duration set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("2fade")) {
    C2FadeDuration = part2.toInt();
    Serial1.print(F("Channel 2 fade in/out duration set: "));
    Serial1.println(part2.toInt());
  }
  else if(part1.equalsIgnoreCase("3fade")) {
    C3FadeDuration = part2.toInt();
    Serial1.print(F("Channel 3 fade in/out duration set: "));
    Serial1.println(part2.toInt());
  }  
  else if(part1.equalsIgnoreCase("time")) {
    Serial1.print(weekDay[weekday(t)]);
    Serial1.print(F(" "));
    Serial1.printf("%02d/%02d/%02d ", month(t), day(t), year(t));
    Serial1.printf("%02d:%02d:%02d ", hour(t), minute(t), second(t));
    Serial1.println();
  }
  else if(part1.equalsIgnoreCase("roomtemp")) {
    Serial1.println(fahrenheit);
  }
  else if(part1.equalsIgnoreCase("ledstatus")) {
    Serial1.print(F("Violet/Cool Blue ="));
    pwmPercent16s(state_chan1);
    Serial1.print(F(" "));
    Serial1.print(F("Cyan ="));
    pwmPercent16s(state_chan2);
    Serial1.print(F(" "));
    Serial1.print(F("White/Lime ="));
    pwmPercent16s(state_chan3);
    Serial1.println(F(" "));
  }
  else if(part1.equalsIgnoreCase("fanstatus")) {
    Serial1.print(F("Fan is "));
    if (state_fan > 0) {
    Serial1.println(F("on"));
    }
    else {
    Serial1.println(F("off "));
    }
    pwmPercent8(state_fan);
  }
  else if(part1.equalsIgnoreCase("moon")) {
    if (C1NightState == 0 && C2NightState == 0 && C3NightState == 0){
    C1NightState = part2.toInt();  
    C2NightState = part2.toInt();
    C3NightState = part2.toInt();
    Serial1.println(F("Moonlight enabled"));
      }
    else{
    C1NightState = 0;  
    C2NightState = 0;
    C3NightState = 0;
    Serial1.println(F("Moonlight disabled"));
      }
  }
  else {
    Serial1.println(F("COMMAND NOT FOUND"));
  }
}
*/

void setup() {
  Serial1.begin(115200);
  pinMode(ocs, OUTPUT);
  digitalWrite(ocs, HIGH);
  tft.begin();
  SD.begin(sd_cs);
  bmpDraw("Splash.bmp", 0, 0);
  delay(4000);
  setSyncProvider(RTC.get);
  //***setTime(hr,min,sec,day,month,yr)***
  //setTime(18, 03, 00, 17, 6, 2017);
  //RTC.set(now());
  tft.fillScreen(BLACK);
  tft.setTextSize(1);
  tft.setCursor(45,55);
  tft.println(F("FW 1.25"));
  delay(4000);
  tft.fillScreen(MINT);
  Wire.begin();
  
  T1PWMInit();
    SetPWM1Period(0xFF);
      SetPWM1A(0);
      SetPWM1B(0);
      SetPWM1C(0);

  T3PWMInit();
    SetPWM3Period(0x3FFF);
      SetPWM3A(0);
      SetPWM3B(0);
      SetPWM3C(0);
 }

void loop () {
  
  ctr = seconds_since_midnight();
  determine_state1();
  determine_state2();
  determine_state3();
  FanCtrl();
  if ((state_chan1 >0 || state_chan2 >0 || state_chan3 >0) && tftini == false) {
    tft.begin();
    tftini = true;
 }
  if (state_chan1 >0 || state_chan2 >0 || state_chan3 >0) {
    PrimaryInfoS();
 }
   else {
    tftini = false;
    digitalWrite(rst, LOW);
 }
}
/*

if(Serial1.available()) {
  
  char c = Serial1.read();

  if(c == '\n')
  {
    parseCommand(command);
    command = "";
  }
  else
  {
    command += c;
  }
 }
 }
*/
