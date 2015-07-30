/**
* This sketch reads three sensors:
*  DS18B20 - Connected to D10
*  MLX90614 - Connected to SCL-A5, SDA-A4
*  Rain sensor conected to A0

* It calculates the temperatures (DS18B20 and MLX90614)
* DS18B20: http://playground.arduino.cc/Learning/OneWire
* MLX90614: http://bildr.org/2011/02/mlx90614-arduino/
*/

#define DEBUG_MODE_OFF

#ifdef DEBUG_MODE_OFF

//get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <OneWire.h>

//get it here: http://jump.to/fleury
#include <i2cmaster.h>

#include <SPI.h>

#endif

// lowest and highest rain sensor readings:
const int sensorMin = 0;     // sensor minimum
const int sensorMax = 1024;  // sensor maximum

// misc
#define FirmwareName "CloudSensorEvoPlus"
#define FirmwareNumber "0.1"
#define invalid -999
#define CHKSUM0_OFF

// last rain sensor reading
int rainSensorReading = invalid;

// last cloud sensor reading
float ds18b20_celsius = invalid;
float MLX90614_celsius = invalid;
float delta_celsius = invalid;

#ifdef DEBUG_MODE_OFF
OneWire  ds(10);  // DS18B20 on Arduino pin 10
#endif

byte ds18b20_addr[8];

void setup(void) 
{
  Serial.begin(9600);

  init_DS18B20();
  init_MLX90614();
}

void init_DS18B20()
{
//  Serial.println("Initializing DS18B20 sensor...");
#ifdef DEBUG_MODE_OFF
  while( !ds.search(ds18b20_addr)) 
  {
    ds.reset_search();
    delay(250);
  }
#endif
}


void init_MLX90614()
{
//  Serial.println("Initializing MLX90614 sensor...");
#ifdef DEBUG_MODE_OFF
  i2c_init(); //Initialise the i2c bus
#endif
  //  PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups if you use 5V sensors and don't have external pullups in the circuit
}

long last = 0;

void loop(void) 
{
  long now=millis();

  // gather data from sensors once a second
  if ((now%1000==0) && (last!=now)) {
    last=now; // blocks calling more than once during the same ms

    // Cloud sensor ------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
    #ifdef DEBUG_MODE_OFF
    ds18b20_celsius = read_DS18B20();
    MLX90614_celsius = read_MLX90614();
    delta_celsius = abs(ds18b20_celsius - MLX90614_celsius);
    #endif
    // End cloud sensor

    // Rain sensor -------------------------------------------------------------
    // it might be a good idea to add some error checking and force the values to invalid if something is wrong
    // read the sensor on analog A0:
    int sensorReading = analogRead(A0);
  
    // map the sensor range (four options):
    // ex: 'long int map(long int, long int, long int, long int, long int)'
    rainSensorReading = map(sensorReading, sensorMin, sensorMax, 0, 3);
    // End rain sensor
  }

  processCommands();
}

#ifdef DEBUG_MODE_OFF
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

float read_MLX90614()
{
  int dev = 0x5A<<1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;

  i2c_start_wait(dev+I2C_WRITE);
  i2c_write(0x07);

  // read
  i2c_rep_start(dev+I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();

  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point

  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor)-0.01;

  float celcius = tempData - 273.15;
  return celcius;
}
#endif
