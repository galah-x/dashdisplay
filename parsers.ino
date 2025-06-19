//    -*- Mode: c++     -*-
// emacs automagically updates the timestamp field on save
// my $ver =  'parsers for display  Time-stamp: "2025-06-19 10:14:24 john"';

// this is the parsers for app to run the display parser for the haflinger
// use tools -> board ->  ESP32 Dev module 


// this is for Serial 0, debug and writing NVM
// 
void parse_buf (char * in_buf)
{
  // Z  eraZe NV memory
  // Rf Read and print Field,
  //    where Field could be    
  //          RP\n   show the stored wifi password
  //          RS\n   show stored wifi ssid
  //          V\n    show software version
  // Wf Write Field to NVM,
  //    where Field could be (variable data to write is in lower case here hopefully fpr clarity. just Type the case you want however)
  //          WP=passsword\n   set wifi Password to "password"
  //          WS=ssid\n        set wifi Ssid  to "ssid
  //                        

  int8_t  address;
  uint8_t  cmd;
  int      value;
  uint8_t  mvalue[6]; // mac address
  float    fvalue;
  const uint8_t out_buf_len = 20;
  char     out_buf[out_buf_len];
  int      match ;
  int      i;
  int32_t soc;
  uint32_t time1;
  uint32_t time2;
  
  cmd = in_buf[0];
  
  switch (cmd)
    {
    case 'Z':
      reinit_NVM();
      load_operational_params();
      break;
      
    case 'R':
      switch (in_buf[1])
	{
	case 'S':
	  Serial.printf("SSID=%20s\n", ssid);
	  break;

	case 'P':
	  Serial.printf("PASSWORD=%20s\n", password);
	  break;
	  
	case 'V':
	  Serial.println(version);
	  break;
	}
      break;
      
    case 'W':
      switch (in_buf[1]) {
	
      case 'P' :
	// nope, I cannot make sscanf work for a char then a 0 terminated string. So do a manual scan
	//	Serial.printf("input to parser is %s len %d\n", in_buf, strlen(in_buf));
	strncpy(out_buf, in_buf+3,  20); // hop over WP= 
	//Serial.print("got len=");
	//Serial.print(strlen(out_buf));
	//Serial.print(" c1=");
	//Serial.print(in_buf+3);
	//Serial.print("= ");
	
	//Serial.print(" str=");
	//Serial.print(out_buf);
	//Serial.println("=");
	
	dispPrefs.begin("dispPrefs", RW_MODE);
	dispPrefs.putString("password",  out_buf);
	dispPrefs.end();
	load_operational_params();
	break;

      case 'S' :
	// nope, I cannot make sscanf work for a char then a 0 terminated string. So do a manual scan
	strncpy(out_buf, in_buf+3,  20); // hop over WS= 
	dispPrefs.begin("dispPrefs", RW_MODE);
	dispPrefs.putString("ssid",  out_buf);
	dispPrefs.end();
	load_operational_params();
	break;
	
	// end of 'W'
      }    
    }
}


/* check the serial interface to see if there are any characters, and process them if so */  

void do_serial_if_ready (void)
{
  int serial_byte;
  if (Serial.available())
    {
      serial_byte = Serial.read();
      if ((serial_byte != '\n') && (serial_buf_pointer < longest_record))
        {
          serial_buf[serial_buf_pointer] = serial_byte;  // write char to buffer if space is available
          serial_buf_pointer++;
        }    
      if (serial_byte == '\n') {
        serial_buf[serial_buf_pointer] = (uint8_t) 0;  // write string terminator to buffer
        if (serial_buf_pointer >= 1) {                 // at least a command letter
          parse_buf(serial_buf);      // parse the buffer if at least one char in it.
          // Serial.print(return_buf);
        }
        serial_buf_pointer = 0;
      }
    }
}

// I have a udp packet in. Parse the stuff I care about out of it.
void parse_udp (void)
{
  // see what type of packet it is.
  uint16_t msg_type = fetch_le_uint16(udp_msg_type_offset);
  //  Serial.printf("got msg type %x\n", msg_type);
  if (msg_type == batrium_msg_5732)
    {
      // Serial.println("got 5732");
      parse_udp_5732();
      // new_data just means a 5732 (the more frequent one) packet has arrived. I don't bother
      // to check if the data I display is actually different. This reduces display flicker to acceptable
      // to me levels.  
      new_data = true;
    }
  else
    if (msg_type == batrium_msg_3233)
      {
	parse_udp_3233();
      }
}


/* parse the variables I want from a Batrium UDP message type 5732 System Discovery Information */
void parse_udp_5732 (void)
{
  dd_data.pack_v     = fetch_le_uint16(dd_offset_5732.pack_v);
  dd_data.pack_i     = fetch_le_float(dd_offset_5732.pack_v);
  dd_data.pack_soc   = ( uint8_t )  udp_buffer[dd_offset_5732.pack_soc];
  dd_data.cell_min_v = fetch_le_uint16(dd_offset_5732.cell_min_v);
  dd_data.cell_max_v = fetch_le_uint16(dd_offset_5732.cell_max_v);
  dd_data.cell_min_t = ((uint8_t) udp_buffer[dd_offset_5732.cell_min_t] )- 40;
  //Serial.printf("packv=%d packi=%d soc=%d minv=%d, maxv=%d mint=%d\n",
  //		 dd_data.pack_v, dd_data.pack_i,
  //		 dd_data.pack_soc, dd_data.cell_min_v,
  //		 dd_data.cell_max_v, dd_data.cell_min_t);
		 
}  

/* parse the variables I want from a Batrium UDP message type 3233 System Discovery Message */
void parse_udp_3233 (void)
{
  dd_data.cell_max_t = ((uint8_t) udp_buffer[dd_offset_3233.cell_max_t]) -40;
}  

// fetch littleendian uint16 from a batrium sourced buffer carried over udp
uint16_t fetch_le_uint16 (uint8_t offset)
{
  uint16_t val;
  // batrium uses little endian buffer data arrangement 
  val = (udp_buffer[offset + 1] << 8) + udp_buffer[offset] ; 
  return val;
}


// not at all sure this works. Certainly doesn't work right when the test app is run on an intel PC.
// do note the perl caveats on float formats being device specific..
// but as I intend to use it from an ESP sourse to an ESP32 sink, its worth testing on the real thing.

// fetch littleendian 4 byte float from a batrium sourced buffer carried over udp
float fetch_le_float (uint8_t offset)
{
  float val;
  // batrium uses little endian buffer data arrangement 
  val = (udp_buffer[offset + 3] << 24) + (udp_buffer[offset + 2] << 16)+ (udp_buffer[offset + 1] << 8) + udp_buffer[offset] ; 
  return val;
}


