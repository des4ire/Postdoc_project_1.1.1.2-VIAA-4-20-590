//####################################################################################################
//#   												     #
//#   Postdoc Research proposal # 1.1.1.2/VIAA/4/20/590						     #
//#   Title:"Portable diagnostic device based on a biosensor array of 2D material sensing elements"  #
//#   File description: ESP32 WROOM d32 based prototype firmware 				     #
//#   Author: Andrejs Ogurcovs									     #
//#   Date: 18.05.2023.										     #
//#   License: MIT										     #
//#												     #
//#												     #
//####################################################################################################
#include <string.h>
#include <stdio.h>

#include <Adafruit_MCP4725.h> 
#include <Adafruit_ADS1X15.h>
//#include <SPI.h>
#include <ctype.h>
#include <stdlib.h>


#include <Wire.h>
Adafruit_MCP4725 MCP4725; 
Adafruit_ADS1115 ADS1115;
Adafruit_MCP4725 GATE_DAC;
#define MATRIX 0x20; //matrix adress
const float multiplier = 0.1875F;

//################################ SPI INIT ###############################
#include <SPI.h>
#define VSPI_MISO   MISO
  #define VSPI_MOSI   MOSI
  #define VSPI_SCLK   SCK
  #define VSPI_SS     SS
  #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
  #define VSPI FSPI
  #endif
   byte address = 0x11;
  int i=0;
  static const int spiClk = 1000000; // 1 MHz
  //uninitalised pointers to SPI objects
  SPIClass * vspi = NULL;
  //SPIClass * hspi = NULL;
// ############################# global variables #############################
  char ctrl1[10]="scan";  //start scan of the array
//char ctrl2[10]="stop"; //stop scanning
char ctrl3[10]="setgsv"; //set gate-drain voltage
char ctrl4[10]="setdsv"; //set source-drain voltage
char ctrl5[10]="getvolt"; //get voltage feedback
char ctrl6[10]="setpot"; //set potentiometer
//char ctrl7[10]="run"; //run scan
char ctrl8[10]="setcell"; //set cell order
char ctrl9[10]="help";
char ctrl10[11]="reboot"; //reboot MCU

String input;
char temp_buffer[20];
char buffer[7][20]; //store tokenized data

float drain_voltage[64]; //

uint16_t scan_buffer[3]; //store scan settings, start cell, end cell
uint16_t gate_buffer[2];// store gate voltage settings
uint16_t drain_buffer[2];//storer  source-drain voltage value

const int M=5; //ints to pass buffer array to a function
const int N=20;

//boolean execute=true; //loop breaking condition
//const int MUX_enable = 8; //enable MUX output
//const int RESET = 9; //reset signal 


const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;

boolean newData = false;


//############################ display section ################################
  #include <Adafruit_GFX.h>
  #include <Adafruit_SH110X.h>

  /* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
  #define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
  //#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  #define OLED_RESET -1   //   QT-PY / XIAO
  Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  #define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};
//############################## end display #######################################
 
  
 

void setup() {
  //-------------------------------------------------------------------------------         
    vspi = new SPIClass(VSPI);
    vspi->begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_SS); //SCLK, MISO, MOSI, SS
    pinMode(vspi->pinSS(), OUTPUT); //VSPI SS
  //-------------------------------------------------------------------------------- 
   
   Wire.begin();  /*I2C Communication begins*/
   Serial.begin(115200);  /*Baud Rate defined for serial communication*/
  //Serial.println("\nI2C Scanner"); /*print scanner on serial monitor*/
  Serial.println(F("SENSOR NODE, ID: 909345678"));
  i2c_scann();
   Wire.beginTransmission(0x20); //disable mux output I2C
                 Wire.write(0x3F);
                 Wire.endTransmission();
  
  
  spi_check();
  
  display.begin(0x3c, true); // Address 0x3C default
 //display.setContrast (0); // dim display
   display.display();
   String msg="System_error";

   display_text(msg);
  ///////////////////////////////////////
   Serial.println();
  
  Serial.println(F("SN: Enter command:"));
    
    Serial.println();
     MCP4725.begin(0x60); //drain-source drain
     ADS1115.begin(0x48);
      ADS1115.setGain(GAIN_TWOTHIRDS); //set +/-6 volt range 
      GATE_DAC.begin(0x61); //gate dac
      MCP4725.setVoltage(0, false); //swith off source-drain voltage
      GATE_DAC.setVoltage(1400, false); //0V at gate 
 
      delay(100);

  ///////////////////////////////////////

    

}

