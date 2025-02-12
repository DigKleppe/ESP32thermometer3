#include <math.h>
#include <stdint.h>

#include "ntc.h"

float a, b, c, d;

float calcTemp(float fRntc)
{
	double temp;
	double rRel = fRntc / RNTC25;
	double rLog = log(rRel); // log = ln

	temp = (1.0 / (A1VALUE + B1VALUE * rLog + C1VALUE * rLog * rLog + D1VALUE * rLog * rLog * rLog)) - 273.15;
	return ((float)temp);
}
