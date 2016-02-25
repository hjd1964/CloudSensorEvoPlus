/**
  This sketch reads up to five sensors:
  DS18B20 - Connected to D9
  MLX90614, and optionally BMP180 and/or HTU21D - Connected to SCL and SDA (A4 and A5 on an UNO, pins 21 and 20 on a Mega2560)
  Rain sensor conected to A0

  and supports the Arduino Ethernet Shield (WIZnet W5100 and SD CARD)

  It detects the presence of clouds, rain, the ambient temperature, humidity, and barometric pressure
  DS18B20: http://playground.arduino.cc/Learning/OneWire
  MLX90614: http://bildr.org/2011/02/mlx90614-arduino/
  BMP180: https://learn.sparkfun.com/tutorials/bmp180-barometric-pressure-sensor-hookup-?_ga=1.90764189.629045862.1424710986
  HTU21D: https://learn.sparkfun.com/tutorials/htu21d-humidity-sensor-hookup-guide?_ga=1.90764189.629045862.1424710986
*/

// --------------------------------------------------------------------------------------------------------------
#define FirmwareName "CloudSensorEvoPlus"
#define FirmwareNumber "0.30"

#include "Config.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "EEPROM.h"
#ifdef DS18B20_ON
//get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
#include "OneWire.h"
#endif

// misc
#define invalid -999
#define CHKSUM0_OFF
boolean valid_BMP180 = false;
boolean valid_HTU21D = false;

// last humidity sensor reading
float pressureSensorReading = invalid;
float humiditySensorReading = invalid;

// last rain sensor reading
int rainSensorReading = invalid;
float rainSensorReading2 = invalid;

// last cloud sensor reading
float ds18b20_celsius = invalid;
float MLX90614_celsius = invalid;
float delta_celsius = invalid;
float avg_delta_celsius = 0;

// etc.
bool sdReady = false;
byte TimeSeconds = 0;
long last = 0;
int log_pos = 0;
byte log_count = 0;

float sa=0.0;
float ss=0.0;
float sad=0.0;
float lad=0.0;

#ifdef MLX90614_ON
//get it here: https://learn.adafruit.com/using-melexis-mlx90614-non-contact-sensors/wiring-and-test
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif

#ifdef DS18B20_ON
OneWire  ds(9);  // DS18B20 on Arduino pin 9
#endif
byte ds18b20_addr[8];

#ifdef BMP180_ON
#include "SFE_BMP180.h"
SFE_BMP180 pressure;
#endif

#ifdef HTU21D_ON
#include "SparkFunHTU21D.h"
HTU21D humidity;
#endif

void setup(void)
{
  // turn off the pullups
#if defined(__AVR_ATmega2560__) && defined(I2C_PULLUPS_OFF)
  digitalWrite(20,LOW);
  digitalWrite(21,LOW);
#endif
#if defined(__AVR_ATmega328__) && defined(I2C_PULLUPS_OFF)
  digitalWrite(A4,LOW);
  digitalWrite(A5,LOW);
#endif

  Serial.begin(9600);

  init_BMP180();
  init_HTU21D();
  init_DS18B20();
  init_MLX90614();

#if defined(W5100_ON)
  // get ready for Ethernet communications
  Ethernet_Init();
#endif

#ifdef SD_CARD_ON
  sdReady=SD.begin(4);
#else
  // disable the SDCARD if not using
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
#endif

  randomSeed(analogRead(0));

  //  Serial.println("Init. Done.");
}

