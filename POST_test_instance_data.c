#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"


int POST_test_instance_data()
{
	PSTR readBuffer = NULL, restPath = NULL;
	//  the following two lines are for testing purposes (locking mechanism primarily)
	//	pttw_save_string("353","RunID");
	//	goto SKIP_CREATE; // bypass the POST

	printf("\n\nCreating Test Instance record in ALM...");

	pttw_save_string(pttw_eval_string("LREMaxVusers"), "LREUserLoad");

	if (DEBUGMODE)
		printf("\n\nPOSTing /testinstances...");

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(LOCATION, "instances/", NULL, NOORDINAL, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestInstances, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[POST],
		"<Entity Type=\"test-instance\"><Fields>"
		"<Field Name=\"test-id\"><Value>{TESTID}</Value></Field>"
		"<Field Name=\"cycle-id\"><Value>{TESTSETID}</Value></Field>"
		"<Field Name=\"test-config-id\"><Value>{TESTCONFIGID}</Value></Field>"
		"<Field Name=\"owner\"><Value>{OWNER}</Value></Field>"
		"<Field Name=\"actual-tester\"><Value>{OWNER}</Value></Field>"
		"<Field Name=\"subtype-id\"><Value>{TESTSUBTYPE}</Value></Field>"
		"<Field Name=\"exec-date\"><Value>{LRESTARTDATE}</Value></Field>"
		"<Field Name=\"exec-time\"><Value>{LRESTARTTIME}</Value></Field>"
		"<Field Name=\"name\"><Value>{lreTestName}</Value></Field>"
		"<Field Name=\"user-01\"><Value>{LREDuration}</Value></Field>"
		"<Field Name=\"user-02\"><Value>{targetRUN}</Value></Field>"
		"<Field Name=\"user-03\"><Value>{TESTTYPE}</Value></Field>"
		"<Field Name=\"user-04\"><Value>{TotalFailedTransactions}</Value></Field>"
		"<Field Name=\"user-05\"><Value>{TotalPassedTransactions}</Value></Field>"
		"<Field Name=\"user-06\"><Value>{LREErrors}</Value></Field>"
		"<Field Name=\"user-07\"><Value>{LREUserLoad}</Value></Field>"
		"<Field Name=\"user-08\"><Value>{LREMaxVusers}</Value></Field>"
		"</Fields></Entity>",
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession};XSRF-TOKEN={XSRF-TOKEN}", CRLF, ENDHEADER,
			"Content-Type:", "application/xml", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	if (strcmp(pttw_eval_string(HTTPSTATUS), POSTSUCCESS)) {
		printf("\n\nFailure creating Test Instance record - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}
	//SKIP_CREATE:
/*
	ALM documentation calls for locking the test-instance before updating it.
	However, this call fails. A ticket has been opened to OT. One and only one posting on the InterWebs suggests this might be a result of database page definitions.
	I brought this up to the Administrator of ALM, but not sure if this will ever get resolved if this is truly the case.
	In any case, the likelihood of a collision when we're updating our own ALM testsets is extremely remote if not altogether impossible.
*/

//	pttw_rest("POST Lock Test-instance",
//       "URL=https://{ALMserver}/qcbin/rest/domains/{DOMAIN}/projects/{PROJECT}/test-instances/{RunID}/lock",
//	   "Method=POST",
//	   	HEADERS,
//	   		"Name=COOKIE","VALUE=LWSSO_COOKIE_KEY={LWSSO_KEY}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=QCSession={QCSession}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=X-XSRF-TOKEN={XSRF-TOKEN}",ENDHEADER,
//			"Name=Content-Type","VALUE=*/*",ENDHEADER,
//			"Name=ACCEPT","VALUE=application/xml",ENDHEADER,
//	LAST);
	
//	pttw_rest("POST Check Lock Test-instance",
//       "URL=https://{ALMserver}/qcbin/rest/domains/{DOMAIN}/projects/{PROJECT}/test-instances/{RunID}/lock",
//	   "Method=GET",
//	   	HEADERS,
//	   		"Name=COOKIE","VALUE=LWSSO_COOKIE_KEY={LWSSO_KEY}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=QCSession={QCSession}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=X-XSRF-TOKEN={XSRF-TOKEN}",ENDHEADER,
//			"Name=ACCEPT","VALUE=application/xml",ENDHEADER,
//	LAST);

	printf("\n\nTest Instance record %s now exists in ALM...",pttw_eval_string("RunID"));

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestInstances, "/{RunID}", NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[PUT],
		"<Entity Type=\"test-instance\">"
		"<Fields>"
		"<Field Name=\"status\"><Value>{LRESTATUS}</Value></Field>"
		"</Fields></Entity>",
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession};XSRF-TOKEN={XSRF-TOKEN}", CRLF, ENDHEADER,
			"Content-Type: ","application/xml", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure creating Test Instance record - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	if (DEBUGMODE)
		printf("\n\nPUTing /testinstance...");

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestInstances, "/{RunID}", NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[PUT],
		"<Entity Type=\"test-instance\">"
		"<Fields>"
		"<Field Name=\"exec-date\"><Value>{LRESTARTDATE}</Value></Field>"
		"<Field Name=\"exec-time\"><Value>{LRESTARTTIME}</Value></Field>"
		"</Fields></Entity>",
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession};XSRF-TOKEN={XSRF-TOKEN}", CRLF, ENDHEADER,
			"Content-Type: ","application/xml", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;
	
	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure creating Test Instance record - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}
	
	//	pttw_rest("DELETE Lock Test-instance",
//       "URL=https://{ALMserver}/qcbin/rest/domains/{DOMAIN}/projects/{PROJECT}/test-instances/{RunID}/lock",
//	   "Method=DELETE",
//	   	HEADERS,
//	   		"Name=COOKIE","VALUE=LWSSO_COOKIE_KEY={LWSSO_KEY}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=QCSession={QCSession}",ENDHEADER,
//	   		"Name=COOKIE","VALUE=X-XSRF-TOKEN={XSRF-TOKEN}",ENDHEADER,
//			"Name=Content-Type","Value=application/xml",ENDHEADER,
//			"Name=ACCEPT","VALUE=application/xml",ENDHEADER,
//	LAST);

	return 0;
}