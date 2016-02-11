// User options

// ------------------------------------------------------------------------------------------------------------------
// SENSORS

// the DS18B20 temperature sensor is on Digital 9 and is accessed using OneWire.h
#define DS18B20_ON    // simulated if _OFF
// the MLX90614, HTU21D, and BMP180 I2C sensors are at the default I2C pins SDA SCL
#define MLX90614_ON   // simulated if _OFF
#define HTU21D_OFF    // disabled if _OFF
#define BMP180_OFF    // disabled if _OFF

// Using this option the built-in pullup resistors should be turned off immediately *after* the library connects,
// if you don't like this situation use a level-converter.  This hasn't been tested and is coded to work only on
// the Mega328 and Mega2560
#define I2C_PULLUPS_ON // set to _OFF to disable the built-in I2C pullups, default=_ON

// ------------------------------------------------------------------------------------------------------------------
// CONFIGURATION

// altitude in meters
#define ALTITUDE 130

// lowest and highest rain sensor readings
// maps to 0 to 3 for Raining, Warning, Dry, Dry
#define RainSensorMin 0     // sensor minimum: 10 bit DAC=0, 0v
#define RainSensorMax 1024  // sensor maximum: 10 bit DAC=1023, 5v (or 3.3v)

// cloud status indication
#define SkyClear 28.0       // Mainly Clear
#define SkyVSCldy 24.0      // Few or High Clouds
#define SkySCldy 19.0       // Some Clouds
#define SkyCldy 17.0        // Mainly Cloudy
#define SkyVCldy 14.0       // Very Cloudy (< this is Overcast or Fog)

// these define the lower limits for the rain resistance and cloud temperature that's considered "SAFE"
#define WetThreshold 0.2
#define CloudThreshold 16.0

#define PlotAvgDeltaTemp_ON // _ON plots the averaged temperature difference, _OFF plots the temperature difference, default=_ON

// Adjust the log resolution here, must be in 2's 2,4,6,8...120,122
// keep in mind that EEPROM is spec'd to last for 100,000 writes
// since a given location gets written to once in 64 readings that amounts to 
// a write of a given location once every 2 hours (64 * 120 seconds) * 100,000 which is 22 years
// at 60 it's 11 years, at 30 5 years life.  These are minimums according to the spec.
#define SecondsBetweenLogEntries 30

// this is the response time required to cover approximately 2/3 of a change in cloud temperature
// adjust higher for less sensitivity to passing clouds/changing conditions, lower for more sensitivity
#define AvgTimeSeconds 600.0

// ------------------------------------------------------------------------------------------------------------------
// NETWORK

// turn the ethernet web-server and command channel _ON, default=_OFF
#define W5100_OFF
// if DHCP is used to obtain the IP addresses, the addresses are overridden, default=_OFF
#define ETHERNET_USE_DHCP_OFF
// Enable chart of readings
#define HTML_CHART_ON

// default IP, Gateway, and subnet
#ifdef W5100_ON
#include "Ethernet.h"
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 56);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#endif
