/*
		version:		1.1.8
		savetimestamp:	1750878125
		program:		TransferLREtoALM
		purpose:		pulls test run data out of LRE using REST API calls and inserts into ALM's corresponding fields creating a Test Instance recordset
		
		Script is executed from the TransferLREtoALM batch file. Both the script and the batch file are located in the Performance Team's shared folder.
		Script and batch file are both stored in LRE and ALM for the sake of redundancy. 

		Six command line parameters are passed in through the command line prompts from the batch file. These are defined in Runtime Settings Additional Attributes:
			
			Project 	- the name of the LoadRunner Enterprise project to target - must match the parent folder name in ALM.
			Run			- the test execution ID to target. ID can be a single execution, a range of executions (xxxx-yyyy), or multiple executions separated by a comma. If left blank, the most recent run will be used.
			Owner		- your network ID
			TestType	- Baseline, Endurance, Increasing Load, Smoke
			Status		- Completed, Completed With Findings, Blocked, Failed, N/A, NoRun, Not Completed, Passed
			TestID		- Reference the Test Plan page in ALM, this is the ID of the TEST entity created in a project folder under "Performance Test Projects"
			TestSetID	- Reference the Test Lab page in ALM, this is the ID of the TESTSET entity created under a project folder
			
		The Project parameter is required in order to move into the correct project to retrieve the run data
	
		Access into LRE/ALM is via their respective REST API calls. LRE's authentication calls use either keys or encrypted ID/password, however, internal applications (LoadRunner/VUGen) 
		don't have to use keys, so just use the normal ID/PWD encrypted in BASE64 to log in. On the ALM side, though, authentication is through keys created by the Administrator.
		
		For awareness, applications outside of LRE tool suite must use keys to log in. For example, curl is an external application and
		therefore must use keys. Here is a POST version using keys:

			curl -v -i -connect-timeout 30 --json "{\"ClientIdKey\":\"I_KEY_0e451738-3c34-4be7-a38f-6541b9437a9c\",\"ClientSecretKey\":\"S_KEY_74059255-1a94-432d-ab7c-f6873fcebdd9\"}" http://{LREserver}/LoadTest/rest/authentication-point/AuthenticateClient

		General flow of this program is:
		
			Log in to LRE (initialize_Program)
			Retrieve LRE scenario execution data (Get_LRE_Info, Get_LRE_Extended)
			Depending upon the format of the LRE RunIDs, the RunLogic module will either execute the following once or multiple times:
			Log in to ALM (ALM_init)
			Retrieve fields required by a test instance recordset (Get_ALM_Info). The database schema is still owned by the legacy Test Director (TD) entity. Documentation thereof can be found here:
				(https://admhelp.microfocus.com/alm/api_refs/project_db/Content/project_db/topic1.html)
			Create the test instance. ALM internal logic defaults the run status to "Not Complete" and so the recordset is then updated to change the status to "Complete" (POST_test_instance_data)
			ALM automatically creates a Test Run record, but it also needs to have its status updated to "Complete" (PUT_run_data)
			Log out of both systems (end_program)
				
				
		LRE REST API documentation - https://admhelp.microfocus.com/lre/en/all/api_refs/Performance_Center_REST_API/Content/Welcome.htm
		ALM REST API documentation - https://admhelp.microfocus.com/alm/api_refs/REST_core/Content/REST_API_Core/General/Overview.html
 */
#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"

