/*
 * httpd_cgi.h
 *
 *  Created on: 26 jul. 2012
 *      Author: dig
 */

#ifndef HTTPD_CGI_H_
#define HTTPD_CGI_H_

#include "../../http/include/httpd.h"
#define CGIRETURNFILE "/CGIreturn.txt"

typedef enum { FLT, STR, INT, DESCR, CALVAL } varType_t;
typedef struct {
  const char *name;
  void *pValue;
  varType_t varType;
  int nrValues;
  int minValue;
  int maxValue;
} CGIdesc_t;

extern bool sendBackOK;
extern CGIresponseFileHandler_t readResponseFile;
extern const CGIdesc_t writeVarDescriptors[];
void parseCGIWriteData(char *buf, int received);
#endif /* HTTPD_CGI_H_ */
