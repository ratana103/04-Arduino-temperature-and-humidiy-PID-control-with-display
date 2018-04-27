/////////////////////////////////////////////////////////////////////////
// Author: Ratana Lim                                                  //
// Updated date: 04/12/2018                                            //
// Arduino YUN Temperature and Humidity PID controller with Display    //
/////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <SHT1x.h>
#include <PID_v1.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SendCarriots.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <EEPROM.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define RED     0x1
#define YELLOW  0x3
#define GREEN   0x2
#define TEAL    0x6
#define BLUE    0x4
#define VIOLET  0x5
#define WHITE   0x7

#define HEAT_COOL_RELAY     8
#define FAN_TEMP_RELAY      4
#define FAN_HUMIDITY_RELAY  5
#define dataPin             6
#define clockPin            7

SHT1x sht1x(dataPin, clockPin);

// Define addresses of EEPROM 
#define ADDR_HEAT_COOL_FLAG   0
#define ADDR_SET_TEMP_UP      1
#define ADDR_SET_TEMP_DOWN    2

#define ADDR_SETHUMIDITY_UP   3
#define ADDR_SETHUMIDITY_DOWN 4
#define ADDR_FAN_TEMP         5
#define ADDR_FAN_HUMIDITY     6
#define ADDR_FAN_DURATION     7
#define ADDR_FAN_PAUSE        11
#define ADDR_HTTP_INTERVAL    10

const String APIKEY = "66c56768c38a24ed1b41cbd7c8b0abcde3dafde72eb761a69b79a3d79d396c2b";
const String DEVICE = "pizzaToppingOven@wmachugh.wmachugh";
const int numElements = 5;
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,75);
SendCarriots sender;

char check_Flag             = 1; // flag for checking EEPROM
char heat_Cool_Flag         = 1;
double set_Temp             = 0;
char set_Temp_Up;
char set_Temp_Down;
double set_Humidity         = 0;
char set_Humidity_Up;
char set_Humidity_Down;
char fan_Temp               = 0; 
char fan_Humidity           = 0;
unsigned char fan_Duration  = 0; // 5 minutes
unsigned char fan_Pause     = 0; // 5 minutes
unsigned char http_Interval = 0; // 5 minutes

unsigned long previous_Time = 0; // time for saving the settings
unsigned long current_Time  = 0; // time for saving the settings
unsigned long refresh_Time  = 0; // time for saving the settings
unsigned char select_Flag   = 0;
unsigned char state         = 0;
double Kp = 2, Ki = 5, Kd   = 1; // PID parameters
double temp_c;
double humidity;
double temp_Output;
double humidity_Output;
PID tempPID(&temp_c, &temp_Output, &set_Temp, Kp, Ki, Kd, DIRECT);
PID humidityPID(&humidity, &humidity_Output, &set_Humidity, Kp, Ki, Kd, DIRECT);

float get_temperature()
{
  temp_c = sht1x.readTemperatureC();
  return temp_c;
}
float get_humidity()
{
  humidity = sht1x.readHumidity();
  return humidity;
}

