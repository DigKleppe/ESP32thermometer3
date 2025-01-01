/*
 * log.cpp
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */
#include "log.h"
#include <stdint.h>

uint32_t  timeStamp=1;
int dayLogRxIdx;
int dayLogTxIdx;

//int logPrescaler = LOGINTERVAL;

log_t accumulator;
log_t dayLog[ MAXDAYLOGVALUES];

extern int scriptState;

void addToLog( log_t logValue)
{
	logValue.timeStamp = ++timeStamp;

	dayLog[dayLogTxIdx] = logValue;
	dayLogTxIdx++;
	if (dayLogTxIdx >= MAXDAYLOGVALUES)
		dayLogTxIdx = 0;

}

// reads all avaiable data from log
// issued as first request.

// reads all avaiable data from log
// issued as first request.

int getDayLogScript(char *pBuffer, int count) {
	static int oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		dayLogRxIdx = 0;
		if( dayLog[dayLogRxIdx].timeStamp == 0)
			return 0; // empty

		oldTimeStamp = 0;
		for (n = 0; n < MAXDAYLOGVALUES; n++) {
			if (dayLog[dayLogRxIdx].timeStamp < oldTimeStamp)
				break;
			else {
				oldTimeStamp = dayLog[dayLogRxIdx++].timeStamp;
			}
		}
		if (dayLog[dayLogRxIdx].timeStamp == 0) { // then log not full
			// not written yet?
			dayLogRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXDAYLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		if (logsToSend) {
			do {
				len += sprintf(pBuffer + len, "%ld,", dayLog[dayLogRxIdx].timeStamp);
				for ( n = 0 ; n < NR_NTCS; n++)
					len += sprintf(pBuffer + len, "%3.2f,", dayLog[dayLogRxIdx].temperature[n]- userSettings.temperatureOffset[n]);

				len += sprintf(pBuffer + len, "%3.3f\n", dayLog[dayLogRxIdx].refTemperature);

				dayLogRxIdx++;
				if (dayLogRxIdx >= MAXDAYLOGVALUES)
					dayLogRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > 40));
		}
	}
	return len;
}

