// main.h

#ifndef MAIN_H
#define MAIN_H

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Stepper.h>
#include <EEPROM.h>

// Firebase project credentials
#define FIREBASE_HOST "https://medicinedispenser-c73da-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "PHSUdXCrMCzpL7wb9vqQoCmjuQuMfPYcNoo6pKth"

// Function declarations
FirebaseData firebaseData;
FirebaseData firebaseStream;
FirebaseAuth auth;
FirebaseConfig config;


const char* ssid = "Z6";
const char* password = "Hajj2016";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;
const int   daylightOffset_sec = 3600;

int medication_streak_eepromAddress = 0;
int longest_streak_eepromAddress = 1;
int missed_medication_days_eepromAddress = 2;
int medication_taken_today_eepromAddress = 3;
int firstRun_eeprom_address = 4;
int lastTime_eeprom_address = 5;

int medication_taken_today = 0;
int medication_streak = 0;
int missed_medication_days = 0;
int longest_streak = 0;
static struct tm lastTime; // Store the last recorded time
static bool firstRun = true; // Flag for the first run
int red_led = 14;
int green_led = 12;
int medication_dispense_button = 13;

Stepper stepperMotor = Stepper(stepsPerRevolution, 26, 33, 25, 32);
const int stepsPerRevolution = 2048;


void streamCallback(StreamData data);
void streamTimeoutCallback(bool timeout);
void printLocalTime();
void dispense_requested();
void reset_requested();
void sendJson(String l_type, String l_value);
void serializeTime(struct tm timeinfo, uint8_t* buffer);
void deserializeTime(struct tm* timeinfo, uint8_t* buffer);
void storeTimeInEEPROM(struct tm timeinfo, int address);
void getTimeFromEEPROM(struct tm* timeinfo, int address);







void streamCallback(StreamData data) {
    Serial.println("Stream data received...");
    Serial.println(data.streamPath());
    Serial.println(data.dataPath());
    Serial.println(data.dataType());

    if (data.dataPath().startsWith("/commands")) 
    {
        FirebaseJson& json = data.jsonObject();
        FirebaseJsonData jsonData;
        String type, value;
        
        if (json.get(jsonData, "type") && jsonData.success) {
            type = jsonData.to<String>();
        }

        if (json.get(jsonData, "value") && jsonData.success) {
            value = jsonData.to<String>();
        }

        if (type == "dispense" && value == "1")
        {
            dispense_requested();
       
        } 
        else if (type == "reset" && value == "1")
        {
            reset_requested();
        }

    }

}

void streamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println("Stream timed out, reconnecting...");
        if (!Firebase.beginStream(firebaseData, "/medication")) {
            Serial.println("Could not begin stream");
            Serial.println("REASON: " + firebaseData.errorReason());
        }
    }
}

void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void dispense_requested()
{
       
        if (!medication_taken_today)
        {
                stepperMotor.step(stepsPerRevolution/8);
                medication_taken_today = 1;
                medication_streak++;
                
                Serial.println("Medication Taken");

                digitalWrite(red_led, HIGH);
                digitalWrite(green_led, LOW);
                sendJson("medication_taken", String(medication_taken_today));
                sendJson("medication_streak", String(medication_streak)); // Send the updated streak to the client

                EEPROM.write(medication_streak_eepromAddress, medication_streak);
                EEPROM.write(medication_taken_today_eepromAddress, medication_taken_today);
                EEPROM.commit();

                if (medication_streak >= longest_streak)
                {
                  longest_streak = medication_streak;
                  
                  sendJson("longest_streak", String(longest_streak));

                  EEPROM.write(longest_streak_eepromAddress, longest_streak);
                  EEPROM.commit();
                }
    }

}

void reset_requested()
{
        medication_taken_today = 0;
        medication_streak = 0;
        missed_medication_days = 0;
        longest_streak = 0;
        firstRun = true;

        EEPROM.write(longest_streak_eepromAddress, longest_streak);
        EEPROM.write(medication_streak_eepromAddress, medication_streak);
        EEPROM.write(missed_medication_days_eepromAddress, missed_medication_days);
        EEPROM.write(medication_taken_today_eepromAddress, medication_taken_today);
        EEPROM.write(firstRun_eeprom_address, firstRun);

        EEPROM.commit();

        digitalWrite(red_led, LOW);
        digitalWrite(green_led, HIGH);

        sendJson("medication_streak", String(medication_streak));
        sendJson("missed_days", String(missed_medication_days));
        sendJson("longest_streak", String(longest_streak));
        sendJson("medication_taken", String(medication_taken_today));
}

void sendJson(String l_type, String l_value) {
    FirebaseJson json;
    json.set(l_type, l_value);
    String path = "/medication/data/" + l_type;
    if (Firebase.set(firebaseData, path, l_value)) {
        Serial.println("\nData set successfully\n");
    } else {
        Serial.println("Failed to set data");
        Serial.print("REASON: ");
        Serial.println(firebaseData.errorReason());
    }
}

void serializeTime(struct tm timeinfo, uint8_t* buffer) {
    buffer[0] = timeinfo.tm_sec;
    buffer[1] = timeinfo.tm_min;
    buffer[2] = timeinfo.tm_hour;
    buffer[3] = timeinfo.tm_mday;
    buffer[4] = timeinfo.tm_mon;
    buffer[5] = timeinfo.tm_year - 1900; // tm_year is years since 1900
    buffer[6] = timeinfo.tm_wday;
    buffer[7] = timeinfo.tm_yday;
    buffer[8] = timeinfo.tm_isdst;
}

void deserializeTime(struct tm* timeinfo, uint8_t* buffer) {
    timeinfo->tm_sec = buffer[0];
    timeinfo->tm_min = buffer[1];
    timeinfo->tm_hour = buffer[2];
    timeinfo->tm_mday = buffer[3];
    timeinfo->tm_mon = buffer[4];
    timeinfo->tm_year = buffer[5] + 1900;
    timeinfo->tm_wday = buffer[6];
    timeinfo->tm_yday = buffer[7];
    timeinfo->tm_isdst = buffer[8];
}

void storeTimeInEEPROM(struct tm timeinfo, int address) {
    uint8_t buffer[9];
    serializeTime(timeinfo, buffer);
    for (int i = 0; i < 9; i++) {
        EEPROM.write(address + i, buffer[i]);
    }
    EEPROM.commit();
}

void getTimeFromEEPROM(struct tm* timeinfo, int address) {
    uint8_t buffer[9];
    for (int i = 0; i < 9; i++) {
        buffer[i] = EEPROM.read(address + i);
    }
    deserializeTime(timeinfo, buffer);
}


#endif // MAIN_H