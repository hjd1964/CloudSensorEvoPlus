# CloudSensorEvoPlus
Cloud and rain sensor for Arduino
This is the Cloud Sensor Evo Plus.

What you need:

One arduino based on the Atmega 328, like the Nano, Uno etc.

One IR sensor (MLX90614) to take readings of the sky, I bought mine on eBay, mounted on a breakout board. Note that the sensor have to be based on the
sensor that has a wider FOV - like the MLX90614BAA or similar, you can find the data sheet here if you want to take a closer look to the specifications. Download DataSheet Now
Ebay search - http://www.ebay.com/sch/i.html?_odkw=DB18B20+waterproof&_osacat=0&_from=R40&_trksid=p2045573.m570.l1313.TR0.TRC0.H0.Xmlx90614.TRS0&_nkw=mlx90614&_sacat=0

One temperature sensor (DB18B20) to read the ambient temp (ground temp).
Ebay search - http://www.ebay.com/sch/i.html?_odkw=DB18B20&_osacat=0&_from=R40&_trksid=p2045573.m570.l1313.TR0.TRC0.H0.XDB18B20+waterproof.TRS0&_nkw=DB18B20+waterproof&_sacat=0

Rain sensor
Ebay  - http://www.ebay.com/itm/Raindrops-Detection-sensor-modue-rain-module-weather-module-Humidity-For-Arduino-/400439668701?hash=item5d3c106fdd

USB heat pad, mounted in the waterproof enclosed to dry off the sensor after rain, and fight off dew.
Ebay - http://www.ebay.com/itm/USB-Heating-Heater-Winter-Warm-Plate-for-Shoes-Golves-Mouse-Pad-Brand-new-TS-/251744637652?pt=LH_DefaultDomain_0&hash=item3a9d2652d4

How to connect:

The MLX90614;
3V		-3V
GND		-GND
SCL		-A4
SDA		-A5

The DB18B20
5V		-5V
GND		-GND
”Yellow”	-D10

The Rain sensor
5V		-5V
GND		-GND
AO		-A0

The heater pad connects to USB Y-split cable inside the box.

The Arduino code:

The arduino code is called Cloud SensorEvoPlus.ino
First, you need the libraries (can be downloaded via the library handler in the arduino program)
- Onewire
- i2cmaster

Save the code and put the Command.ino -file in the same folder as the CloudSensorEvoPlus.ino
You can now compile and upload the code to the Arduino.

Install the ASCOM driver

Install the driver
Now you can connect the arduino to the computer and open up your Sequence program (all programs that uses ASCOM safety monitor) like ASA’s Sequence, SGPro etc.
In the ASCOM chooser dialog box, choose Cloud Sensor Evo Plus and click properties - if everything went ok, you should now se the values.

Fine tune the settings

To fine tune the cloud sensor, you can use this table to set when you want to trigger the unsafe-command.
The temperatures in the table is the delta temperature, the difference between the ambient temp and the cloud or sky temp
Let’s take an example:
If there is no cloud at all, there will be an temp difference between the ground and the sky of at least 25 degrees Celsius
If there is some clouds present, the diff will be roughly 21 degrees C. I have mine set on 21.

Mainly clear					Less than -28.0
Few clouds or high clouds			between -28.0 and -24.0
Some clouds					between -24.0 and -19.0
Mainly cloudy				between -19.0 and -17.0
Very cloudy					between -17.0 and -14.0
Overcast or fog				Higher than -14.0

The rain sensor, you have to try it out with some droplets of water so it suits you best, i have mine set to trigger ”Unsafe” after two small droplets.


- When you are happy with your settings, press ok in the ASCOM setup dialog window and ok in the ASCOM chooser dialog window.

