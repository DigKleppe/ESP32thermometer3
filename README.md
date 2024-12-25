UNDER CONSTRUCTION

ESP32 thermometer
====================

Uses RC to to get a high resolution AD converter for measuring NTCs
Hardware:
Film capacitor 0.33uF (not critical) to port-pin PC  and gnd
reference resistor 10k and
4 ntc's (or more) 10k
one pin connected to port pin and one pin connected to the capacitor port pin PC.

The capacitor C is charged.
A timer is started and C is discharged. first through te reference resistor and a second time through the NTC.
The voltage level of the C is sensed by common port PC. An interrupt is fired when the level is read 0. The timer value is the RC value.
The ratio of the ref and ntc is the ratio of both timings.

 

