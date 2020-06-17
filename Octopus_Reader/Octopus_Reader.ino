/* 
* Octopus Card Reader
*
*
*
* Modified "BUFFER_LENGTH" from 32 to 64
* @ /Arduino.app/Contents/Java/hardware/arduino/avr/libraries/Wire/src/Wire.h
* 
* Modified "TWI_BUFFER_LENGTH" from 32 to 64
* @ /Arduino.app/Contents/Java/hardware/arduino/avr/libraries/Wire/src/utility/twi.h
* Ref:
* 
*/

// Program Parameter
// #define CENSOR_MODE      //Disable showing Card Information
#define SERIAL_OUTPUT       //Show the program status via USB Serial Console
// #define DISPLAY_MODE 2   //Show the program status via hardware display
                            // Uncomment the line above to enable hardware display output
                            // Set to "1" for SSD1306 OLED, "2 for" HD44780 LCD display



//Include Header
#include "program_setting.h"
#include "octopus_setting.h"
#include "message_data.h"
#include <Arduino.h>

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#endif


#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <PN532_debug.h>


// NFC Declaration
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

// Display Declaration
#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
Adafruit_SSD1306 oled(128, 64, &Wire, -1);//OLED 128x64 No Reset Pin
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#endif

//NFC global variable
uint8_t        _prevIDm[8];
unsigned long  _prevTime;

//Useful Fuction
String convByteToString(const uint8_t* data, uint8_t length)
{
  String return_value;
  for(int i=0; i< length; i++)
  {
    if (data[i] < 0x10) 
    {
      return_value+="0"+String(data[i], HEX);
    } 
    else 
    {
      return_value+=String(data[i], HEX);
    }
  }
  return return_value;
}

void setup(void)
{
//Enable Required Hardware module and show welcome message

#if defined(SERIAL_OUTPUT)
//Enable USB Serial
  Serial.begin(115200);
  Serial.println(WORD_NFC_WELLCOME);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
//Enable SSD1306 OLED Display
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  for(;;); // Don't proceed, loop forever
  }
  oled.setTextColor(SSD1306_WHITE); //Set to write Color
  oled.clearDisplay();

  oled.setCursor(0, 0);
  oled.print(F(WORD_NFC_WELLCOME));
  oled.display();
  
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
//Enable HD44780 LCD Display
  lcd.init();
  lcd.setCursor(0, 0);
  lcd.print(WORD_NFC_WELLCOME);
  lcd.backlight();
#endif

//Start the NFC Module
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) // If no nfc module detected
  {
#if defined(SERIAL_OUTPUT)
    Serial.print(WORD_NFC_INI_ERROR);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
// SSD1306 LCD Display
  oled.setCursor(0, 10);
  oled.print(F(WORD_NFC_INI_ERROR));
  oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
//HD44780 LCD Display
  lcd.setCursor(0, 10);
  lcd.print(WORD_NFC_INI_ERROR);
#endif
    while (1) {delay(10);};      // halt
  }

//Print NFC Module data
  //Prepare data string
  String nfc_type = String(WORD_NFC_FOUND) + String(((versiondata >> 24) & 0xFF), HEX);
  String nfc_firmware = String(WORD_NFC_SHOW_FIRMWARE) + String(((versiondata >> 16) & 0xFF), DEC) + String(".") + String(((versiondata >> 8) & 0xFF), DEC);

#if defined(SERIAL_OUTPUT)
  // Got ok data, print it out!
  Serial.println(nfc_type);
  Serial.println(nfc_firmware);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 10);
  oled.print(nfc_type);
  //Print firmware version
  oled.setCursor(0, 20);
  oled.print(nfc_firmware);
  oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
  lcd.setCursor(0, 1);
  lcd.print(nfc_type);
  lcd.setCursor(0, 2);
  lcd.print(nfc_firmware);
#endif

  

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();

  memset(_prevIDm, 0, 8); //Clear the variable

  //Clean the display
#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
  oled.clearDisplay();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
  lcd.clear();
#endif
}

