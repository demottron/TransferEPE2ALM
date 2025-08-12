/*

	Log in to ALM and parse the command line parameters relevant to the ALM repository as well as set up some of the required ALM logical name values.
	String values are forced to lowercase as per the default values created for ALM's dropdownlistboxes.
	The TestID is the integer value of the target TEST found on the Test Plan page in ALM.
	The TestSetID is the integer value of the target TESTSET found on the Test Lab page in ALM.

*/
#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"

int ALM_init() {
	PSTR lptr, lptr2, readBuffer = NULL, restPath = NULL;
	int firstChar=1;	

	if (DEBUGMODE)
		printf("\n\nConnecting to TEST ALM...");
	else 
		printf("\n\nConnecting to PRODUCTION ALM...");

	if (DEBUGMODE)
		printf("\n\nCalling /login...");

	if (DEBUGMODE)
		bShowHeaders = 1;
	targetKey = ALM_LWSSO_KEY;
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(SETCOOKIE, "QCSession", ";", QCSession, "headers", LAST);
	pttw_register_save_parameter(SETCOOKIE, "XSRF-TOKEN", ";", XSRFtoken, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, ALMoauth, ALMlogin, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[POST],
		ALMauthBody,
		HEADERS,
			"Content-Type: ", "application/json", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	targetKey = restPath = readBuffer = NULL;
	if (DEBUGMODE)
		bShowHeaders = 0;

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure POSTing to /alm-authenticate - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	if (DEBUGMODE)
		printf("\n\nCalling /is-authenticated...");

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMversion, ALMrest, ALMisAuthenticated, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE:", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession};", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure confirming authentication - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	lptr=pttw_eval_string("TestID");													// retrieve the TestID which was passed in on the command line
	if ((!lptr)||(*lptr==0x30)) {
		printf("%s","TestID not found in Runtime Settings or command line parameter. Verify TestID in commandline parameter");
		return -1;
	} else
		pttw_save_string(lptr,"TESTID");
	
	lptr=pttw_eval_string("TestSetID");
	if ((!lptr)||(*lptr==0x30)) {
		printf("%s","TestSetID not found in Runtime Settings or command line parameter. Verify TestSetID in commandline parameter");
		return -1;
	} else
		pttw_save_string(lptr,"TESTSETID");

	lptr2=lptr=pttw_eval_string("Owner");
	do {										// convert to lower case
		if ((*lptr2>64)&&(*lptr2<91))
			*lptr2+=32;
		++lptr2;
	} while (*lptr2);
	if (!lptr)
		printf("%s","Owner not found in Runtime Settings or command line parameter. Verify Owner in commandline parameter");
	else
		pttw_save_string(lptr,"OWNER");

	lptr2=lptr=pttw_eval_string("TestType");
	do {										// convert to lower case
		if (!firstChar) { 						// unless first character of a word
			if ((*lptr2>64)&&(*lptr2<91))
				*lptr2+=32;
		} else
			firstChar=0;
		++lptr2;
		if (*lptr2==32) {
			firstChar=1;
			++lptr2;
		}
	} while (*lptr2);
	if (!lptr)
		printf("%s","TestType not found in Runtime Settings or command line parameter. Verify TestType in commandline parameter");
	else
		pttw_save_string(lptr,"TESTTYPE");
	
	pttw_save_string("hp.qc.test-instance.LR-SCENARIO","TESTSUBTYPE");
	pttw_save_string("hp.qc.run.LR-SCENARIO","RUNSUBTYPE");

	return 0;
}