void state_Machine()
{       
  uint8_t buttons = lcd.readButtons();
  while(!buttons) 
  {
  	current_Time = millis();
  	refresh_Time = current_Time - previous_Time;
  	if(refresh_Time > 4000)
  	{
  		state = 0;
  		previous_Time = current_Time;
  		lcd.setCursor(0, 0);
      lcd.print("TEMPERATURE:");
      lcd.print(temp_c, DEC);
      lcd.setCursor(0, 1);
      lcd.print("HUMIDITY:");
      lcd.print(humidity);
  		break;
  	}
  	buttons = lcd.readButtons();
  }

  char  upFlag  = 0;          // flag for up button
  char downFlag = 0;          // flag for down button
  char setFlag  = 0;   		  // flag for select button
  char select_Flag = 0;
  previous_Time = millis();
  if (buttons)
  {
    char state_Flag;
   if (state_Flag)
   {
     state = 0;
   }
   else
   {
     state = 7;
   }
    lcd.clear();
    if (buttons & BUTTON_LEFT)
    {
     state_Flag = 1;
      state --;
    }
    if (buttons & BUTTON_RIGHT)
    {	
      state_Flag = 0;
       state ++;
    }

    select_Flag = state % 8;
      
    if(buttons & BUTTON_UP)
    {	
      upFlag = 1;
    }
    
    if(buttons & BUTTON_DOWN)
    {
      downFlag = 1;
    }
    
    if(buttons & BUTTON_SELECT)
    {
      setFlag = 1;
    }
    
    switch (select_Flag)
      {
          // case 0: //standard mode
          //    lcd.clear();
          //    lcd.setCursor(0, 0);
          //    lcd.print("TEMPERATURE:");
          //    lcd.print(temp_c, DEC);
          //    lcd.setCursor(0, 1);
          //    lcd.print("HUMIDITY:");
          //    lcd.print(humidity);
          //   break;

        case 0: // heat/cool mode
           if (heat_Cool_Flag)
           {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HEAT/COOL MODE");
           	lcd.setCursor(0, 1);
           	lcd.print("HEAT");           
           }
           else
           {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HEAT/COOL MODE");
            lcd.setCursor(0, 1);
            lcd.print("COOL");            	
           }
           if (upFlag)         
           {
            heat_Cool_Flag = 1;
            digitalWrite(HEAT_COOL_RELAY, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HEAT/COOL MODE");            
            lcd.setCursor(0, 1);
            lcd.print("HEAT");
           }
           if (downFlag)
           {
            heat_Cool_Flag = 0;
            digitalWrite(HEAT_COOL_RELAY, LOW);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HEAT/COOL MODE");            
            lcd.setCursor(0, 1);
            lcd.print("COOL");
           }
           if (setFlag)
           {
           	EEPROM.write(ADDR_HEAT_COOL_FLAG, heat_Cool_Flag);
           }
          break;

        case 1: // temperature set point mode (in Celsius)
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("TEMPERATURE");
           lcd.setCursor(0,1);
           lcd.print(set_Temp);
           if (upFlag)
           {
            set_Temp ++;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("TEMPERATURE");            
            lcd.setCursor(0,1);
            lcd.print(set_Temp);
           }
           if (downFlag)
           {
            set_Temp --;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("TEMPERATURE");            
            lcd.setCursor(0,1);
            lcd.print(set_Temp);           
           }
           if (setFlag)
           {
             set_Temp_Up = set_Temp * 100 / 100;
             set_Temp_Down = set_Temp * 100 - set_Temp_Up * 100;
  		       EEPROM.write(ADDR_SET_TEMP_UP, set_Temp_Up);
  		       EEPROM.write(ADDR_SET_TEMP_DOWN, set_Temp_Down);
           }
          break;

        case 2: // humidity set point mode (in percent)    
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("HUMIDITY");
           lcd.setCursor(0, 1);
           lcd.print(set_Humidity);             
           if (upFlag)
           {
             set_Humidity ++;
	           lcd.clear();
	           lcd.setCursor(0, 0);
	           lcd.print("HUMIDITY");
	           lcd.setCursor(0, 1);
	           lcd.print(set_Humidity);             
           }
           if (downFlag)
           {
             set_Humidity --;
	           lcd.clear();
	           lcd.setCursor(0, 0);
	           lcd.print("HUMIDITY");
	           lcd.setCursor(0, 1);
	           lcd.print(set_Humidity);              
           }
           if (setFlag)
           {
             set_Humidity_Up = set_Humidity * 100 / 100;
             set_Humidity_Down = set_Humidity * 100 - set_Humidity_Up * 100;
		         EEPROM.write(ADDR_SETHUMIDITY_UP, set_Humidity_Up);
  		       EEPROM.write(ADDR_SETHUMIDITY_DOWN, set_Humidity_Down);  				
           }
          break;

        case 3: // fan with temperature relay mode
           if (fan_Temp)
           {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-TEMP RELAY");           	
           	lcd.setCursor(0, 1);
           	lcd.print("YES");
           }
           else
           {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-TEMP RELAY");           	
           	lcd.setCursor(0, 1);
           	lcd.print("NO");
           }
           if (upFlag)
           {
            fan_Temp = 1;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-TEMP RELAY");            
            lcd.setCursor(0, 1);
            lcd.print("YES");
           }
           if (downFlag)
           {
            fan_Temp = 0;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-TEMP RELAY");            
            lcd.setCursor(0, 1);
            lcd.print("NO");           
           }
           if (fan_Temp)
            {
              digitalWrite(FAN_TEMP_RELAY, HIGH);
              // delay(fan_Duration*60*1000);
              digitalWrite(FAN_TEMP_RELAY, LOW);
              // delay(fan_Pause*60*1000);
            }
            else
            {
              digitalWrite(FAN_TEMP_RELAY, LOW);
            }                      
           if (setFlag)
           {
           	EEPROM.write(ADDR_FAN_TEMP, fan_Temp);
           }
          break;
          
        case 4: // fan with humidity relay mode
           if (fan_Humidity)
           {
           	lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-HUMI RELAY");
           	lcd.setCursor(0, 1);
           	lcd.print("YES"); 
           }
           else
           {
           	lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-HUMI RELAY");
           	lcd.setCursor(0, 1);
           	lcd.print("NO"); 
           }
           if (upFlag)
           {
            fan_Humidity = 1;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAN-HUMI RELAY");            
            lcd.setCursor(0, 1);
            lcd.print("YES");            
           }
           if (downFlag)
           {
            fan_Humidity = 0;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("FAN-HUMI-RELAY");
            lcd.setCursor(0, 1);
            lcd.print("NO");              
           }
           if (fan_Humidity)
            {
              digitalWrite (FAN_HUMIDITY_RELAY, HIGH);
              //delay (fan_Duration*1000);
              digitalWrite (FAN_HUMIDITY_RELAY, LOW);
              //delay (fan_Pause*1000);              
            }
            else
            {
              digitalWrite (FAN_HUMIDITY_RELAY, LOW);
            }
           if (setFlag)
           {
           	EEPROM.write(ADDR_FAN_HUMIDITY, fan_Humidity);
           }
          break;

        case 5: // auto fan duration (in minute)
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("AUTO FAN TIME");
           lcd.setCursor(0, 1);
           lcd.print(fan_Duration);
           lcd.print(" min");
           
           if (upFlag)
           {
            fan_Duration ++;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("AUTO FAN TIME");
            lcd.setCursor(0, 1);
            lcd.print(fan_Duration);
            lcd.print(" min");
           }
           if (downFlag)
           {
            fan_Duration --;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("AUTO FAN TIME");
            lcd.setCursor(0, 1);
            lcd.print(fan_Duration);
            lcd.print(" min");           
           }
           if (setFlag)
           {
            EEPROM.write(ADDR_FAN_DURATION, fan_Duration);
           }
          break;

        case 6: // auto fan pause (in minute)
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("AUTO FAN PAUSE");
           lcd.setCursor(0, 1);
           lcd.print(fan_Pause); 
           lcd.print(' min');
           
           if (upFlag)
           {
            fan_Pause ++;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("AUTO FAN PAUSE");
            lcd.setCursor(0, 1);
            lcd.print(fan_Pause);
            lcd.print(" min");           
           }
           if (downFlag)
           {
            fan_Pause --;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("AUTO FAN PAUSE");
            lcd.setCursor(0, 1);
            lcd.print(fan_Pause);
            lcd.print(" min");            
           }
           if (setFlag)
           {
            EEPROM.write(ADDR_FAN_PAUSE, fan_Pause);          
           }
          break;

        case 7: // HTTP log interval (in minute)
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("HTTP INTERVAL");
           lcd.setCursor(0,1);
           lcd.print(http_Interval);
           lcd.print(" min");
           if (upFlag)
           {
            http_Interval ++;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HTTP INTERVAL");
            lcd.setCursor(0, 1);
            lcd.print(http_Interval);
            lcd.print(" min");
           }
           if (downFlag)
           {
            http_Interval --;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HTTP INTERVAL");
            lcd.setCursor(0, 1);
            lcd.print(http_Interval);
            lcd.print(" min");
           }
           if (setFlag)
           {
             EEPROM.write(ADDR_HTTP_INTERVAL, http_Interval);
           }
          break;
      }
  }
  upFlag   = 0;
  downFlag = 0;
  setFlag  = 0;
  buttons  = 0;
}

void setup() {
  //Serial.begin(9600);
  Ethernet.begin(mac,ip);

  pinMode(HEAT_COOL_RELAY, OUTPUT);
  pinMode(FAN_TEMP_RELAY, OUTPUT);
  pinMode(FAN_HUMIDITY_RELAY, OUTPUT);
  
  tempPID.SetMode(AUTOMATIC);
  humidityPID.SetMode(AUTOMATIC);
  lcd.begin(16, 2);
  lcd.setBacklight(BLUE);
  
  lcd.setCursor(3, 0);
  lcd.print("WELCOME");
  lcd.setCursor(5, 1);
  lcd.print("...");
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("TEMPERATURE:");
  lcd.print(temp_Output);
  lcd.setCursor(0, 1);
  lcd.print("HUMIDITY:");
  lcd.print(humidity_Output);
}

void loop() 
{
	if (check_Flag)
	{
		heat_Cool_Flag = EEPROM.read(ADDR_HEAT_COOL_FLAG);
		set_Temp_Up = EEPROM.read(ADDR_SET_TEMP_UP);
		set_Temp_Down = EEPROM.read(ADDR_SET_TEMP_DOWN);
		set_Humidity_Up = EEPROM.read(ADDR_SETHUMIDITY_UP);
		set_Humidity_Down = EEPROM.read(ADDR_SETHUMIDITY_DOWN);
		fan_Temp = EEPROM.read(ADDR_FAN_TEMP);
		fan_Humidity = EEPROM.read(ADDR_FAN_HUMIDITY);
		fan_Duration = EEPROM.read(ADDR_FAN_DURATION);
		fan_Pause = EEPROM.read(ADDR_FAN_PAUSE);
		http_Interval = EEPROM.read(ADDR_HTTP_INTERVAL);
		set_Temp = set_Temp_Up + set_Temp_Down/100;
		set_Humidity = set_Humidity_Up + set_Humidity_Down/100;
		check_Flag = 0;
	}

  temp_c = get_temperature();
  humidity = get_humidity();

  tempPID.Compute();
  humidityPID.Compute();
  if (set_Temp > temp_Output)
  {
    digitalWrite(HEAT_COOL_RELAY, HIGH);
  }
  else
  {
    digitalWrite(HEAT_COOL_RELAY, LOW);
  }
  
  state_Machine();

  // String array[numElements][2] = {{"TEMPERATURE", temp_c}, {"HUMIDITY", humidity}, {"HEAT/COOL MODE", "YES"}, {"FAN-TEMP RELAY", "YES"}, {"FAN-HUMIDITY RELAY", "YES"}};
  // Serial.println(sender.send(array, numElements, APIKEY, DEVICE));
  }
