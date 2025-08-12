#include "globals.h"
#include "enums_ref.h"
#include <stdio.h>


int Get_ALM_Info()
{	PSTR lptr, lptr2, readBuffer = NULL, restPath = NULL;
	int i,statusCount,foundProject=0,parentID;
	
	if (DEBUGMODE)
		printf("\n\nCalling /domains...");

	if (DEBUGMODE)
		bShowHeaders = 1;
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("almDOMAIN", "<Domain Name=\"", "\"", NOORDINAL, "body", LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, NULL);
 	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
	   		"COOKIE: ","LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	if (DEBUGMODE)
		bShowHeaders = 0;

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting domains - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	if (DEBUGMODE)
		printf("\n\nCalling /projects...");

	lptr=pttw_eval_string("almDOMAIN");
	if (PRODUCTION)
		lptr2=ALMDOMAINNAME_PROD;
	else
		lptr2=ALMDOMAINNAME_TEST;
	if (strcmp(lptr,lptr2)) {
		printf("ALM domain not as expected (%s) - retrieved %s...aborting.",lptr2,lptr);
		return -1;
	}

	/*
		In development, there were only two projects in the DEFAULT domain.
		This will likely need to be adjusted when the program gets moved over to production and the Ordinal of the Performance_Test_Projects changes
	*/
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("almPROJECT", "Project Name=\"","\"","{ALMPROJECT}","body",LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting projects - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	if (DEBUGMODE)
		printf("\n\nCalling /tests?testsquery...");
	/*
	Validate TESTID and that they belong to the project entered as a parameter
*/
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("Test_id", "<Field Name=\"id\"><Value>","</Value>",NOORDINAL,"body",LAST);
	pttw_register_save_parameter("Test_parent-id", "<Field Name=\"parent-id\"><Value>","</Value>",NOORDINAL,"body",LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", Tests, TestsQuery, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting tests - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	lptr=pttw_eval_string("Test_parent-id");				// confirm a record exists for TESTID by retrieving it
	if (lptr)
		parentID=atoi(lptr);
	else {
		printf("\n\n%s does not exist!",pttw_eval_string("TESTID"));
		return -1;
	}
	if (parentID) 
		while (parentID>2) {								// ROOT folder ID is 2
			pttw_save_int(parentID,"PID");

			if (DEBUGMODE)
				printf("\n\nCalling /testfolders?testfoldersquery...");

			pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
			pttw_register_save_parameter("TestFolders_id", "<Field Name=\"id\"><Value>","</Value>",NOORDINAL,"body",LAST);
			pttw_register_save_parameter("TestFolders_parent-id", "<Field Name=\"parent-id\"><Value>","</Value>",NOORDINAL,"body",LAST);
			pttw_register_save_parameter("TestFolders_name", "<Field Name=\"name\"><Value>","</Value>",NOORDINAL,"body",LAST);
			pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
			restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestFolders, TestFoldersQuery, NULL);
			pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
				HEADERS,
					"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
				LAST);
			free(restPath);
			free(readBuffer);

			if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
				printf("\n\nFailure GETting test folders - return code was %s", pttw_eval_string(HTTPSTATUS));
				return -1;
			}

			lptr=pttw_eval_string("TestFolders_parent-id");				// examine record's IDs and evaluate name
			if (lptr)
				parentID=atoi(lptr);
			if (!strcmp(pttw_eval_string("TestFolders_name"),holdProject)) {
			    foundProject=1;
			    break;
			}
		}
	else {
		printf("TestID %s did not retrieve a valid parent-id",pttw_eval_string("TESTID"));
		return -1;
	}

	if (!foundProject) {
		printf("TestID %s does not belong to project %s",pttw_eval_string("{TESTID}"),holdProject);
		return -1;
	}
	foundProject = 0;					// clear the flag in preparation for the TestSetID check - added 06/25/2025 TPN

	if (DEBUGMODE)
		printf("\n\nCalling /testsets?testsetsquery...");
