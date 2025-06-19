//    -*- Mode: c++     -*-
// emacs automagically updates the timestamp field on save
// my $ver =  'data and variables for display  Time-stamp: "2025-06-19 10:18:26 john"';


#include <Preferences.h>  // the NV memory interface
#include <Wire.h>
#include <SPI.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#include <stdio.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>

#define BLACK 0
#define WHITE 1



#define DEBUG

// for preferences
#define RW_MODE false
#define RO_MODE true

const uint8_t ssid_maxlen = 20;
char password[ssid_maxlen];
char ssid[ssid_maxlen];
uint32_t old_message_time;

// serial is used for debug and for programming NVM
const uint8_t longest_record = 24;  //   worst is display comand WD1=12345678901234567890
const uint8_t record_size = longest_record + 2;    //  char char = nnnnn \n
char serial_buf[record_size];
uint8_t serial_buf_pointer;

const char * version = "DISPLAY 19 Jun 2025 Rev1";

Preferences dispPrefs;  // NVM structure
// these will be initialized from the NV memory

/* IOs  definitions */
// only IO is for SPI, which seems to manage its IO just fine.

// The first 2 are the real ESP32 SPI pins. They are shared. The CS pins are randim GPIOs
const uint8_t SHARP_SCK    = 14;
const uint8_t SHARP_MOSI   = 13;
const uint8_t SHARP_CS     = 5;   //  CS is display specific. MOSI and SCK are shared. No MISO.
const uint8_t SHARP_2CS    = 4;


// the sharp display I use display. There are 2, in landscape. display1 is immediately above display2
const int display_h_pixels = 400;
const int display_v_pixels = 240;

const uint8_t udp_msg_type_offset = 1; // os 0 == ':'. offset 1 is the msg type littleendian that Batrium seems to use.

const uint16_t batrium_port = 18542 ; // from Batrium doc

char udp_buffer[255];
WiFiUDP udp;

bool new_data = 1;

struct dd_data_str
{
  uint16_t pack_v ; // in 10's of mV
  float    pack_i ; // + is charge
  uint8_t  pack_soc    ; // in 0.5% steps with 5% offset. ranges from -5% to 105% in 0.5% steps.
  uint16_t cell_min_v ; // in mV. Ranges from 0 to 6.500V
  uint16_t cell_max_v ; // ditto
  int8_t cell_min_t ; // in degrees C, no offset.  Ranges from -40C to 125C
  int8_t cell_max_t ; // ditto
} dd_data ;


const uint16_t batrium_msg_5732 = 0x5732;
// items of interest in msg 5732 
struct dd_offset_5732_str
{
  const uint8_t pack_v ; 
  const uint8_t pack_i ; 
  const uint8_t pack_soc ; 
  const uint8_t cell_min_v ; 
  const uint8_t cell_max_v ; 
  const uint8_t  cell_min_t ;
}  ;
struct dd_offset_5732_str  dd_offset_5732 = { 42, 44, 41, 31, 33, 37};

const uint16_t batrium_msg_3233 = 0x3233;
struct dd_offset_3233_str
{
  const uint8_t cell_max_t ;
}  ;
struct dd_offset_3233_str  dd_offset_3233 = { 19};


// update times

const uint32_t display_update_millis = 500;
uint32_t last_display_update_time;
const uint32_t connect_check_millis = 10000;
uint32_t last_connect_check_time;

// display items disp1
// this is where the various items appear on the displays.
struct d1_xy_str
{
  const uint16_t pack_v_x;
  const uint16_t pack_v_y;
  const uint16_t pack_i_x;
  const uint16_t pack_i_y;
  const uint16_t pack_soc_x;
  const uint16_t pack_soc_y;
};
struct d1_xy_str d1_xy = {0,0,  0, 80,   0,160};

struct d2_xy_str
{
  const uint16_t cell_min_v_x;
  const uint16_t cell_min_v_y;
  const uint16_t cell_max_v_x;
  const uint16_t cell_max_v_y;
  const uint16_t cell_min_t_x;
  const uint16_t cell_min_t_y;
  const uint16_t cell_max_t_x;
  const uint16_t cell_max_t_y;
};
struct d2_xy_str d2_xy = {0,0,  0,60,   0,120,  0,180};

  
