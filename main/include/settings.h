/*
 * settings.h
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "esp_system.h"
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include "cgiScripts.h"
#include "measureTask.h"

#define MAX_STRLEN 32
#define USERSETTINGS_CHECKSTR "ttest-6"

typedef struct {
  char spiffsVersion[16]; // holding current version
  float temperatureOffset[NR_NTCS];
  char moduleName[MAX_STRLEN + 1];
  int resolution;
  int middleInterval;
  int logInterval;
  char checkstr[MAX_STRLEN + 1];
} userSettings_t;

// #ifndef varType_t
// typedef enum { FLT, STR, INT , DESCR , CALVAL} varType_t;
// #endif

typedef struct {
  varType_t varType;
  int size;
  void *pValue;
  int minValue;
  int maxValue;
} settingsDescr_t;

//extern const settingsDescr_t settingsDescr[];
extern bool settingsChanged;

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t saveSettings(void);
esp_err_t loadSettings(void);

#ifdef __cplusplus
}
#endif

extern userSettings_t userSettings;
void parseCGIsettings (char *buf, int received); 

#endif /* SETTINGS_H_ */
