#include <math.h>
#include <stdint.h>

#include "ntc.h"

#define ADC_FULL_SCALE 0x7FFFFF

#pragma GCC optimize("0")
// for pull up based system
// float calcNTC( uint32_t adcVal ){
//	float fRntc;
////	fRntc = 10000 *  (float) adcVal / ( ADC_FULL_SCALE  - (float) adcVal);
//	fRntc = 10000 *  ( ADC_FULL_SCALE  - (float) adcVal) /(float) adcVal ;
//	return(fRntc);
//}

float a, b, c, d;

float calcTemp(float fRntc) {
  double temp;
  double rRel = fRntc / RNTC25;
  double rLog = log(rRel); // log = ln

  //	a = A1VALUE;
  //	b =  B1VALUE *rLog ;
  // = C1VALUE *rLog *rLog;
  //	d =  D1VALUE *rLog *rLog* rLog;
  ////	temp = 1.0 / ( (log(rRel)/BVALUE  + 1.0/298.0)) - 273.0; //log = ln
  //

  temp = (1.0 / (A1VALUE + B1VALUE * rLog + C1VALUE * rLog * rLog +
                 D1VALUE * rLog * rLog * rLog)) -
         273.15;
  //	temp = (1.0 / (AVALUE + BVALUE * rLog +  CVALUE * rLog *rLog* rLog) ) -
  //273.15;

  return ((float)temp);
}

// returnwaarde: temperatuur in graden C
// input sensor

// float readTemp(uint32_t adcVal){
//	float fRntc,fTemp;
//	fRntc = calcNTC(adcVal);
//	fTemp = calcTemp (fRntc);
//	return(fTemp);
// }
