UNDER CONSTRUCTION

ESP32 thermometer
====================

Uses RC to to get a high resolution AD converter for measuring NTCs.
Hardware:
Film capacitor 0.33uF (not critical) to port-pin PC  and gnd.
reference resistor 10k and 4 ntc's (or more) 10k.
One pin connected to port pin and one pin connected to the capacitor port pin PC.

The capacitor C is charged.
A timer is started and C is discharged. first through te reference resistor and a second time through the NTC.
The voltage level of the C is sensed by common port PC. An interrupt is fired when the level is read 0. The timer value is the RC value.
The ratio of the ref and ntc is the ratio of both timings.

Note: I used a an ESP32-S3 running at 240 Mhz.
If using an other device note that ports can have an internal connection that influences the measurement. Use only simple GPIO ports.

results:
*10084  0: 24.87        stdev:0.08      *14964  1: 16.02        *32832  2: -0.01        *12597  3: 19.72
*10078  0: 24.87        stdev:0.06      *14982  1: 16.02        *32620  2: -0.01        *12528  3: 19.72
*10060  0: 24.87        stdev:0.06      *14976  1: 16.02        *32718  2: -0.01        *12538  3: 19.72
*10071  0: 24.87        stdev:0.06      *14957  1: 16.02        *32715  2: -0.02        *12623  3: 19.73
*10029  0: 24.87        stdev:0.06      *14967  1: 16.02        *32695  2: -0.02        *12616  3: 19.73
*10076  0: 24.87        stdev:0.06      *15023  1: 16.02        *32524  2: -0.02        *12555  3: 19.73
*10080  0: 24.87        stdev:0.06      *14997  1: 16.02        *32571  2: -0.02        *12616  3: 19.74
*10074  0: 24.87        stdev:0.06      *15006  1: 16.02        *32610  2: -0.02        *12619  3: 19.74
*10087  0: 24.87        stdev:0.06      *15120  1: 16.02        *32637  2: -0.02        *12538  3: 19.74
*10066  0: 24.87        stdev:0.06      *15020  1: 16.02        *32999  2: -0.02        *12542  3: 19.74
*10071  0: 24.86        stdev:0.05      *14970  1: 16.02        *32450  2: -0.02        *12711  3: 19.75
*10063  0: 24.86        stdev:0.05      *14871  1: 16.02        *32604  2: -0.02        *12581  3: 19.75
*10043  0: 24.86        stdev:0.05      *15101  1: 16.02        *32728  2: -0.02        *12532  3: 19.75
*10069  0: 24.86        stdev:0.05      *14982  1: 16.01        *32656  2: -0.02        *12556  3: 19.75
*10015  0: 24.86        stdev:0.05      *14994  1: 16.01        *32610  2: -0.02        *12690  3: 19.76
*10069  0: 24.86        stdev:0.05      *14979  1: 16.01        *32706  2: -0.02        *12595  3: 19.76
*10081  0: 24.86        stdev:0.05      *15072  1: 16.01        *32694  2: -0.02        *12597  3: 19.76
*10029  0: 24.86        stdev:0.05      *15018  1: 16.01        *32721  2: -0.02        *12564  3: 19.76
*10073  0: 24.86        stdev:0.04      *14961  1: 16.01        *32607  2: -0.02        *12556  3: 19.76
*10056  0: 24.86        stdev:0.04      *14978  1: 16.01        *32596  2: -0.02        *12516  3: 19.76
*10071  0: 24.86        stdev:0.04      *14902  1: 16.01        *32643  2: -0.02        *12487  3: 19.77
*10088  0: 24.86        stdev:0.04      *14985  1: 16.01        *32556  2: -0.02        *12458  3: 19.77
*10057  0: 24.86        stdev:0.04      *15009  1: 16.01        *32530  2: -0.02        *12466  3: 19.77
*10080  0: 24.86        stdev:0.04      *14935  1: 16.01        *32612  2: -0.02        *12465  3: 19.78

 The first item marked * is the timervalue. 0-1-2 are fixed resistors. No 3 is a real NTC. This sensor sees me sitting next to it!
 The values are averaged  

