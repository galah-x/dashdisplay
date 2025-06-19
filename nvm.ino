void reinit_NVM (void)
{
  Serial.println("Initializing NVM");
  
  // If tpInit is 'false', the key "nvsInit" does not yet exist therefore this
  //  must be our first-time run. We need to set up our Preferences namespace keys. So...
  dispPrefs.begin("dispPrefs", RW_MODE);       //  open it in RW mode.
  
  // The .begin() method created the "dispPrefs" namespace and since this is our
  //  first-time run we will create
  //  our keys and store the initial "factory default" values.

  dispPrefs.putString("ssid",      "unknown");      // network ssid
  dispPrefs.putString("password",  "unknown");  // network password

  dispPrefs.putULong("omt", 60);        // what defines an OLD message. in seconds.

  dispPrefs.putBool("nvsInit", true);            // Create the "already initialized"
  //  key and store a value.
  // The "factory defaults" are created and stored so...
  dispPrefs.end();                               // Close the namespace in RW mode and...
}


void load_operational_params(void)
{

  dispPrefs.begin("dispPrefs", RO_MODE);       // Open our namespace (or create it
                                               //  if it doesn't exist) in RO mode.
   // Retrieve the operational parameters from the namespace
   //  and save them into their run-time variables.
   dispPrefs.getString("ssid", ssid, ssid_maxlen); 
   dispPrefs.getString("password", password, ssid_maxlen); 

   old_message_time  = dispPrefs.getULong("omt");        // what defines an OLD message. in seconds.
   // All done. Last run state (or the factory default) is now restored.
   dispPrefs.end();                                      // Close our preferences namespace.
}
