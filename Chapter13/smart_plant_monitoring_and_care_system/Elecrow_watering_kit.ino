#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
//#include <RTClib.h>

//U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
//RTC_DS1307 RTC;

// set all moisture sensors PIN ID
int sensor1 = 36;
int sensor2 = 39;
int sensor3 = 34;
int sensor4 = 35;

// set water relays
const int relay1 = 26;
const int relay2 = 27;
const int relay3 = 14;
const int relay4 = 13;
const int pump_relay = 32; //ADC1 CH4

// declare moisture values
int moisture1_value = 0 ;
int moisture2_value = 0;
int moisture3_value = 0;
int moisture4_value = 0;

const int low_trigger_level = 30;
const int high_trigger_level = 55;
const int sensor_min_value = 1200;
const int sensor_max_value = 3800;
const int no_sensor_value = 800;

bool relay_on = LOW;
bool relay_off = HIGH;


// set button
//int button = 33;
//int button = 12;

int pump_state = 0;
int relay1_state = 0;
int relay2_state = 0;
int relay3_state = 0;
int relay4_state = 0;


void setup()
{
  //Wire.begin();
  //RTC.begin();
  Serial.begin(115200);
  u8g2.begin();
  // declare relay as output
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(pump_relay, OUTPUT);

}

void loop()
{
  // read the value from the moisture sensors:
  read_sensor_value();
  // check whether need to start or stop plant watering
  plant_watering();

  u8g2.firstPage();
  do
  {
    drawMoisture();
    drawPlant();
  } while ( u8g2.nextPage() );
  delay (5000);
}

//Set moisture value
void read_sensor_value()
{
  // After analogRead the specific sensor, we compare the result
  // if < no_sensor_value, we detect no sensor present, give 999 as the result value
  // if < sensor_min_value then result value is 99;
  // if > sensor_max_value then result value is 0;
  // if sensor reading value between min_value and max_value, then map the reading result
  // within the min and max value, using linear projection with range from 0 to 99
  float value1 = analogRead(sensor1);
  if(value1 < no_sensor_value)
    moisture1_value = 999;
  else if(value1 < sensor_min_value)
    moisture1_value = 99;
  else if(value1 > sensor_max_value)
    moisture1_value = 0;
  else
    moisture1_value = map(value1, 1200, 3800, 99, 0);
  float value2 = analogRead(sensor2);
  if(value2 < no_sensor_value)
    moisture2_value = 999;
  else if(value2 < sensor_min_value)
    moisture2_value = 99;
  else if(value2 > sensor_max_value)
    moisture2_value = 0;
  else
    moisture2_value = map(value2, 1200, 3800, 99, 0);
  float value3 = analogRead(sensor3);
  if(value3 < no_sensor_value)
    moisture3_value = 999;
  else if(value3 < sensor_min_value)
    moisture3_value = 99;
  else if(value3 > sensor_max_value)
    moisture3_value = 0;
  else
    moisture3_value = map(value3, 1200, 3800, 99, 0);
  float value4 = analogRead(sensor4);
  if(value4 < no_sensor_value)
    moisture4_value = 999;
  else if(value4 < sensor_min_value)
    moisture4_value = 99;
  else if(value4 > sensor_max_value)
    moisture4_value = 0;
  else
    moisture4_value = map(value4, 1200, 3800, 99, 0);
  // Serial print the result to the console
  Serial.println("");
  Serial.print("S1 = "); Serial.print(value1); Serial.print(" - "); Serial.print(moisture1_value);
  Serial.print(", S2 = "); Serial.print(value2); Serial.print(" - "); Serial.print(moisture2_value);
  Serial.print(", S3 = "); Serial.print(value3); Serial.print(" - "); Serial.print(moisture3_value);
  Serial.print(", S4 = "); Serial.print(value4); Serial.print(" - "); Serial.println(moisture4_value);
}

