//    -*- Mode: c++     -*-
// my $ver =  'display  Time-stamp: "2025-06-20 16:41:08 john"';

/* All code in this directory written by me and is BSD licensed.
   John Sheahan June 2025
/*********************************************************************
This is the second cut of the dash display for the haflinger. This time its going to
connect to the AP set up by the batrium BMS, 

It uses parts of:
   the adafruit example sketch for their Monochrome SHARP Memory Displays
   Written by Limor Fried/Ladyada  for Adafruit Industries.
   BSD license, check license.txt for more information
   All text above, must be included in any redistribution
   However I think the only thing I have replicated is calls to Adafruits libraries.
   I don't claim to own the call lines.
   
   This thing contains 2 adafruit sharp displays, just displays what remote sensors send to it.  
    (Did I get these from Core electronics or adafruit? not sure now. Core are a local reseller)

   Various code fragments from my mower upgrade project too. All my own work I think.
   And from my Cattleyards gadget.

POSSIBLE extensions
  Consider a switch to control the fans in the battery boxes. Thinking switching them off for a big puddle might make sense.
  Consider a rotary encoder to control more general things. Apparently I can treat some of the flash as EE. Although some ee might be nice if I want say a distance or maybe hour meter. 

*********************************************************************/
#include "display_data.h"

// Set the IO pins used, and size of the displays

Adafruit_SharpMem display1(SHARP_SCK, SHARP_MOSI, SHARP_CS, display_h_pixels, display_v_pixels);
Adafruit_SharpMem display2(SHARP_SCK, SHARP_MOSI, SHARP_2CS, display_h_pixels, display_v_pixels);

void setup(void)
{
  
  Serial.begin(115200);
  Serial.println(version);
  delay(50);

  // start & clear the displays
  display1.begin();
  display1.clearDisplay();
  display1.setRotation(0);

  display2.begin();
  display2.clearDisplay();
  display2.setRotation(0);
  
  Serial.println("init NVM");
  // initialize NVM  
   dispPrefs.begin("dispPrefs", RO_MODE);     // Open our namespace (or create it if it doesn't exist)
   bool tpInit = dispPrefs.isKey("nvsInit"); // Test for the existence of the "already initialized" key.
   dispPrefs.end();                          // close the namespace in RO mode
   if (tpInit == false) 
     reinit_NVM();                          // reinitialize the nvm structure if magik key was missing 
   
   // load local variables from NVM
   load_operational_params();                // initialize RAM variables from NVM

   // init some general variables and IOs 
   serial_buf_pointer = 0;
   last_display_update_time = millis();
   last_connect_check_time = millis();

   initWiFi();
   Serial.println("Connected to wifi");
   udp.begin(batrium_port);
  
   // Enable WiFi power-saving workaround for UDP broadcast.
   // wifi_set_ps(WIFI_PS_MIN_MODEM);

   // Enable broadcast on the socket
   //   int broadcastEnable = 1;
   //int sock = udp.getSocket(); // Get the underlying socket descriptor
   //setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

   Serial.println("done setup");
   
}

void loop(void) 
{
  char outbuf[20];
  
  // update display(s) if display variables have changed.
  // refresh display at least once per second, 
  if (millis() >= (last_display_update_time + display_update_millis) )
    {
      if (new_data)
	{
	  display1.clearDisplay();
	  display1.setTextSize(6);
	  display1.setTextColor(BLACK);
	  // pack voltage
	  display1.setCursor(d1_xy.pack_v_x, d1_xy.pack_v_y);
	  sprintf(outbuf, "%3d.%02dV", dd_data.pack_v/100, dd_data.pack_v % 100);
	  display1.print(outbuf);

	  // pack I
	  display1.setCursor(d1_xy.pack_i_x, d1_xy.pack_i_y);
	  sprintf(outbuf, "%4.3fA", dd_data.pack_i/1000.0f);
	  display1.print(outbuf);

	  // SOC
	  display1.setCursor(d1_xy.pack_soc_x, d1_xy.pack_soc_y);
	  sprintf(outbuf, "%3d.%1d%%", (dd_data.pack_soc -10)/2, 5*(dd_data.pack_soc % 2));
	  display1.print(outbuf);
	}
	  
      display1.refresh();

      if (new_data)
	{
	  display2.clearDisplay();
	  display2.setTextSize(5);
	  display2.setTextColor(BLACK);
	  display2.setCursor(d2_xy.cell_min_v_x, d2_xy.cell_min_v_y);
	  sprintf(outbuf, "min=%1d.%03dV", dd_data.cell_min_v/1000, dd_data.cell_min_v % 1000);
	  display2.print(outbuf);
	  
	  display2.setCursor(d2_xy.cell_max_v_x, d2_xy.cell_max_v_y);
	  sprintf(outbuf, "max=%1d.%03d", dd_data.cell_max_v/1000, dd_data.cell_max_v % 1000);
	  display2.print(outbuf);
	  
	  display2.setCursor(d2_xy.cell_min_t_x, d2_xy.cell_min_t_y);
	  sprintf(outbuf, "min=%2dC", dd_data.cell_min_t);
	  display2.print(outbuf);
	  
	  display2.setCursor(d2_xy.cell_max_t_x, d2_xy.cell_max_t_y);
	  sprintf(outbuf, "max=%2dC", dd_data.cell_max_t);
	  display2.print(outbuf);
	  new_data = false;
	}
      
      display2.refresh();
      last_display_update_time = millis();
    }
  // service serial character if any available.
  do_serial_if_ready();
  
  // check for a packet
  int packetSize = udp.parsePacket();
  if (packetSize)
    {
      uint8_t len = udp.read(udp_buffer, 255);
      if (len >= 3) 
	{
	  parse_udp();
	}
    }  
  
  
  // if WiFi is down, try reconnecting
  if ((WiFi.status() != WL_CONNECTED) && (millis() >= (last_connect_check_time + connect_check_millis))) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    last_connect_check_time = millis();
  }
}    

    


void initWiFi( void)
{
  uint8_t retries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while ((WiFi.status() != WL_CONNECTED) && (retries < 10)) {
    Serial.print('.');
    delay(1000);
    retries++;
  }
  Serial.println(WiFi.localIP());
}