/*
	Validate TESTSETID and that they belong to the project entered as a parameter
*/
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("Test_id", "<Field Name=\"id\"><Value>","</Value>",NOORDINAL,"body",LAST);
	pttw_register_save_parameter("Test_parent-id", "<Field Name=\"parent-id\"><Value>","</Value>",NOORDINAL,"body",LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestSets, TestSetsQuery, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting test sets - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	lptr=pttw_eval_string("Test_parent-id");				// confirm a record exists for TESTID by retrieving it
	if (lptr)
		parentID=atoi(lptr);
	else {
		printf("%s does not exist!",pttw_eval_string("{TESTID}"));
		return -1;
	}
	if (parentID) 
		while (parentID>0) {								// ROOT folder ID is 0
			pttw_save_int(parentID,"PID");

			if (DEBUGMODE)
				printf("\n\nCalling /testsetfolders?testsetfoldersquery...");

			pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
			pttw_register_save_parameter("Folders_id", "<Field Name=\"id\"><Value>","</Value>",NOORDINAL,"body",LAST);
			pttw_register_save_parameter("Folders_parent-id", "<Field Name=\"parent-id\"><Value>","</Value>", NOORDINAL, "body",LAST);
			pttw_register_save_parameter("Folders_name", "<Field Name=\"name\"><Value>","</Value>", NOORDINAL, "body",LAST);
			pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
			restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestSetFolders, TestSetFoldersQuery, NULL);
			pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
				HEADERS,
					"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
				LAST);
			free(restPath);
			free(readBuffer);

			if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
				printf("\n\nFailure GETting test set folders - return code was %s", pttw_eval_string(HTTPSTATUS));
				return -1;
			}

			lptr=pttw_eval_string("Folders_parent-id");				// examine record's IDs and evaluate name
			if (lptr)
				parentID=atoi(lptr);
			if (!strcmp(pttw_eval_string("Folders_name"),holdProject)) {
			    foundProject=1;
			    break;
			}
		}
	else {
		printf("TestSetID %s did not retrieve a valid parent-id",pttw_eval_string("{TESTSETID}"));
		return -1;
	}

	if (!foundProject) {
		printf("TestSetID %s does not belong to project %s",pttw_eval_string("{TESTSETID}"),holdProject);
		return -1;
	}
/*

	Retrieve the needed ConfigID

*/
	if (DEBUGMODE)
		printf("\n\nCalling /testconfigs?tetsconfigsquery...");

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter("TESTCONFIGID", "\"id\"><Value>","<", NOORDINAL, "body",LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", TestConfigs, TestConfigsQuery, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	
	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting test configs - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	/*

	Set default Status
	
*/
	if (DEBUGMODE)
		printf("\n\nCalling /usedlists?usedlistsquery...");

	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	pttw_register_save_parameter(AISTATE, "value=\"","\"","ALL","body",LAST);
	pttw_register_save_parameter(STATUS, " ", "\r\n", NOORDINAL, "headers", LAST);
	restPath = pttw_build_restPath(ALMbase, ALMrest, Domains, "/{almDOMAIN}", Projects, "/{almPROJECT}", Customization, UsedLists, UsedListsQuery, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[GET],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);

	if (strcmp(pttw_eval_string(HTTPSTATUS), HTTPOKAY)) {
		printf("\n\nFailure GETting used lists - return code was %s", pttw_eval_string(HTTPSTATUS));
		return -1;
	}

	lptr=COMPLETED;
	if (strlen(lptr)) {
		statusCount = pttw_paramarr_len(AISTATE);
		for (i=1;i<=statusCount;i++) {
			lptr2=pttw_paramarr_idx(AISTATE,i);
			if (!strcmp(lptr,lptr2))
				break;
		}
		if (i>statusCount) {
			printf("Status %s not found. Something ridiculous just happened",lptr);
			return 0;
		}
		pttw_save_string(lptr2,"RUNSTATUS");
	} else
		pttw_save_string(pttw_eval_string("Status_1"),"RUNSTATUS");

	return 0;
}