void plant_watering()
{
  // Compare if moisture value lower than low_trigger level
  //    if yes, then switch on the appropriate relay to the water valve,
  //        and check if the pump relay is already on, if not then switch the pump on
  // Next compare if the moisture value higher than high_trigger_level
  //    if yes, then switch off the appropriate relay to the water valve,
  //        and check if all the water valve relay are off, if yes then switch off the pump
  if (moisture1_value < low_trigger_level)
  {
    digitalWrite(relay1, relay_on);
    relay1_state = 1;
    delay(20);
    if (pump_state == 0)
    {
      digitalWrite(pump_relay, relay_on);
      pump_state = 1;
      delay(20);
    }
  }
  else if (moisture1_value > high_trigger_level)
  {
    digitalWrite(relay1, relay_off);
    relay1_state = 0;
    delay(20);
    if ((relay1_state == 0) && (relay2_state == 0) && (relay3_state == 0) && (relay4_state == 0))
    {
      digitalWrite(pump_relay, relay_off);
      pump_state = 0;
      delay(20);
    }
  }

  if (moisture2_value < low_trigger_level)
  {
    digitalWrite(relay2, relay_on);
    relay2_state = 1;
    delay(20);
    if (pump_state == 0)
    {
      digitalWrite(pump_relay, relay_on);
      pump_state = 1;
      delay(20);
    }
  }
  else if (moisture2_value > high_trigger_level)
  {
    digitalWrite(relay2, relay_off);
    relay2_state = 0;
    delay(20);
    if ((relay1_state == 0) && (relay2_state == 0) && (relay3_state == 0) && (relay4_state == 0))
    {
      digitalWrite(pump_relay, relay_off);
      pump_state = 0;
      delay(20);
    }
  }

  if (moisture3_value < low_trigger_level)
  {
    digitalWrite(relay3, relay_on);
    relay3_state = 1;
    delay(20);
    if (pump_state == 0)
    {
      digitalWrite(pump_relay, relay_on);
      pump_state = 1;
      delay(20);
    }
  }
  else if (moisture3_value > high_trigger_level)
  {
    digitalWrite(relay3, relay_off);
    relay3_state = 0;
    delay(20);
    if ((relay1_state == 0) && (relay2_state == 0) && (relay3_state == 0) && (relay4_state == 0))
    {
      digitalWrite(pump_relay, relay_off);
      pump_state = 0;
      delay(20);
    }
  }

  if (moisture4_value < low_trigger_level)
  {
    digitalWrite(relay4, relay_on);
    relay4_state = 1;
    delay(20);
    if (pump_state == 0)
    {
      digitalWrite(pump_relay, relay_on);
      pump_state = 1;
      delay(20);
    }
  }
  else if (moisture4_value > high_trigger_level)
  {
    digitalWrite(relay4, relay_off);
    relay4_state = 0;
    delay(50);
    if ((relay1_state == 0) && (relay2_state == 0) && (relay3_state == 0) && (relay4_state == 0))
    {
      digitalWrite(pump_relay, relay_off);
      pump_state = 0;
      delay(50);
    }
  }
}

// bit map picture of good plant
static const unsigned char bitmap_good_plant[] U8X8_PROGMEM = {

  0x00, 0x42, 0x4C, 0x00, 0x00, 0xE6, 0x6E, 0x00, 0x00, 0xAE, 0x7B, 0x00, 0x00, 0x3A, 0x51, 0x00,
  0x00, 0x12, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00,
  0x00, 0x04, 0x60, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00, 0x08, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0xE0, 0x0F, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xC1, 0x00, 0x00, 0x0E, 0x61, 0x00,
  0x00, 0x1C, 0x79, 0x00, 0x00, 0x34, 0x29, 0x00, 0x00, 0x28, 0x35, 0x00, 0x00, 0x48, 0x17, 0x00,
  0x00, 0xD8, 0x1B, 0x00, 0x00, 0x90, 0x1B, 0x00, 0x00, 0xB0, 0x09, 0x00, 0x00, 0xA0, 0x05, 0x00,
  0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00
};

// bit map picture of bad plant
static const unsigned char bitmap_bad_plant[] U8X8_PROGMEM = {
  0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x0D, 0x00, 0x00, 0xA0, 0x0F, 0x00,
  0x00, 0x20, 0x69, 0x00, 0x00, 0x10, 0x78, 0x02, 0x00, 0x10, 0xC0, 0x03, 0x00, 0x10, 0xC0, 0x03,
  0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0xC0, 0x00, 0x00, 0x30, 0x60, 0x00,
  0x00, 0x60, 0x30, 0x00, 0x00, 0xC0, 0x1F, 0x00, 0x00, 0x60, 0x07, 0x00, 0x00, 0x60, 0x00, 0x00,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC7, 0x1C, 0x00,
  0x80, 0x68, 0x66, 0x00, 0xC0, 0x33, 0x7B, 0x00, 0x40, 0xB6, 0x4D, 0x00, 0x00, 0xE8, 0x06, 0x00,
  0x00, 0xF0, 0x03, 0x00, 0x00, 0xE0, 0x00, 0x00
};

