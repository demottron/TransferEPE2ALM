#include <strsafe.h>
#include "globals.h"
#include "enums.h"

Get_LRE_Extended()
{
	PSTR lptr, lptr2, lptr4, lptr5 = NULL, readBuffer = NULL, restPath = NULL;
	char szErrorMessage[128];
	size_t buflen = 0, offset = 0, sepLen = 0;
	int i = 0, lptr5Len = 0, nameCount = 0, vuserCount = 0, x = 0;

	pttw_register_save_parameter("lreTestID", "<TestID>","</TestID>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("TestInstanceID", "<TestInstanceID>","</TestInstanceID>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREDuration", "<Duration>","</Duration>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LRERunState", "<RunState>","</RunState>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREStartTime", "<StartTime>","</StartTime>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREEndTime", "<EndTime>","</EndTime>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREMaxVusers", "<MaxVusers>","</MaxVusers>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREErrors", "<TotalErrors>","</TotalErrors>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("TotalPassedTransactions", "<TotalPassedTransactions>","</TotalPassedTransactions>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("TotalFailedTransactions", "<TotalFailedTransactions>","</TotalFailedTransactions>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("AverageHitsPerSecond", "<AverageHitsPerSecond>","</AverageHitsPerSecond>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("AverageThroughputPerSecond", "<AverageThroughputPerSecond>","</AverageThroughputPerSecond>", NOORDINAL, "body",LAST);
	pttw_register_save_parameter("LREErrorCode", "<ErrorCode>", "</ErrorCode>", NOORDINAL, "body", LAST);

	restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, "/{targetPROJECT}", Runs, "/{targetRUN}", Extended, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET], 
		HEADERS, 
			"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	lptr = pttw_eval_string("LREErrorCode");
	if (lptr) {
		sprintf_s(szErrorMessage, sizeof(szErrorMessage), "Completely unexpected failure retrieving data from EPE. Return code was %s", lptr);
		ErrorExit((LPTSTR)szErrorMessage);
	}

	pttw_register_save_parameter("lreTestName", "<Name>", "</Name>", NOORDINAL, "body", LAST);

	restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, "/{targetPROJECT}", Tests, lreTestsQuery, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET],
		HEADERS,
		"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	pttw_register_save_parameter("grpVusers", "<Vusers>", "</Vusers>", "all", "body", LAST);
	pttw_register_save_parameter("grpId", "<ID>", "</ID>", "all", "body", LAST);

	restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, "/{targetPROJECT}", Tests, "/{lreTestID}", Groups, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET], 
		HEADERS, 
			"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	sepLen = strlen(SEPARATOR);
	vuserCount = pttw_paramarr_len("grpVusers");		// find an active group for the test run (has non-zero value)
	if (vuserCount) {									// if somehow the call to get the test group's info doesn't work - not that big a deal, just skip it
		for (i = 1; i <= vuserCount; i++) {					// usually there's only one, but if there were multiple, all will need to be retrieved
			lptr = pttw_paramarr_idx("grpVusers", i);
			if (atoi(lptr)) {							// look for a non-zero value
				lptr2 = pttw_paramarr_idx("grpId", i);		// get the associated script ID
				if (!lptr2)
					continue;
				pttw_register_save_parameter(SCRIPT, "<Name>", "</Name>", NOORDINAL, "body", LAST);
				restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, "/{targetPROJECT}", Scripts, "/", lptr2, NULL); // go get the script name
				pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET],
					HEADERS,
					"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER,
					LAST);
				free(restPath);
				free(readBuffer);
				restPath = readBuffer = NULL;
				lptr4 = pttw_eval_string(SCRIPT);
				if ((++x) > 1) {								// this signifies there was more than one script involved in scenario
					lptr5 = realloc(lptr5, (buflen = (strlen(lptr4) + sepLen + 1)) + offset); // plus three for " / " token separator + 1 for nullbyte
					memset(lptr5 + offset, 0, buflen);
					strcat_s(lptr5, buflen+offset, SEPARATOR);
					lptr5Len = (int)sepLen;
				}
				else {
					lptr5 = realloc(lptr5, buflen = (strlen(lptr4) + 1)); // first time through
					memset(lptr5, 0, buflen);
					lptr5Len = 0;
				}
				strcat_s(lptr5, buflen + offset, lptr4);
				offset += (strlen(lptr4) + lptr5Len);
			}
		}
		pttw_save_string(lptr5, "SCRIPTNAME");	// script name saved into Run record
		free(lptr5);
		for (i = 1; i <= vuserCount; i++) {
			pttw_drop_parm("grpVusers");
			pttw_drop_parm("grpId");
		}
	}
	if (DEBUGMODE)
		pttw_display_parms('s');
	lptr=pttw_eval_string("LREStartTime");		// separate date from time
	if (lptr) {
		lptr2=lptr;
		while (*lptr!=0x20)
			++lptr;
		*lptr=0;
		lptr++;
		pttw_save_string(lptr2,"LRESTARTDATE");
		pttw_save_string(lptr,"LRESTARTTIME");
	}
	
	lptr=pttw_eval_string("LRERunState");
	if (!strcmp(lptr,"Finished"))									// if the test is not running and has been analyzed
		pttw_save_string(pttw_eval_string("Status"),"LRESTATUS");	// save the passed-in status as the final status
	else if (!strcmp(lptr, "Before Creating Analysis Data"))		// 1.1.4 added - if the test is not running but has not been analyzed
		pttw_save_string(pttw_eval_string("Status"), "LRESTATUS");	// 1.1.4 added - save the passed-in status as the final status
	else
		pttw_save_string("Not Completed","LRESTATUS");				// otherwise just say it's not done yet. 
	
	return 0;
}