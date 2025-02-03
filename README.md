# ESP32 4 channel high resolution and accurate NTC thermometer

This project uses a dual slope approach to achive high resolution and accurate resistor measurements with a minimal nr of components.
The ADC of the ESP is not very accurate. 
An simple RC is used in combination with a internal ESP timer and a comparator to measure the RC time.
The comparater output is connected to an (high priority) interrupt input.
The process is simple: 
Charge the capacitor to VCC.
Start the timer, and discharge throuh a known reference resitor.
When the voltage level of the C is lower than the comparator reference the interrupt is fired.
The timer stops an the RC time is read.
The same steps for the NTCs.
The resistorvalues of the NTCs are proportional to the RC times.
The temperarures are calculated.

The hardware is not critical. A Filmcapacitor of 1uF and a reference resitor of 10k is used.
For optimal accuracy the value of the referency resistor must be measured and set in measureTask.h (RREF)
As comparator a LM358 is used. Maybe a real (CMOS) comparator gives even better results. If you don't need 
a very high resolution (now about 0.001 C!) you can ommit the comparator.

When measuring a fixed resistor the maximun noise is about +- 0.002 degrees C, depending of averaging.
The accuracy (-10 - 25 degrees C) is better than 0.02 degrees.
At 50 degrees (lower RC values) the deviation is 0.2 degrees. 

I use Vishay 10k 1% NTCs NTCLE203E3 103SBO.  No problem to use long wires.
To use another NTC you have to provide the parameters in NTC.h.

The nr of NTCs can be more than the 4 used here. Use IOports that don't have internal connections in the ESP.




## Website
A website is embedded to show the results in google charts.
To set the system to your network use ESP touch.

Also some settings are provided. You can set offsets to compensate for difference in NTCs. Typical 0.2 degrees max.
I use a (home made ESP) thermometer with a +- 0.1 degree accurate TMP117 sensor to calibrate.

## Project

The project is made in visual studio code. No arduino , LVGL version 9,  IDF version 5.4. 

