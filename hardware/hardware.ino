//##################################################################################################################
//##                                      ELET2415 DATA ACQUISITION SYSTEM CODE                                   ##
//##                                                                                                              ##
//##################################################################################################################

 

// IMPORT ALL REQUIRED LIBRARIES
#include <rom/rtc.h>

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <ArduinoJson.h>


//IMPORT IMAGES
#include "lockclose.h"
#include "lockopen.h"


#ifndef _WIFI_H 
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif 
 


// DEFINE VARIABLES

uint8_t currentDigit = 1;  
bool lockState = false;  
uint8_t digit_1 = 0;
uint8_t digit_2 = 0;
uint8_t digit_3 = 0;
uint8_t digit_4 = 0;

#define TFT_DC    17
#define TFT_CS    5
#define TFT_RST   16
#define TFT_CLK   18
#define TFT_MOSI  23
#define TFT_MISO  19

#define BTN1      32
#define BTN2      25
#define BTN3      27
#define analogPin    36

#define BG_colour 0xF73F
// IMPORT FONTS FOR TFT DISPLAY
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

 


// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620161521";                    // Add your ID number here
static const char* subtopic[]    = {"620161521_sub","/elet2415"};  // Array of Topics(Strings) to subscribe to
static const char* mqtt_server   = "yanacreations.com";         // Broker IP address or Domain name as a String 
static uint16_t mqtt_port        = 1883;

// WIFI CREDENTIALS
const char* ssid       = "MonaConnect"; // Add your Wi-Fi ssid
const char* password   = ""; // Add your Wi-Fi password 



// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL; 


// FUNCTION DECLARATION   
void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters ); 

void digit1(uint8_t number);
void digit2(uint8_t number);
void digit3(uint8_t number);
void digit4(uint8_t number);

void checkPasscode(void);
void showLockState(void);

void fillScreen(uint16_t color);

 

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif


/* Initialize class objects*/

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

 
 
/* Declare your functions below */



void setup() {
  Serial.begin(115200);  // INIT SERIAL 
 
  // CONFIGURE THE ARDUINO PINS OF THE 7SEG AS OUTPUT
  pinMode(BTN1, INPUT_PULLUP); 
  pinMode(BTN2, INPUT_PULLUP); 
  pinMode(BTN3, INPUT_PULLUP); 

  /* Configure all others here */
  tft.begin();
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillScreen(BG_colour);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.fillRoundRect(185, 260, 50, 50, 5, ILI9341_BLACK);
  tft.fillRoundRect(125, 260, 50, 50, 5, ILI9341_BLACK);
  tft.fillRoundRect(65, 260, 50, 50, 5, ILI9341_BLACK);
  tft.fillRoundRect(5, 260, 50, 50, 5, ILI9341_BLACK);
  tft.setCursor(20, 295);
  tft.print(0);
  tft.setCursor(80, 295);
  tft.print(0);
  tft.setCursor(140, 295);
  tft.print(0);
  tft.setCursor(200, 295);
  tft.print(0);

  initialize();           // INIT WIFI, MQTT & NTP 
  vButtonCheckFunction(); // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION

}
  


void loop() {
  int potValue = analogRead(analogPin);
  int digitValue = map(potValue, 0, 4095, 0, 9); // ESP32 12-bit ADC

  switch (currentDigit) {
    case 1:
      digit_1 = digitValue;
      digit1(digit_1);
      break;
    case 2:
      digit_2 = digitValue;
      digit2(digit_2);
      break;
    case 3:
      digit_3 = digitValue;
      digit3(digit_3);
      break;
    case 4:
      digit_4 = digitValue;
      digit4(digit_4);
      break;
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);  
}



  
//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );     
      
    for( ;; ) {
        // Add code here to check if a button(S) is pressed
        // then execute appropriate function if a button is pressed  

        // 1. Implement button1  functionality
        if (digitalRead(BTN1) == LOW) {
          vTaskDelay(50 / portTICK_PERIOD_MS);
          currentDigit = currentDigit%4 + 1;
        }
        // 2. Implement button2  functionality
        if (digitalRead(BTN2) == LOW) {
          vTaskDelay(50 / portTICK_PERIOD_MS);
          checkPasscode();
         }

        // 3. Implement button3  functionality
        if (digitalRead(BTN3) == LOW) {
          vTaskDelay(50 / portTICK_PERIOD_MS);
          lockState = false;
          showLockState();
          }
        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }
}

void vUpdate( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
 
    for( ;; ) {
          // Task code goes here.   
          // PUBLISH to topic every second.  
            
        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}

unsigned long getTimeStamp(void) {
          // RETURNS 10 DIGIT TIMESTAMP REPRESENTING CURRENT TIME
          time_t now;         
          time(&now); // Retrieve time[Timestamp] from system and save to &now variable
          return now;
}

void callback(char* topic, byte* payload, unsigned int length) {
  // ############## MQTT CALLBACK  ######################################
  // RUNS WHENEVER A MESSAGE IS RECEIVED ON A TOPIC SUBSCRIBED TO
  
  Serial.printf("\nMessage received : ( topic: %s ) \n",topic ); 
  char *received = new char[length + 1] {0}; 
  
  for (int i = 0; i < length; i++) { 
    received[i] = (char)payload[i];    
  }

  Serial.printf("Payload : %s \n", received);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, received);  

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

  // PROCESS MESSAGE

}

