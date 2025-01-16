#ifndef NTCH
#define NTCH

float calcTemp(float fRntc);
float calcNTC(uint32_t adcVal);
float updateNTC(uint32_t ID, uint32_t adcVal);
float resToTemp(float Rntc);

extern float NTCreferenceValue;
#define ERRORTEMP -9999.0

//// for vishay NTCLE100E3 104 ( 100k 2% )
// #define RNTC25 		100000.0
// #define BVALUE 		4190.0
// #define A1VALUE		3.354016E-03
// #define B1VALUE		2.519107E-04
// #define C1VALUE		3.510939E-06
// #define D1VALUE	   	1.105179E-07

// for vishay	NTCLE203E3 103SBO

#define RNTC25 10000.0
// #define B25_85 VALUE 		39770.0
//  from NTC calculator https://www.vishay.com/thermistors/ntc-rt-calculator/

#define AVALUE 0.00335401643468053
#define BVALUE 0.000256523550896126
#define CVALUE 0.00000260597012072052
#define DVALUE 0.000000063292612648746

#define A1VALUE 0.00335401643468053
#define B1VALUE 0.000256523550896126
#define C1VALUE 2.60597012072052E-06
#define D1VALUE 6.3292612648746E-08

#endif