void i2c_scann(){
  Serial.println("\nI2C Scanner");
   byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("Done\n");
  }
  delay(500);          
  }

void spi_check()
{
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);  
  }

 void spiCommand(SPIClass *spi, byte data) {
  //use it as you would the regular arduino SPI API
  spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(spi->pinSS(), LOW); //pull SS slow to prep other end for transfer
  spi->transfer(address);
  spi->transfer(data);
  digitalWrite(spi->pinSS(), HIGH); //pull ss high to signify end of data transfer
  spi->endTransaction();
}
//#####################################3 display ##################################################
void display_text(String msg)
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
  delay(2000);
  display.clearDisplay();
  }

//################################################ CELL SCAN SECTION ######################################

 
 void matrix(uint16_t period, uint8_t start_cell, uint8_t end_cell)  
{
  uint8_t i=0;
  uint8_t y=0;
  uint32_t MCP4725_value;
  uint8_t j=0; //cell counter
  String msg1="Scan started", msg2="Scan_complete!", msg3="Aborted!";
   
  //unsigned int c;
  //unsigned long timer;

    MCP4725.setVoltage(drain_buffer[0], false);   
    
   // digitalWrite(MUX_enable, LOW); //enable mux output native port
               //  Wire.beginTransmission(0x20); //enable mux output I2C
               //  Wire.write(0x0);
               //  Wire.endTransmission();
    display_text(msg1);
    for(i=(start_cell-1); i<=(end_cell-1); i++)
    {
     j++;
      if (Serial.available()>0){
  
        String data =Serial.readString();
        data.trim();
          if (data=="<scan:OFF>"){
          Serial.println("###### Aborted! #######");
         break;
          }
          }
          
         Wire.beginTransmission(0x20);
         Wire.write(i);
         Wire.endTransmission();
         //PORTD=i<<2; //write data to the port  
         //Serial.println(get_voltage()); //read voltage values
         drain_voltage[i]=get_voltage();
        delay(period);
        }
    
               MCP4725.setVoltage(0, false); //swith off source-drain voltage
                 
                 Wire.beginTransmission(0x20); //disable mux output I2C
                 Wire.write(0x3F);
                 Wire.endTransmission();
     
                //digitalWrite(MUX_enable, HIGH); //disable mux output
                display_text(msg2);
                Serial.println("###### Scan complete! #######");
                Serial.println("Voltage drop on shunt:");
                for(j=(start_cell-1); j<i; j++)
                {
                
               Serial.print("Cell#");
               Serial.print(j+1);
               Serial.print(" ");
               Serial.println(drain_voltage[j]);
                  }
              ///drain_voltage[64]={0};  
    
    
    }

//######################################### gate voltage control ###########################

void setgs(){
GATE_DAC.setVoltage(550, false); //-2V
delay(10000);
GATE_DAC.setVoltage(975, false); //-1V
delay(10000);
GATE_DAC.setVoltage(1187, false); //-0.5V
delay(10000);
GATE_DAC.setVoltage(1400, false); //0V
delay(10000);
GATE_DAC.setVoltage(1825, false); //1v
delay(10000);
GATE_DAC.setVoltage(2037, false); //1.5v
delay(10000);
GATE_DAC.setVoltage(2250, false); //2V
delay(10000);
}
//################################################################################################
//######################################### source-drain voltage controle ##########################3
  
  void setds(){
    uint32_t MCP4725_value;
    for (MCP4725_value = 0; MCP4725_value < 4096; MCP4725_value = MCP4725_value + 15)
    {
     
      MCP4725.setVoltage(MCP4725_value, false);
      delay(50);
     
      
      //Serial.print("MCP4725 Value: ");
      //Serial.print(MCP4725_value);
      
    }
    MCP4725.setVoltage(0, false); //swith off source-drain voltage
  }
  
