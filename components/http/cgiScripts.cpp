// #pragma GCC optimize ( "0")

/*
 * cgiScripts.c
 *
 *  Created on: 8 nov. 2010
 *      Author: d.kleppe
 *      changes:
 */

// http://192.168.2.63/cgi-bin/Readvar?allSettings
// http:/192.168.2.7///cgi-bin/getLogMeasValues
#include <stdio.h>
#include <string.h>

//#include "lwip/mem.h"

#include "../../main/include/log.h"
#include "../../main/include/measureTask.h"
#include "../../main/include/settings.h"
#include "../http/include/httpd.h"
#include "cgiScripts.h"

// const tCGI *g_pCGIs;
// int g_iNumCGIs;

// #define NUM_CGIurls 10

char *startCGIscript(int iIndex, char *pcParam);
char *readCGIvalues(int iIndex, char *pcParam);

int readVarScript(char *pBuffer, int count);
int actionRespScript(char *pBuffer, int count);
bool readActionScript(char *pcParam, const CGIdesc_t *CGIdescTable, int size);

int scriptState;
CGIresponseFileHandler_t readResponseFile;
int todoIndex;

CGIdesc_t * getCGIdescriptorsTable();

//extern const CGIurlDesc_t CGIurls[]; 
//extern const CGIdesc_t CGIdescriptors[];

const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n,";

int readDescriptors(char *pBuffer, int count);
// @formatter:off
// do not alter
// static const tCGI CGIurls[NUM_CGIurls] = {
// 	{"/cgi-bin/readvar", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)readVarScript},  // !!!!!! index  !!
// 	{"/cgi-bin/writevar", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)readVarScript}, // !!!!!! index  !!
// 	{"/action_page.php", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)actionRespScript},// !!!!!! index  !!
// 	{"/cgi-bin/getLogMeasValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getDayLogScript},
// 	{"/cgi-bin/getRTMeasValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getRTMeasValuesScript},
// 	{"/cgi-bin/getInfoValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getInfoValuesScript},
// 	{"/cgi-bin/getCalValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)buildCalTable},
// 	{"/cgi-bin/getSensorName", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getSensorNameScript},
// 	{"/cgi-bin/saveSettings", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)saveSettingsScript},
// 	{"/cgi-bin/cancelSettings", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)cancelSettingsScript},
// };


// @formatter:on

// static const CGIdesc_t calibrateDescriptors[] = {
//	{ "Temperature",&calValue, FLT, 1 },
//	{ "RH",&calValue, FLT, 1 },
//	{ "Temperature",&calValue, FLT, 1 },
// };
CGIdesc_t *getSettingsDescriptorTable();
CGIurlDesc_t * getCGIurlsTable(); 

char *readCGIvalues(int iIndex, char *pcParam)
{
	int n;
  CGIurlDesc_t * CGIurls = getCGIurlsTable();
  CGIdesc_t * CGIdescriptors = getSettingsDescriptorTable();
	scriptState = 0;
	readResponseFile = CGIurls[iIndex].responseFileHandler;
	switch (iIndex)
	{
	case 0: // readvar
		for (n = 0; (CGIdescriptors[n].nrValues != 0); n++)
		{
			if (strcmp(pcParam, CGIdescriptors[n].name) == 0)
			{
				todoIndex = n;
				break;
			}
		}
		if (CGIdescriptors[n].nrValues == 0 ) //  sizeof(CGIdescriptors) / sizeof(CGIdesc_t))
			return "";
		break;
	case 2: // writevar
		break;
	case 8: // setCal
			//		readActionScript(pcParam , &calibrateDescriptors[0],
		// sizeof(calibrateDescriptors) / sizeof(CGIdesc_t)); 	return
		//("/spiffs/dmm.html");
		break;

	default:
		todoIndex = iIndex;
	}
	return ("/CGIreturn.txt");
}

bool readActionScript(char *pcParam, const CGIdesc_t *CGIdescTable, int size)
{
	int n, m, len;
	bool success = false;

	char *p = pcParam; // var=1.23&var2=4.56.	<tr>
	char *pDest, *pSrc;
	if (pcParam == NULL)
		return false;

	printf("%s\n", pcParam);
	char name[20];
	do
	{
		success = false;
		len = strlen(p);
		for (n = 0; n < len; n++)
		{
			if (p[n] == '=')
			{
				strncpy(name, p, n);
				name[n] = 0;
				for (m = 0; m < size; m++)
				{
					if (strcmp(name, CGIdescTable->name) == 0)
					{ // found
						if (p[n + 1] != '&')
						{ // empty value
							switch (CGIdescTable->varType)
							{
							case FLT:
								sscanf(&p[n + 1], "%f",
									   (float *)CGIdescTable->pValue); // read value
								break;
							case INT:
								sscanf(&p[n + 1], "%d",
									   (int *)CGIdescTable->pValue); // read value
								break;
							case STR:
								pDest = (char *)CGIdescTable->pValue;
								pSrc = &p[n + 1];
								for (int m = 0; m < MAX_STRLEN - 1; m++)
								{
									if (*pSrc == '+') // spaces are replaced by '+' in HTML form
										*pSrc = ' ';
									*pDest++ = *pSrc++;
									if ((*pSrc == '&') || (*pSrc == 0)) // read until '&' or EOS
										break;
								}
								*pDest = 0;
								break;

							case CALVAL:
								//								if
								//(sscanf(&p[n + 1], "%lf", (double*)
								// actionDescriptors[m].pValue) ==1) // read value
								//									newCalValueReceived
								//= true;
								break;

							case DESCR:
								break;
							}
						}
						success = true;
						break;
					}
					CGIdescTable++;
				}
				break;
			}
		}
		p += n + 1;
		if (success)
			settingsChanged = true;

		success = false; // try to find next value
		for (n = 0; n < strlen(p); n++)
		{
			if (p[n] == '&')
			{
				p += n + 1;
				success = true;
				break;
			}
		}

	} while (success);

	return settingsChanged;
}
// void parseCGIWriteData(char * buf, int received) {
//
// }