initialize_Program(int argc,char *argv[]) {

	PSTR lptr = NULL, lptr2 = NULL, regBuffer = NULL, readBuffer = NULL, restPath, secretPass;
	int argCount;
	unsigned int buflen, j, k, validTSTATUS = 0, validTTYPE = 0;
	long lCheckParm;
	HINTERNET  almSession=NULL, lreSession=NULL, almConnect=NULL, lreConnect=NULL, almRequest=NULL, lreRequest=NULL;
	
	// move past the program name (0 = full program path and name)
	// process the command line arguments and save them off into parms
	for (argCount = 1; argCount < argc; argCount++) {
		lptr = argv[argCount];
		++lptr;
		for (j = PROJECT; j < ENDPARMS; j++)		// identify which parameter type argv[i] contains
			if (!_stricmp(lptr, parms[j])) {		// case insensitive comparison
				lptr = argv[++argCount];			// found the parameter type, now jump over to the associated value and increment the counter 
				break;
			}
		if (j == ENDPARMS) {						// if for loop got to end of parameter type list, something went wrong
			printf("\nInvalid parameter - check for misspelling");
			return -1;
		}

		// Validate and display the parameters just for reference in the output.txt
		switch (j) {
			case PROJECT:
			case OWNER:
				if (!lptr) {
					printf("\n%s must have a value", parms[j]);						// if there's nothing in the parameter, the pointer will be NULL
					return -1;
				}
				if ((!strlen(lptr)) || (strlen(lptr) < 2) || (*lptr == 0x20)) {		// check for blanks or no word or project less than two characters
					printf("\n%s requires a valid value", parms[j]);
					return -1;
				}
				lptr2 = lptr;
				do {
					if ((*lptr2 < 0x41) || (*lptr2 > 0x7a)) {					// check for non alpha characters
						printf("\nPlease provide a valid name for %s", parms[j]);
						return -1;
					}
					if ((*lptr2 > 0x5A) && (*lptr2 < 0x61)) {					// check for non alpha characters
						printf("\nPlease provide a valid name for %s", parms[j]);
						return -1;
					}
					++lptr2;
				} while (*lptr2);
				break;
			case TESTTYPE:
				k = BASELINE;									// make sure the TestType is one of the available options
				do {
					if (!(_stricmp(lptr, ttype[k]))) {
						validTTYPE = 1;
						break;
					}
					++k;
				} while (k < ENDTYPE);
				if (!validTTYPE) {
					printf("\n%s must be one of %s, %s, %s, %s", parms[j], ttype[BASELINE], ttype[ENDURANCE], ttype[LOAD], ttype[SMOKE]);
					return -1;
				}
				break;
			case PSTATUS:
				k = SCOMPLETED;									// make sure the Status is one of the eight available options found in ALM
				do {
					if (!(_stricmp(lptr, tstatus[k]))) {
						validTSTATUS = 1;
						break;
					}
					++k;
				} while (k < ENDSTATUS);
				if (!validTSTATUS) {
					printf("\n%s must be one of %s, %s, %s, %s, %s, %s, %s, %s", parms[j], tstatus[SCOMPLETED], tstatus[FINDINGS], tstatus[BLOCKED], tstatus[FAILED], \
						tstatus[NA], tstatus[NORUN], tstatus[NOTCOMPLETED], tstatus[PASSED]);
					return -1;
				}
				break;
			case RUN:
			case TESTID:
			case TESTSETID:
				if (!lptr) {
					printf("\n%s must have a value", parms[j]);
					return -1;
				}
				if ((!strcmp(lptr, "Value")) || (*lptr < 0x30) || (*lptr > 0x39)) {
					printf("\nPlease provide a valid number for %s", parms[j]);
					return -1;
				}
				lCheckParm = atol(lptr);
				if ((j != RUN) && (!lCheckParm)) {
					printf("\n%s cannot be zero", parms[j]);
					return -1;
				}
				if (lCheckParm < 0) {
					printf("%s cannot be negative", parms[j]);
					return -1;
				}
				break;
			}
		if (j!=RUN)
			printf("\n %s: \t%s", parms[j], lptr);
		else
			printf("\n %s: \t\t%s", parms[j], lptr);
		if (lptr2 = malloc(buflen = ((int)strlen(parms[j]) + 1)))								// allocate buflen bytes to be pointed at by lptr2
			strcpy_s(lptr2, buflen, parms[j]);													// setting up the save_string parameter name
		else
			ErrorExit(TEXT("Failure parsing command line arguments"));
		pttw_save_string(lptr, lptr2);															// the commandline argument is saved off into the parameter name
		free(lptr2);
	}

	// Read from registry
	printf("\n\nReading server names from registry...");
	ReadRegistryString(HKEY_CURRENT_USER, SUBKEY, ALMSERVER, &ALMserver);						// get the name of the ALM server
	ReadRegistryString(HKEY_CURRENT_USER, SUBKEY, LRESERVER, &LREserver);						// get the name of the (now) EPE server

	printf("\n\nConnecting to EPE...");

	targetKey = LRE_LWSSO_KEY;
	pttw_register_save_parameter(SETCOOKIE, "LWSSO_COOKIE_KEY", ";", LWSSOIDX, "headers", LAST);
	restPath=pttw_build_restPath(LREbase,AUTHpoint,LREauth,NULL);
	buflen = (int)strlen(LREsecret) + 1;
	if (secretPass = malloc(buflen))
		sprintf_s(secretPass, buflen, "%s", LREsecret);
	else
		return -1;
	pttw_rest(HTTPscheme,LREserver,restPath,readBuffer,rtype[GET],
		HEADERS,
			"Authorization: ", secretPass, CRLF, ENDHEADER,
		LAST);
	free(regBuffer);
	free(restPath);
	free(readBuffer);

	pttw_save_string(LREDEPT_OF_VET_AFFAIRS, "LREDOMAIN");									// LRE domain doesn't change - always pulling from main domain
	if (PRODUCTION) {
		pttw_save_string(ALMVA_PROJECTS, "ALMDOMAIN");
		pttw_save_string(ALMPERFORMANCE_TEST_PROJECTS, "ALMPROJECT");
	} else {
		pttw_save_string(ALMVA_PROJECTS, "ALMDOMAIN");					// Test domain/project were dropped
		pttw_save_string(ALMPERFORMANCE_TEST_PROJECTS, "ALMPROJECT");	// when ALM upgraded to 17
	}
	return 0;
}