//########################################### ADC control ################################################
float get_voltage()
{
  
  int16_t adc0, adc1, adc2, adc3;
 
  // ads.setGain(GAIN_TWOTHIRDS);  +/- 6.144V  1 bit = 0.1875mV (default)
  // ads.setGain(GAIN_ONE);        +/- 4.096V  1 bit = 0.125mV
  // ads.setGain(GAIN_TWO);        +/- 2.048V  1 bit = 0.0625mV
  // ads.setGain(GAIN_FOUR);       +/- 1.024V  1 bit = 0.03125mV
  // ads.setGain(GAIN_EIGHT);      +/- 0.512V  1 bit = 0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    +/- 0.256V  1 bit = 0.0078125mV 
  
  adc0 = ADS1115.readADC_SingleEnded(0);
 // Serial.println(adc0);
 // adc1 = ADS1115.readADC_SingleEnded(1);
 // adc2 = ADS1115.readADC_SingleEnded(2);
 // adc3 = ADS1115.readADC_SingleEnded(3);
 // Serial.print("AIN0: "); Serial.println(adc0 * multiplier);
 // Serial.print("AIN1: "); Serial.println(adc1 * multiplier);
 // Serial.print("AIN2: "); Serial.println(adc2 * multiplier);
 // Serial.print("AIN3: "); Serial.println(adc3 * multiplier);
  //Serial.println(" ");
  return (adc0 * multiplier);
  //delay(1000);
  
  
 // } 
}

//################################### help section ##################################################

void help(){
  Serial.println("\tAvailiable commands:");
  Serial.println("\tsetcell - scan options for sensor array."); 
  Serial.println(          "\tArguments: period (50-2000 ms), start cell (1-64), end cell (1-64)");
  Serial.println(          "\tExample:<scan:1:10:60>");
  Serial.println("\tsetdsv -  sets voltage level of source-drain channel"); 
  Serial.println(          "\tArguments: voltage (0-4500 mV)");
  Serial.println(          "\tExample:<setdsv:3500>");
  Serial.println("\tsetgsv - sets voltage level of gate-source"); 
  Serial.println(          "\tArguments: voltage (+/-2500 mV)");
  Serial.println(          "\tExample:<setgsv:-2000>");
  Serial.println("\treboot -  reboots the device"); 
  Serial.println(          "\tArguments: NO");
  Serial.println(          "\tExample:<reboot>");
  }

 //#######################################################################################################
//################################## parser section ###################################################

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}



//============

