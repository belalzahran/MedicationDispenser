
#include <main.h>

// Function declarations

void setup() 
{

    Serial.begin(9600);   

    EEPROM.begin(14);

    pinMode(medication_dispense_button, INPUT);
    pinMode(red_led, OUTPUT);
    pinMode(green_led, OUTPUT);  

    stepperMotor.setSpeed(2);
                            

    WiFi.begin(ssid, password);                         // start WiFi interface
    Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));     // print SSID to the serial interface for debugging
  
    while (WiFi.status() != WL_CONNECTED) {             // wait until WiFi is connected
      delay(1000);
      Serial.print(".");
    }

    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    if (Firebase.beginStream(firebaseStream, "/medication")) {
        Firebase.setStreamCallback(firebaseStream, streamCallback, streamTimeoutCallback);
        Serial.println("Stream started successfully");
    } else {
        Serial.println("Failed to start stream");
        Serial.println("REASON: " + firebaseStream.errorReason());
    }


    Serial.print("Connected to network with IP address: ");
    Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
   




    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();

    longest_streak = EEPROM.read(longest_streak_eepromAddress);
    medication_streak = EEPROM.read(medication_streak_eepromAddress);
    medication_taken_today = EEPROM.read(medication_taken_today_eepromAddress);
    missed_medication_days = EEPROM.read(missed_medication_days_eepromAddress);
    firstRun = EEPROM.read(firstRun_eeprom_address);
    getTimeFromEEPROM(&lastTime, lastTime_eeprom_address);


    if (medication_taken_today){
      digitalWrite(red_led, HIGH);
    }
    else{
      digitalWrite(green_led, HIGH);
    }

    // EEPROM.write(longest_streak_eepromAddress, 0);
    // EEPROM.write(medication_streak_eepromAddress, 0);
    // EEPROM.write(missed_medication_days_eepromAddress, 0);
    // EEPROM.write(medication_taken_today_eepromAddress, 0);

    // EEPROM.commit();

}

void loop() 
{

    unsigned long currentTime = millis();


    struct tm timeinfo;
     if (!getLocalTime(&timeinfo)) {  
        Serial.println("Failed to obtain time");
        return;
    }

    if (firstRun) 
    {
        lastTime = timeinfo; 
        firstRun = false; // Reset the flag for next iterations
        EEPROM.write(firstRun_eeprom_address, firstRun);
        EEPROM.commit();
    }

    // Choose your interval: hour or minute
    // bool timeChanged = (timeinfo.tm_mday != lastTime.tm_mday)
    // bool timeChanged =  (timeinfo.tm_hour != lastTime.tm_hour) 
    bool timeChanged = timeinfo.tm_min != lastTime.tm_min;

    if (timeChanged) 
    {
        printLocalTime();
        
        Serial.print("medication_streak:");
        Serial.println(String(medication_streak));

        Serial.print("longest_streak:");
        Serial.println(String(longest_streak));

        Serial.print("missed_days:");
        Serial.println(String(missed_medication_days));
        Serial.print("\n\n\n\n");



        if (medication_taken_today)
        {
            Serial.println("Day reset");
            digitalWrite(red_led, LOW);
            digitalWrite(green_led, HIGH);
            medication_taken_today = 0;
            
            sendJson("medication_taken", String(medication_taken_today));

            EEPROM.write(medication_taken_today_eepromAddress, medication_taken_today);
        
        }
        else{

            Serial.println("Day Reset, Streak restarting");
            medication_streak = 0;
            missed_medication_days++;
            
            sendJson("missed_days", String(missed_medication_days));
            sendJson("medication_streak", String(medication_streak));

            EEPROM.write(medication_streak_eepromAddress, medication_streak);
            EEPROM.write(missed_medication_days_eepromAddress, missed_medication_days);
          
        }
        

        lastTime = timeinfo;
        storeTimeInEEPROM(lastTime, lastTime_eeprom_address);
        
    }

    if (digitalRead(medication_dispense_button) == HIGH)
    {
        dispense_requested(); 
    }
    


    // if (currentTime % 10000 == 0){

    //   Serial.print("\n\n\nmedication_streak:");
    //   Serial.println(String(medication_streak));

    //   Serial.print("longest_streak:");
    //   Serial.println(String(longest_streak));

    //   Serial.print("missed_days:");
    //   Serial.println(String(missed_medication_days));



    // }




}



