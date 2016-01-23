/**
  This sketch reads three sensors:
   DS18B20 - Connected to D9
   MLX90614 - Connected to SCL-A5, SDA-A4
   Rain sensor conected to A0
   
   and supports the Arduino Ethernet Shield (WIZnet W5100)
   
  It calculates the temperatures (DS18B20 and MLX90614)
  DS18B20: http://playground.arduino.cc/Learning/OneWire
  MLX90614: http://bildr.org/2011/02/mlx90614-arduino/
*/

#define DEBUG_MODE_OFF_I2C
#define DEBUG_MODE_OFF_ONEWIRE

// default IP,Gateway,subnet are in the Network.ino file
// if ethernet is available DHCP is used to obtain the IP address (default addresses are overridden), default=OFF
// uncomment the next two lines to enable the ethernet web-server
//#define W5100_ON
//#include "Ethernet.h"
#define ETHERNET_USE_DHCP_OFF
#define HTML_CHART_ON

// --------------------------------------------------------------------------------------------------------------
#define FirmwareName "CloudSensorEvoPlus"
#define FirmwareNumber "0.21"

#ifdef DEBUG_MODE_OFF_ONEWIRE
//get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
#include "OneWire.h"
#endif

#ifdef DEBUG_MODE_OFF_I2C
#include <Wire.h>
//get it here: https://learn.adafruit.com/using-melexis-mlx90614-non-contact-sensors/wiring-and-test
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif

#include "EEPROM.h"
//#include <SPI.h>

// lowest and highest rain sensor readings:
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum

// misc
#define invalid -999
#define CHKSUM0_OFF

// last rain sensor reading
int rainSensorReading = invalid;
float rainSensorReading2 = invalid;

// last cloud sensor reading
float ds18b20_celsius = invalid;
float MLX90614_celsius = invalid;
float delta_celsius = invalid;
float avg_delta_celsius = 0;

#ifdef DEBUG_MODE_OFF_ONEWIRE
OneWire  ds(9);  // DS18B20 on Arduino pin 9
#endif

byte ds18b20_addr[8];

void setup(void)
{
  Serial.begin(9600);

  init_DS18B20();
  init_MLX90614();

#if defined(W5100_ON)
  // get ready for Ethernet communications
  Ethernet_Init();
#endif

  //  Serial.println("Init. Done.");
}

void init_DS18B20()
{
  //  Serial.println("Initializing DS18B20 sensor...");
#ifdef DEBUG_MODE_OFF_ONEWIRE
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
#ifdef DEBUG_MODE_OFF_I2C
    mlx.begin();
#endif
  //  PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups if you use 5V sensors and don't have external pullups in the circuit
}

long TimeSeconds = 0;
long last = 0;
int log_pos = 0;
int log_count = 0;

float sa=0.0;
float ss=0.0;
float sad=0.0;

void loop(void)
{
  long now = millis();

  // gather data from sensors once every two seconds
  if ((now-last)>2000L) {
        Serial.print(".");
    last = now; // blocks calling more than once during the same ms

    // Cloud sensor ------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
#ifdef DEBUG_MODE_OFF_ONEWIRE
    ds18b20_celsius = read_DS18B20();
#else
    ds18b20_celsius=20.1;  //ground
    delta_celsius=8.1;     //cloud
#endif
#ifdef DEBUG_MODE_OFF_I2C
    MLX90614_celsius = read_MLX90614();

    delta_celsius = abs(ds18b20_celsius - MLX90614_celsius);
    avg_delta_celsius = ((avg_delta_celsius*299.0) + delta_celsius)/300.0;
#else
    MLX90614_celsius=11.5; //sky
    delta_celsius=8.1;     //cloud
    avg_delta_celsius = ((avg_delta_celsius*299.0) + delta_celsius)/300.0;
#endif
    
    // short-term average ambient temp
    sa = ((sa*29.0) + ds18b20_celsius)/30.0;
    // short-term sky temp
    ss = ((ss*29.0) + MLX90614_celsius)/30.0;
    // short-term average diff temp
    sad = ((sad*29.0) + delta_celsius)/30.0;

    // End cloud sensor

    // Rain sensor -------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
    // read the sensor on analog A0:
    int sensorReading = analogRead(A0);

    // map the sensor range (four options):
    // ex: 'long int map(long int, long int, long int, long int, long int)'
    rainSensorReading = map(sensorReading, sensorMin, sensorMax, 0, 3);
    rainSensorReading2 = (float)sensorReading / 1023.0;
    // End rain sensor

    TimeSeconds=TimeSeconds+2;
    Serial.println(log_pos);

    // Logging ------------------------------------------------------------------
    // two minutes between writing values
    if (TimeSeconds%120==0) { // 120
      EEPROM_writeQuad(log_pos,(byte*)&sa);
      log_pos=log_pos+4; if (log_pos>1024) log_pos-=1024;
      EEPROM_writeQuad(log_pos,(byte*)&ss);
      log_pos=log_pos+4; if (log_pos>1024) log_pos-=1024;
      EEPROM_writeQuad(log_pos,(byte*)&sad);
      log_pos=log_pos+4; if (log_pos>1024) log_pos-=1024;
      EEPROM_writeQuad(log_pos,0);
      log_pos=log_pos+4; if (log_pos>1024) log_pos-=1024;

      log_count++; if (log_count>64) log_count==64;
    }
  }

  processCommands();
}

#ifdef DEBUG_MODE_OFF_ONEWIRE
float read_DS18B20()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];

  float celsius;

  if (OneWire::crc8(ds18b20_addr, 7) != ds18b20_addr[7])
  {
    Serial.println("CRC is not valid!");
    return -300.0f;
  }


  // the first ROM byte indicates which chip
  switch (ds18b20_addr[0])
  {
    case 0x10: type_s = 1; break;
    case 0x28: type_s = 0; break;
    case 0x22: type_s = 0; break;
    default:
      return -301.0f;
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

#ifdef DEBUG_MODE_OFF_I2C
float read_MLX90614()
{
  return mlx.readObjectTempC();
}
#endif