bool publish(const char *topic, const char *payload){   
     bool res = false;
     try{
        res = mqtt.publish(topic,payload);
        // Serial.printf("\nres : %d\n",res);
        if(!res){
          res = false;
          throw false;
        }
     }
     catch(...){
      Serial.printf("\nError (%d) >> Unable to publish message\n", res);
     }
  return res;
}

//***** Complete the util functions below ******
  
void digit1(uint8_t number){
  // CREATE BOX AND WRITE NUMBER IN THE BOX FOR THE FIRST DIGIT
  // 1. Set font to FreeSansBold18pt7b 
  // 2. Draw a filled rounded rectangle close to the bottom of the screen. Give it any colour you like 
  // 3. Set cursor to the appropriate coordinates in order to write the number in the middle of the box 
  // 4. Set the text colour of the number. Use any colour you like 
  // 5. Set font size to one 
  // 6. Print number to the screen 
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRoundRect(5, 260, 50, 50, 5, ILI9341_BLACK);
  tft.setCursor(20, 295);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.print(number); 
}
 
void digit2(uint8_t number){
  // CREATE BOX AND WRITE NUMBER IN THE BOX FOR THE SECOND DIGIT
  // 1. Set font to FreeSansBold18pt7b 
  // 2. Draw a filled rounded rectangle close to the bottom of the screen. Give it any colour you like 
  // 3. Set cursor to the appropriate coordinates in order to write the number in the middle of the box 
  // 4. Set the text colour of the number. Use any colour you like 
  // 5. Set font size to one 
  // 6. Print number to the screen 
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRoundRect(65, 260, 50, 50, 5, ILI9341_BLACK);
  tft.setCursor(80, 295);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.print(number);
}

void digit3(uint8_t number){
  // CREATE BOX AND WRITE NUMBER IN THE BOX FOR THE THIRD DIGIT
  // 1. Set font to FreeSansBold18pt7b 
  // 2. Draw a filled rounded rectangle close to the bottom of the screen. Give it any colour you like 
  // 3. Set cursor to the appropriate coordinates in order to write the number in the middle of the box 
  // 4. Set the text colour of the number. Use any colour you like 
  // 5. Set font size to one 
  // 6. Print number to the screen 
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRoundRect(125, 260, 50, 50, 5, ILI9341_BLACK);
  tft.setCursor(140, 295);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.print(number);
}

void digit4(uint8_t number){
  // CREATE BOX AND WRITE NUMBER IN THE BOX FOR THE FOURTH DIGIT
  // 1. Set font to FreeSansBold18pt7b 
  // 2. Draw a filled rounded rectangle close to the bottom of the screen. Give it any colour you like 
  // 3. Set cursor to the appropriate coordinates in order to write the number in the middle of the box 
  // 4. Set the text colour of the number. Use any colour you like 
  // 5. Set font size to one 
  // 6. Print number to the screen 
  tft.setFont(&FreeSansBold18pt7b);
  tft.fillRoundRect(185, 260, 50, 50, 5, ILI9341_BLACK);
  tft.setCursor(200, 295);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.print(number);
}
 
 
void checkPasscode(void){
    WiFiClient client;
    HTTPClient http;

    if(WiFi.status() == WL_CONNECTED){ 
        // Replace localhost with your backend server's IP address
        http.begin(client, "172.16.192.70:8080/api/check/combination"); 
        
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        char message[20];
        
        // Format the passcode as "passcode=1234"
        sprintf(message, "passcode=%d%d%d%d", digit_1, digit_2, digit_3, digit_4);
                      
        int httpResponseCode = http.POST(message);

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String received = http.getString();
            
            // Convert received string to JSON
            JsonDocument doc;  // Adjust size as needed
            DeserializationError error = deserializeJson(doc, received);
            
            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }

            // Process the response
            const char* status = doc["status"];
            if (strcmp(status, "complete") == 0) {
                lockState = true;
                showLockState();
            } else {
                lockState = false;
                showLockState();
            }
        } else {
            Serial.print("Error in HTTP request: ");
            Serial.println(httpResponseCode);
        }
        
        // Free resources
        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}


void showLockState(void){
  
    // Toggles the open and close lock images on the screen based on the lockState variable  
    tft.setFont(&FreeSansBold18pt7b);  
    tft.setTextSize(1);
    

    if(lockState == true){
      tft.drawRGBBitmap(68,10, lockopen, 104, 97); 
      tft.setCursor(5, 200);  
      tft.setTextColor(ILI9341_WHITE); 
      tft.printf("Access Denied"); 
      tft.setCursor(5, 200);  
      tft.setTextColor(ILI9341_GREEN); 
      tft.printf("Access Granted");
      
    }
    else {
      tft.drawRGBBitmap(68,10, lockclose, 104, 103); 
      tft.setCursor(5, 200);  
      tft.setTextColor(ILI9341_WHITE); 
      tft.printf("Access Granted"); 
      tft.setCursor(5, 200);  
      tft.setTextColor(ILI9341_RED); 
      tft.printf("Access Denied"); 
    }
    
}
 