void loop(void)
{
  long now = millis();

  // gather data from sensors once every two seconds
     
   if ((now-last)>2000L) {
//        Serial.print(".");
    last = now; // time of last call

    // Cloud sensor ------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
#ifdef DS18B20_ON
    ds18b20_celsius = read_DS18B20();
#else
    ds18b20_celsius=random(20,25);    //ground
    MLX90614_celsius=random(-1,10);   //sky
#endif
#ifdef MLX90614_ON
    MLX90614_celsius = read_MLX90614();
#else
    MLX90614_celsius=random(-1,10);  //sky
#endif
    delta_celsius = abs(ds18b20_celsius - MLX90614_celsius);
    avg_delta_celsius = ((avg_delta_celsius*(AvgTimeSeconds/2.0-1.0)) + delta_celsius)/(AvgTimeSeconds/2.0);
    
    // short-term average ambient temp
    sa = ((sa*((double)SecondsBetweenLogEntries/2.0-1.0)) + ds18b20_celsius)/((double)SecondsBetweenLogEntries/2.0);
    // short-term sky temp
    ss = ((ss*((double)SecondsBetweenLogEntries/2.0-1.0)) + MLX90614_celsius)/((double)SecondsBetweenLogEntries/2.0);
    // short-term average diff temp
    sad = ((sad*((double)SecondsBetweenLogEntries/2.0-1.0)) + delta_celsius)/((double)SecondsBetweenLogEntries/2.0);
    // long-term average diff temp
    lad=avg_delta_celsius;
    // End cloud sensor

    // Rain sensor -------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
    // read the sensor on analog A0:
    int sensorReading = analogRead(A0);

    // map the sensor range (four options):
    // ex: 'long int map(long int, long int, long int, long int, long int)'
    rainSensorReading = map(sensorReading, RainSensorMin, RainSensorMax, 0, 3);
    rainSensorReading2 = (float)sensorReading / 1023.0;
    // End rain sensor

    TimeSeconds+=2;
//    Serial.println(log_pos);

    // Pressure ----------------------------------------------------------------
#ifdef BMP180_ON
  if (valid_BMP180) pressureSensorReading = read_BMP180(); else pressureSensorReading=invalid;
#endif

    // Humidity ----------------------------------------------------------------
#ifdef HTU21D_ON
  if (valid_HTU21D) humiditySensorReading = read_HTU21D(); else humiditySensorReading=invalid;
#endif

    // Logging ------------------------------------------------------------------
    // two minutes between writing values
#if defined(MLX90614_ON) && defined(DS18B20_ON)
    if (TimeSeconds%SecondsBetweenLogEntries==0) { // x seconds
#else
    if (TimeSeconds%4==0) { // 4 seconds
      log_count=64;
#endif
      TimeSeconds=0;    
      EEPROM_writeQuad(log_pos,(byte*)&sa);
      log_pos+=4;
      EEPROM_writeQuad(log_pos,(byte*)&ss);
      log_pos+=4;
 #ifdef PlotAvgDeltaTemp_ON
     EEPROM_writeQuad(log_pos,(byte*)&lad);
 #else
     EEPROM_writeQuad(log_pos,(byte*)&sad);
 #endif
      log_pos+=4;
      EEPROM_writeQuad(log_pos,0);
      log_pos+=4;
      if (log_pos>=1024) log_pos-=1024;
      
      log_count++; if (log_count>64) log_count==64;

      sa=ds18b20_celsius;
      ss=MLX90614_celsius;
      sad=delta_celsius;
    }
  }

  processCommands();
}

void init_HTU21D()
{
  //  Serial.println("Initializing HTU21D sensor...");
#ifdef HTU21D_ON
  humidity.begin();
  valid_HTU21D=true;
#endif
}

void init_BMP180()
{
  //  Serial.println("Initializing BMP180 sensor...");
#ifdef BMP180_ON
  valid_BMP180=pressure.begin();
#endif
}

void init_DS18B20()
{
  //  Serial.println("Initializing DS18B20 sensor...");
#ifdef DS18B20_ON
  while ( !ds.search(ds18b20_addr))
  {
    ds.reset_search();
    delay(250);
  }
#endif
}

void init_MLX90614()
{
  //  Serial.println("Initializing MLX90614 sensor...");
#ifdef MLX90614_ON
    mlx.begin();
#endif
  //  PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups if you use 5V sensors and don't have external pullups in the circuit
}

#ifdef DS18B20_ON
float read_DS18B20()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];

  float celsius;

  if (OneWire::crc8(ds18b20_addr, 7) != ds18b20_addr[7])
  {
//    Serial.println("CRC is not valid!");
    return invalid;
  }


  // the first ROM byte indicates which chip
  switch (ds18b20_addr[0])
  {
    case 0x10: type_s = 1; break;
    case 0x28: type_s = 0; break;
    case 0x22: type_s = 0; break;
    default:
      return invalid;
//      return -301.0f;
  }

  ds.reset();
  ds.select(ds18b20_addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(ds18b20_addr);
  ds.write(0xBE);         // Read Scratchpad

  //read 9 data bytes
  for ( i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s)
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;

  return celsius;
}
#endif

#ifdef MLX90614_ON
float read_MLX90614()
{
  return mlx.readObjectTempC();
}
#endif

#ifdef HTU21D_ON
float read_HTU21D()
{
  float f=humidity.readHumidity();
  if (f>=997) return invalid; else return f;
}
#endif

#ifdef BMP180_ON
float read_BMP180()
{
  double T = invalid;
  double P = invalid;
  int status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
/*      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.print(" deg C, ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");
*/      
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
  /*        
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          Serial.print(P,2);
          Serial.print(" mb, ");
          Serial.print(P*0.0295333727,2);
          Serial.println(" inHg");
   */
          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          double p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          return p0;
  /*        
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");
   */
        }
        else return invalid;
      }
      else return invalid;
    }
    else return invalid;
  }
  else return invalid;
}
#endif