void parsedata(){
  char * token; // this is used by strtok() as an index
  //char * token; // this is used by strtok() as an index
  int len=0;
  uint8_t counter = 0;
  int res;

  token = strtok(tempChars,":");      // get the first part - the string
   while (token != NULL) {
        //Serial.println(token);
        strcpy(buffer[counter], token);
        Serial.println(buffer[counter]);
        token = strtok(NULL, " : ");
        counter++;
       
    }
      Serial.print(F(" SN: Received number of tokens: ")); Serial.println(counter);
        
        
        //######################################## matrix parser #######################################      
        
        if ((res = strcmp(buffer[0], ctrl8)==0) && (counter != 4) )
        {
         Serial.println("Not enough arguments for <setcell> function!");
         Serial.println("Please specify delay (range: 50-2000 ms), start cell and end cell (range: 1 - 64) followed by <setcell>");
         Serial.println("Example: <setcell:250:5:50>");
         
          }
        else if ( (res = strcmp(buffer[0], ctrl8)==0) && (counter = 4) )
                {
               
                    if( (50<=atoi(buffer[1]) && atoi(buffer[1])<=2000) && (1<=atoi(buffer[2]) && atoi(buffer[2])<=64) && (1<=atoi(buffer[3]) && atoi(buffer[3])<=64) && (atoi(buffer[2]) <= atoi(buffer[3]))   )
                    {
                      
                      Serial.println("Setcell settings are OK!");
                           scan_buffer[0]=atoi(buffer[1]);
                           scan_buffer[1]=atoi(buffer[2]);
                           scan_buffer[2]=atoi(buffer[3]);
                           Serial.println(scan_buffer[0]);
                           Serial.println(scan_buffer[1]);
                           Serial.println(scan_buffer[2]);
                           //matrix(scan_buffer[0], scan_buffer[1], scan_buffer[2]);
                      }
                  
                   else
                    {
                      Serial.println("Setcell arguments are not valid!");
                      //Serial.println(buffer[1]);
                    }
  
              }
             
   
          //####################################### gate parser #####################################
   
         
        if((res = strcmp(buffer[0], ctrl3)==0) && (counter != 2) )
          {
         Serial.println("Not enough arguments for <setgsv> function!");
         Serial.println("Please specify voltage value (range: -2000/+2000 mV) followed by <setgsv>"); // setgsv
         Serial.println("Example: <setgsv:1000>");
        
          }
        else if ((res = strcmp(buffer[0], ctrl3)==0) && (counter = 2) )
                
                {
               
                    if(   (-2000<=atoi(buffer[1]) && atoi(buffer[1])<=2000)     )
                    {
                      Serial.println("Gate settings are OK!");
                          
                          gate_buffer[0]=(2000+(atoi(buffer[1])))/2.35+550;
                          
                           Serial.println(gate_buffer[0]);
                          
                   }
                  
                   else
                    {
                      Serial.println("Setgsv arguments are not valid!");
                      //Serial.println(buffer[1]);
                    }
  
              }
       //############################################## drain-source parser #################################
     
        if((res = strcmp(buffer[0], ctrl4)==0) && (counter != 2) ) ///1500 mV max equal to 1230 digit
          {
         Serial.println("Not enough arguments for <setdsv> function!");
         Serial.println("Please specify voltage value (range: 0 - 1500mV mV) followed by <setgsv>"); // setgsv
         Serial.println("Example: <setdsv:1000>");
        
          }
        else if ((res = strcmp(buffer[0], ctrl4)==0) && (counter = 2) )
                
                {
               
                    if(   (0<=atoi(buffer[1]) && atoi(buffer[1])<=4500)     )
                    {
                      Serial.println("Source-drain settings are OK!");
                          drain_buffer[0]=atoi(buffer[1])/1.220;
                          
                           Serial.println(drain_buffer[0]);
                          
                   }
                  
                   else
                    {
                      Serial.println("Setgsv arguments are not valid!");
                      //Serial.println(buffer[1]);
                    }
  
              }
       
       
       // ############################################### run scan  #####################################
          if((res = strcmp(buffer[0], ctrl1)==0) && (counter != 2) ) //
          {
           Serial.println("Command not valid!");
           Serial.println("Please specify voltage value (range: 0 - 1500mV mV) followed by <setgsv>"); // setgsv
           Serial.println("Example: <scan:ON>");
        
          }
             else if ((res = strcmp(buffer[0], ctrl1)==0) && (counter = 1) )
                
                {
               
                    if(   (res=strcmp(buffer[1], "ON")==0))
                    {
                      Serial.println("Running scan!");
                      matrix(scan_buffer[0], scan_buffer[1], scan_buffer[2]);    
                   } 
                  
                    else
                    {
                      Serial.println("Command not valid!");
                      
                    }
  
              }        
          if((res = strcmp(buffer[0], ctrl10)==0) && (counter = 1) ) //
          {
//          reboot();
          //Serial.println("YO!");
          }        
    //################################################## help ############################################
        if((res = strcmp(buffer[0], ctrl9)==0) && (counter != 1) ) //
          {
           Serial.println("Command not valid!");
        
          }
          else if ((res = strcmp(buffer[0], ctrl9)==0) && (counter = 1) )
             {
              help();
              }   
             
     //################################################ reboot ##########################################
      if((res = strcmp(buffer[0], ctrl10)==0) && (counter != 1) ) //
          {
           Serial.println("Command not valid!");
        
          }
          else if ((res = strcmp(buffer[0], ctrl10)==0) && (counter = 1) )
             {
              reboot();
              }                             

    //####################################################################################################
}

void parse(){         
   recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        //parseData();
        parsedata();
        //showParsedData();
        newData = false;
  
  }
}
 //###############################################################################3

 //################################## potentiometer ################################
 void potentiometer()
 {
  //use the SPI buses
  for (i = 0; i <= 255; i++)
    {
      spiCommand(vspi, i);
      delay(10);
    }
    delay(500);
    for (i = 255; i >= 0; i--)
    {
      spiCommand(vspi, i);;
      delay(10);
    }
  //spiCommand(vspi, 0b01010101); // junk data to illustrate usage
 // spiCommand(hspi, 0b11001100);
  delay(100);

  
  }
 //################################################################################
//################################ restart ##############################
void reboot(){
Serial.println("Restarting in 5 seconds");
Serial.println();
  for (int i =0; i<5; i++){
    Serial.print(" . ");
    delay(1000);
    }
  
  ESP.restart();
}
//######################################################################
 
void loop() {
  //use the SPI buses
   parse();
   potentiometer();
}