void loop(void)
{
  uint8_t ret;
  uint16_t systemCode = OCTOPUS_SYSTEM_CODE;
  uint8_t requestCode = OCTOPUS_REQUEST_CODE;       // System Code request
  uint8_t idm[8];
  uint8_t pmm[8];
  uint16_t systemCodeResponse;

  
#if defined(SERIAL_OUTPUT)
//Enable USB Serial
  Serial.print(WORD_NFC_WAIT_CARD);
#endif
#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
  oled.fillRect(0, OLED_INFO_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
  oled.display();
  oled.setTextSize(1);
  oled.setCursor(0, OLED_INFO_POSITION);
  oled.setTextColor(SSD1306_WHITE); //Show the display
  oled.print(F(WORD_NFC_WAIT_CARD));
  oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
  lcd.setCursor(0, 0);
  lcd.print(WORD_NULL_LINE); //Clean the line
  lcd.setCursor(0, 0);
  lcd.print(WORD_NFC_WAIT_CARD);
#endif

  // Poll the Octopus card
  ret = nfc.felica_Polling(systemCode, requestCode, idm, pmm, &systemCodeResponse, 5000);  

  //Cannot find an Octopus Card
  if (ret != 1)
  {
#if defined(SERIAL_OUTPUT)
    Serial.println(WORD_FELICA_NFOUND);
#endif
    delay(500);
    return;
  }
  // Found an Octopus Card
  else
  {
    //Find Same card id
    if ( memcmp(idm, _prevIDm, 8) == 0 ) 
    {
        //Not yet timeout, then skip processing
        if ( (millis() - _prevTime) < OCTOPUS_SAME_CARD_TIMEOUT ) 
        {
#if defined(SERIAL_OUTPUT)
        Serial.println(WORD_FELICA_SAME);
#endif
        delay(500);
        return;
        }
    }
    

//Found a new card
//Censor mode
#ifdef CENSOR_MODE
    String felica_idm = WORD_CARD_CENSOR;
    String felica_pmm = WORD_CARD_CENSOR;
//Not censor card data
#else
    String felica_idm = convByteToString(idm, 8);
    String felica_pmm = convByteToString(pmm, 8);
#endif
    String scode_re = String(systemCodeResponse, HEX);

// display the card info
#if defined(SERIAL_OUTPUT)
    Serial.println(WORD_FELICA_FOUND);
    Serial.print(WORD_FELICA_CARD_IDM);
    Serial.println(felica_idm);
    Serial.print(WORD_FELICA_CARD_PMM);
    Serial.println(felica_pmm);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
  oled.fillRect(0, OLED_IDM_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
  oled.fillRect(0, OLED_PMM_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
  oled.display();

  oled.setTextSize(1);
  oled.setCursor(0, OLED_IDM_POSITION);
  oled.setTextColor(SSD1306_WHITE); //Show the display
  oled.print(String(WORD_FELICA_CARD_IDM)+felica_idm); //Print Card ID
  oled.setCursor(0, OLED_PMM_POSITION);
  oled.setTextColor(SSD1306_WHITE); //Show the display
  oled.print(String(WORD_FELICA_CARD_PMM)+felica_pmm); //Print Card PM
  oled.display();

#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
  lcd.setCursor(0, 2);
  lcd.print(WORD_NULL_LINE); //Clean the line
  lcd.setCursor(0, 3);
  lcd.print(WORD_NULL_LINE); //Clean the line
  lcd.setCursor(0, 2);
  lcd.print(String(WORD_FELICA_CARD_IDM)+felica_idm); //Print Card ID
  lcd.setCursor(0, 3);
  lcd.print(String(WORD_FELICA_CARD_PMM)+felica_pmm); //Print Card PM
#endif


#if defined(SERIAL_OUTPUT)
    Serial.print(WORD_FELICA_CARD_SCODE);
    Serial.println(scode_re);
#endif
  

//Start Read the octopus card balance
    uint16_t serviceCodeList[1];
    uint16_t returnKey[1];
    uint16_t blockList[1];
    uint8_t blockData[1][16];


    serviceCodeList[0]=OCTOPUS_BALANCE_SERVICE_CODE;
    blockList[0] = OCTOPUS_BALANCE_SERVICE_BLOCK;


    //Request the Service, Read Balance
    // if Omit Request procedure, octopus on Apple Pay cannot complete the reading process until Timeout
    ret = nfc.felica_RequestService(1, serviceCodeList, returnKey); //Request the [fix the ]
    if(ret == 1)
    {
        Serial.println(WORD_SUCESS_REQUEST);
        ret = nfc.felica_ReadWithoutEncryption(1, serviceCodeList, 1, blockList, blockData);

        //Successful Read the balance
        if(ret == 1)
        {
            String print_balance = "$";
            int32_t balance = 0;
            int16_t upper_bal = 0;
            int16_t lower_bal = 0;

            //Process the balance from block data
            for(int i=0;i<4;i++) {
                balance<<=8;
                balance|=blockData[0][i];  
            }
            //Offset the balance
            balance = balance - OCTOPUS_BALANCE_OFFSET;
            if (balance<0) {
                print_balance+="-";
                balance=-balance;
            }
            //Prepare balance String object
            print_balance+=((balance - (balance % 10))/10);
            print_balance+=".";
            print_balance+=(balance % 10); 
            
#if defined(SERIAL_OUTPUT)
            Serial.println(WORD_SUCESS_READ_BALANCE);
            Serial.print(WORD_SUCESS_PRINT_BALANCE);
            Serial.println(print_balance);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
            oled.fillRect(0, OLED_INFO_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
            oled.fillRect(0, OLED_BAL_POSITION, 128, OLED_CLEAR_RANGE*3, BLACK); //Clear Display
            oled.display();
            //Print Successful Message
            oled.setTextSize(1);
            oled.setCursor(0, OLED_INFO_POSITION);
            oled.setTextColor(SSD1306_WHITE); //Show the display
            oled.print(F(WORD_SUCESS_READ_BALANCE));
            //Print Balance
            oled.setTextSize(3);
            oled.setCursor(0, OLED_BAL_POSITION);
            oled.setTextColor(SSD1306_WHITE); //Show the display
            oled.print(print_balance);
            oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
            lcd.setCursor(0, 1);
            lcd.print(WORD_SUCESS_PRINT_BALANCE+print_balance);
#endif
        }
        //Cannot Read Balance
        else
        {
#if defined(SERIAL_OUTPUT)
            Serial.println(WORD_ERR_READ_BALANCE);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1
            oled.fillRect(0, OLED_INFO_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
            oled.display();

            oled.setTextSize(1);
            oled.setCursor(0, OLED_INFO_POSITION);
            oled.setTextColor(SSD1306_WHITE);
            oled.print(F(WORD_ERR_READ_BALANCE));
            oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
            lcd.setCursor(0, 1);
            lcd.print(WORD_NULL_LINE); //Clean the line
            lcd.setCursor(0, 1);
            lcd.print(WORD_ERR_READ_BALANCE);
#endif
        }
    }
    //Fail to request Service
    else
    {
#if defined(SERIAL_OUTPUT)
      Serial.println(WORD_ERR_REQUEST);
#endif

#if defined(DISPLAY_MODE) && DISPLAY_MODE == 1 
            oled.fillRect(0, OLED_INFO_POSITION, 128, OLED_CLEAR_RANGE, BLACK); //Clear Display
            oled.display();

            oled.setTextSize(1);
            oled.setCursor(0, OLED_INFO_POSITION);
            oled.setTextColor(SSD1306_WHITE);
            oled.print(F(WORD_ERR_REQUEST));
            oled.display();
#elif defined(DISPLAY_MODE) && DISPLAY_MODE == 2
            lcd.setCursor(0, 1);
            lcd.print(WORD_NULL_LINE); //Clean the line
            lcd.setCursor(0, 1);
            lcd.print(WORD_ERR_REQUEST);
#endif
    }

    //Finish Reading, Release the card    
    ret = nfc.felica_Release(); 
    if(ret == 1)
    {
#if defined(SERIAL_OUTPUT)
      Serial.println(WORD_SUCESS_RELEASE_CARD);
#endif
    }
    else
    {
#if defined(SERIAL_OUTPUT)
      Serial.println(WORD_FAIL_RELEASE_CARD);
#endif
    }

    // Wait 1 second before continuing
    memcpy(_prevIDm, idm, 8);
    _prevTime = millis();
#if defined(SERIAL_OUTPUT)
    Serial.println(WORD_PROCEDURE_FINISH);
#endif
    delay(1000);
  }
}