#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Time.h>

#include<Wire.h>
#include<ADXL345_WE.h>
#define ADXL345_I2CADDR 0x53  // 0x1D if SDO = HIGH
//const int int1Pin = 37;
volatile bool tap = false;

ADXL345_WE myAcc = ADXL345_WE(ADXL345_I2CADDR);

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  4        /* Time ESP32 will go to sleep (in seconds) */

#define DEFAULT_I2C_PORT &Wire
#define NUMPIXELS        1

Adafruit_NeoPixel pixels(NUMPIXELS, 0, NEO_GRB + NEO_KHZ800);
ESP32Time rtc;

bool deviceConnected = false;
String valor;
volatile int device_disconnect;
int device_disconnect_start;

RTC_DATA_ATTR int bootCount = 0;

struct Button {
    const uint8_t PIN;
    uint32_t numberKeyPresses;
    bool pressed;
};

Button button_33 = {33, 0, false};
Button button_38 = {38, 0, false};

void ARDUINO_ISR_ATTR isr(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->pressed = true;
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      Serial.println(value.data());
      if (value.length() > 0) {
        valor = "";
        for (int i = 0; i < value.length(); i++){
          // Serial.print(value[i]); // Presenta value.
          valor = valor + value[i];
        }
        if(valor == "red")
        {
        // set color to red
          pixels.fill(0xFF0000);
          pixels.show();
          //delay(2000); // wait half a second
        }
        if(valor == "on")
        {
        // set color to red
        //Serial.println(valor);
        pixels.fill(0x0000FF);
        pixels.show();                
          //deviceConnected=false;
          //tone(27,234);
          //delay(2000); // wait half a second
        }
        if(valor == "blue")
        {
        // set color to red
          pixels.fill(0x0000FF);
          pixels.show();
          //delay(2000); // wait half a second
        }
        else
        {
          if(valor.length() > 10)
          {
          //string_parse(valor);
          valor="";
          }
        }
        
       /* Serial.println("*********");
        Serial.print("valor = ");
        Serial.println(valor); // Presenta valor.*/
      }
    }
};



class MyServerCallbacks1: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    //ble_initialization();
    device_disconnect_start=1;
    device_disconnect++;
    deviceConnected = false;
  }
};


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :  Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 :  pixels.fill(0x00FF00);pixels.show();delay(500);pixels.fill(0xFF0000);pixels.show();delay(500);
                                  Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long currentMillis = 0;
const long interval = 8000;           // interval at which to blink (milliseconds)


void setup(){
   if(bootCount == 0)
   {
   rtc.setTime(30, 54, 16, 8, 5, 2024);  // 17th Jan 2021 15:24:30
   bootCount = 1;
   }
   Wire.begin();
   Serial.begin(115200);
   delay(200);
   Serial.println(rtc.getTime());          //  (String) 15:24:38
   Serial.println(rtc.getDate());          //  (String) Sun, Jan 17 2021
   
   ble_initialization();
  //delay(1000); //Take some time to open up the Serial Monitor
    //pinMode(int1Pin, INPUT);
    pinMode(button_33.PIN, INPUT_PULLUP);
    attachInterruptArg(button_33.PIN, isr, &button_33, RISING);
    
    if (!myAcc.init()) {
    Serial.println("ADXL345 not connected!");
    }
      myAcc.setDataRate(ADXL345_DATA_RATE_200);
      Serial.print("Data rate: ");
     Serial.print(myAcc.getDataRateAsString());

    myAcc.setRange(ADXL345_RANGE_2G);
    Serial.print("  /  g-Range: ");
    Serial.println(myAcc.getRangeAsString());
    Serial.println();

    // attachInterrupt(digitalPinToInterrupt(int1Pin), tapISR, FALLING);

    myAcc.setGeneralTapParameters(ADXL345_XY0, 3.0, 30, 100.0);
    myAcc.setAdditionalDoubleTapParameters(false, 250);

    //myAcc.setInterrupt(ADXL345_SINGLE_TAP, button_33.PIN);
    myAcc.setInterrupt(ADXL345_DOUBLE_TAP, button_33.PIN);
    myAcc.readAndClearInterrupts();









    pinMode(button_38.PIN, INPUT_PULLUP);
    attachInterruptArg(button_38.PIN, isr, &button_38, RISING);

 /// NeoPixel Setup  starts  ///
#if defined(NEOPIXEL_POWER)
  // If this board has a power control pin, we must set it to output and high
  // in order to enable the NeoPixels. We put this in an #if defined so it can
  // be reused for other boards without compilation errors
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
pixels.setBrightness(2); // not so bright
pixels.fill(0x00FF00);pixels.show();
///// Power on green  //

//Print the wakeup reason for ESP32
 print_wakeup_reason();


///    wait for some seconds ...Find my device feature /////
  previousMillis = millis();
       while(1)
          {
      currentMillis = millis();
      if (currentMillis - previousMillis >= 5000) {
         Serial.print(currentMillis); Serial.print("\t"); Serial.println(previousMillis);
         break;
              }
              if(button_33.pressed )    // Dosage interrupt
              {
                button_33.pressed = false;
     
        String axes = myAcc.getActTapStatusAsString();
        byte intSource = myAcc.readAndClearInterrupts();

         if(myAcc.checkInterrupt(intSource, ADXL345_DOUBLE_TAP)){
          pixels.fill(0x00FF00);pixels.show();
          Serial.print("DOUBLE TAP at: ");
          Serial.println(axes);
         }
    
            delay(1000);
            myAcc.readAndClearInterrupts();
            
      // tap = false;

                //Read_Sensor();
                //Write_eeprom();
              }
              if(button_38.pressed)   //  Accelerometer interrupt
              {
                 button_38.pressed = false;
                 Serial.println("Accelerometer Interrupt");
                 //Read_eeprom();
                 //Notify_BLE();
              }
              

           }



////   Wait for some time over   /////////

 

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_38,0); //1 = High, 0 = Low

  //If you were to use ext1, you would use it like
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //Go to sleep now

  esp_deep_sleep_start();
}



void loop(){
  
  
 
}

void ble_initialization()
{
  // Create the BLE2 Device
  if(device_disconnect_start == 0)
  {
  BLEDevice::init("SAANS");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks1());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  }
  if(device_disconnect_start == 1)
  {
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  }
}