/**
 * parses CGI values
 * @param[in] iIndex not used
 * @param[in] pcParam pointer to string containing string with values
 * @param[out] pointer to responsefilename, this file is used to supply variable
 * data to client
 */

// char *readCGIvalues(int iIndex, char *pcParam)
// {
// 	return startCGIscript(iIndex, pcParam);
// }

// nothing		cnt = 0;
int actionRespScript(char *pBuffer, int count)
{
	//	char *pntr = pBuffer;
	int nrChars = 0;
	switch (scriptState)
	{
	case 0:
		// Open for reading hello.tx
		nrChars = sprintf(pBuffer, "%s", http_html_hdr);
		FILE *f = fopen("/spiffs/submRespOk.html", "r"); // fits in 8k
		if (f == NULL)
		{
			printf("submRespOk.html no found");
		}
		else
		{
			// 	printf(" hello found");

			nrChars += fread(pBuffer + nrChars, 1, 8000, f);
			fclose(f);
		}

		//	nrChars = sprintf(pntr, "%s", http_html_hdr);
		//	pntr += nrChars;
		//	nrChars += sprintf(pntr, "Ok\n");
		scriptState++;
		break;
	}
	return nrChars;
}

int readDescriptorsScript(char *pBuffer, int count)
{
	switch (scriptState)
	{
	case 0:
		scriptState++;
		return readDescriptors(pBuffer, count);

		break;
	default:
		break;
	}
	return 0;
}

int readVarScript(char *pBuffer, int count)
{
	char *pntr = pBuffer;
	int nrChars = 0;
	int cnt = 0;
	//	void *pValue;
	uint8_t *pValue;
	int nrValues = 1;
	settingsDescr_t *pDescr;
	int n;
  CGIdesc_t * settingsDescrTable = getSettingsDescriptorTable();

	pValue = (uint8_t *)settingsDescrTable[todoIndex].pValue;
	nrValues = settingsDescrTable[todoIndex].nrValues;

	switch (scriptState)
	{
	case 0:
		nrChars = sprintf(pntr, "%s", http_html_hdr);
		pntr += nrChars;
		if (settingsDescrTable[todoIndex].varType == DESCR)
		{
			pDescr = (settingsDescr_t *)settingsDescrTable[todoIndex].pValue;
			while (pDescr->size > 0)
			{
				pValue = (uint8_t *)pDescr->pValue;
				for (n = 0; n < pDescr->size; n++)
				{
					switch (pDescr->varType)
					{
					case FLT:
						cnt = sprintf(pntr, "%2.1f,", *(float *)pValue);
						pValue += sizeof(float);
						break;
					case INT:
						cnt = sprintf(pntr, "%d,", *(int *)pValue);
						pValue += sizeof(int);
						break;
					case STR:
						cnt = sprintf(pntr, "%s,", (char *)pValue);
						pValue += MAX_STRLEN + 1;
						break;
					case DESCR:
					case CALVAL:
						break;
					}
					pntr += cnt;
					nrChars += cnt;
				}
				pDescr++;
			}
		}
		else
		{
			for (int n = 0; n < nrValues; n++)
			{
				switch (settingsDescrTable[todoIndex].varType)
				{
				case FLT:
					cnt = sprintf(pntr, "%2.1f", *(float *)pValue);
					pValue += sizeof(float);
					break;
				case INT:
					cnt = sprintf(pntr, "%d", *(int *)pValue);
					pValue += sizeof(int);
					break;
				case STR:
					cnt = sprintf(pntr, "%s", (char *)pValue);
					pValue += MAX_STRLEN + 1;
					break;
				case DESCR:
				case CALVAL:
					break;
				}
				pntr += cnt;
				if (n < (nrValues - 1))
				{
					*pntr++ = ',';
					cnt++;
				}
				nrChars += cnt;
			}
		}
		scriptState++;
		break;
	}
	return nrChars;
}

