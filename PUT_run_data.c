#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"

int PUT_run_data()
{
	PSTR readBuffer = NULL, restPath = NULL;

	printf("\n\nUpdating Test Instance record %s with status %s in ALM...", pttw_eval_string("RunID"),pttw_eval_string("LRESTATUS"));

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("RunID", "\"id\"><Value>", "</Value>", NOORDINAL, "body", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", ALMruns, ALMrunsQuery, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE:","LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};", "QCSession={QCSession}", CRLF, ENDHEADER,
	  LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting ALM runs record - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}
	
	/*

	Leaving all the fields in here in case something comes up in the future. 
	Be advised that this was copied from the test-instance entity and there may be fields which don't belong to a run entity
	
*/ 
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", ALMruns, "/{RunID}", NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[PUT],
		"<Entity Type=\"run\"><Fields>"
		"<Field Name=\"name\"><Value>{SCRIPTNAME}</Value></Field>"
		"<Field Name=\"status\"><Value>{LRESTATUS}</Value></Field>"
		"<Field Name=\"owner\"><Value>{OWNER}</Value></Field>"
		"<Field Name=\"test-execution-id\"><Value>{targetRUN}</Value></Field>"
		"<Field Name=\"execution-date\"><Value>{LRESTARTDATE}</Value></Field>"
		"<Field Name=\"execution-time\"><Value>{LRESTARTTIME}</Value></Field>"
		"<Field Name=\"duration\"><Value>{LREDuration}</Value></Field>"
		"</Fields></Entity>",
		//			"<Field Name=\"test-id\"><Value>{TESTID}</Value></Field>" 
		//			"<Field Name=\"testcycl-id\"><Value>{RunID}</Value></Field>"
		//	   		"<Field Name=\"cycle-id\"><Value>{TESTSETID}</Value></Field>"
		//	   		"<Field Name=\"test-config-id\"><Value>{TESTCONFIGID}</Value></Field>"
		//	   		"<Field Name=\"subtype-id\"><Value>{RUNSUBTYPE}</Value></Field>"
		//	   		"<Field Name=\"actual-tester\"><Value=VHAISPNICHOT></Field>"
		//	   		"<Field Name=\"subtype-id\"><Value=\"{subtype-id}\"></Field>"
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession};XSRF-TOKEN={XSRF-TOKEN}", CRLF, ENDHEADER,
			"Content-Type:","application/xml", CRLF, ENDHEADER,
	  LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure PUTting ALM runs record - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	return 0;
}