//Style the flowers     bitmap_bad: bad flowers     bitmap_good:good  flowers
void drawPlant()
{
  if (moisture1_value < low_trigger_level)
  {
    u8g2.drawXBMP(0, 0, 32, 30, bitmap_bad_plant);
  }
  else
  {
    if(moisture1_value != 999)
      u8g2.drawXBMP(0, 0, 32, 30, bitmap_good_plant);
  }
  if (moisture2_value < low_trigger_level)
  {
    u8g2.drawXBMP(32, 0, 32, 30, bitmap_bad_plant);
  }
  else
  {
    if(moisture2_value != 999)
      u8g2.drawXBMP(32, 0, 32, 30, bitmap_good_plant);
  }
  if (moisture3_value < low_trigger_level)
  {
    u8g2.drawXBMP(64, 0, 32, 30, bitmap_bad_plant);
  }
  else
  {
    if(moisture3_value != 999)
      u8g2.drawXBMP(64, 0, 32, 30, bitmap_good_plant);
  }
  if (moisture4_value < low_trigger_level)
  {
    u8g2.drawXBMP(96, 0, 32, 30, bitmap_bad_plant);
  }
  else
  {
    if(moisture4_value != 999)
      u8g2.drawXBMP(96, 0, 32, 30, bitmap_good_plant);
  }

}


void drawMoisture()
{
  int A = 0;
  int B = 0;
  int C = 64;
  int D = 96;
  char moisture1_temp[5] = {0};
  char moisture2_temp[5] = {0};
  char moisture3_temp[5] = {0};
  char moisture4_temp[5] = {0};
  read_sensor_value();
  if(moisture1_value != 999)
    itoa(moisture1_value, moisture1_temp, 10);
  if(moisture2_value != 999)
    itoa(moisture2_value, moisture2_temp, 10);
  if(moisture3_value != 999)
    itoa(moisture3_value, moisture3_temp, 10);
  if(moisture4_value != 999)
    itoa(moisture4_value, moisture4_temp, 10);
  u8g2.setFont(u8g2_font_8x13_tr);
  u8g2.setCursor(9, 60);
  u8g2.print("S1  S2  S3  S4");
  if (moisture1_value < 10)
  {
    u8g2.drawStr(A + 14, 45, moisture1_temp);
  }
  else if (moisture1_value < 100)
  {
    u8g2.drawStr(A + 6, 45, moisture1_temp);
  }
  else
  {
    u8g2.drawStr(A + 2, 45, moisture1_temp);
  }
  if(moisture1_value != 999){
    u8g2.setCursor(A + 23, 45 );
    u8g2.print("%");
  }
  if (moisture2_value < 10)
  {
    u8g2.drawStr(B + 46, 45, moisture2_temp);
  }
  else if (moisture2_value < 100)
  {
    u8g2.drawStr(B + 37, 45, moisture2_temp);
  }
  else
  {
    u8g2.drawStr(B + 32, 45, moisture2_temp);
  }
  if(moisture2_value != 999){
    u8g2.setCursor(B + 54, 45);
    u8g2.print("%");
  }
  if (moisture3_value < 10)
  {
    u8g2.drawStr(C + 14, 45, moisture3_temp);
  }
  else if (moisture3_value < 100)
  {
    // u8g2.setCursor(C + 5, 45);
    u8g2.drawStr(C + 5, 45, moisture3_temp);
  }
  else
  {
    u8g2.drawStr(C + 2, 45, moisture3_temp);
  }
  if(moisture3_value != 999){
    u8g2.setCursor(C + 23, 45);
    u8g2.print("%");
  }
  if (moisture4_value < 10)
  {
    u8g2.drawStr(D + 14, 45, moisture4_temp);
  }
  else if (moisture4_value < 100)
  {
    u8g2.drawStr(D + 5, 45, moisture4_temp);
  }
  else
  {
    u8g2.drawStr(D + 2, 45, moisture4_temp);
  }
  if(moisture4_value != 999){
    u8g2.setCursor(D + 23, 45);
    u8g2.print("%");
  }
}
