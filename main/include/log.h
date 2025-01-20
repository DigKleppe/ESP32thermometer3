/*
 * log.h
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LOG_H_
#define MAIN_INCLUDE_LOG_H_

#include <stdint.h>
#include <time.h>


#define MAXLOGVALUES	24 * 60 / 5 // 24 hours of 5 minute logs

typedef struct {
	uint32_t timeStamp; // in 10ms
	float temperature[4];

} log_t;

extern int logRxIdx;
extern int logTxIdx;
extern uint32_t timeStamp;

int getAllLogsScript(char *pBuffer, int count);
int getNewLogsScript(char *pBuffer, int count);
void addToLog( log_t logValue);
void testLog( void) ;


#endif /* MAIN_INCLUDE_LOG_H_ */
