#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"

Get_LRE_Info()
{
	PSTR lptr,lptr2,readBuffer = NULL,restPath = NULL;
	int i,projectCount,runCount;

	printf("\n\nGathering test information for %s from EPE...",pttw_eval_string("Run"));

	/* 
			LRE only has two domains:
				DEFAULT				// as of 07/16/2024, the one project in this domain was deleted and so this domain no longer returns data
				DEPT_OF_VET_AFFAIRS // as of 07/16/2024, this is the only domain
			so the parameter save function goes directly to the second one
	*/
	
	pttw_register_save_parameter("lreDOMAINS", "<Name>","</Name","{LREDOMAIN}",NOORDINAL,"body");
	/*	
		There is only one domain in which our projects exist.
		All the REST calls depend upon the domain name and the project name, so both are retrieved
	 */
	
	restPath = pttw_build_restPath(LREbase, Domains, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET], 
		HEADERS, 
			"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	pttw_register_save_parameter("lrePROJECTS", "<Name>","</Name","all","body");
	restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, NULL);
 	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET], 
		HEADERS, 
			"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	holdProject=lptr2=lptr = pttw_eval_string("Project");
	do {										// ensure uppercase
		if ((*lptr2>96)&&(*lptr2<123))
			*lptr2-=32;
		++lptr2;
	} while (*lptr2);

	projectCount = pttw_paramarr_len("lrePROJECTS");
	for (i=1;i<=projectCount;i++) {
		lptr2=pttw_paramarr_idx("lrePROJECTS",i);
		if (!strcmp(lptr,lptr2))
			break;
	}
	pttw_save_string(lptr2,"targetPROJECT");
	
	pttw_register_save_parameter("lreRUNS", "<ID>", "</ID>", "all","body", LAST);

	/* Same deal with gathering the runs - if you know the ID, it can just be inserted or a placeholder created 
	/* to hold it in the following call to get the extended run data that will be transferred to ALM.			*/

	/*
		ALM documentations describes a rich set of parameters for sorting or filtering.
		LRE documentation does not mention any Order-by or query= syntax and I haven't tried it
		This means the the following call assumes the runIDs are returned in descending order (which they are)
	*/
	restPath = pttw_build_restPath(LREbase, Domains, "/{lreDOMAINS}", Projects, "/{targetPROJECT}", Runs, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET], 
		HEADERS, 
			"COOKIE: ", "LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER, 
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	/* check to see if the RUN parameter is requesting a range of LRE RunIDs */
	lptr2=lptr=pttw_eval_string("Run");
	do {												// Run parameter can be a single LRE RunID, a range of RunIDs, or a selection of RunIDs. Range is two numbers separated by a...
		if (*lptr2==0x2d) {								// hyphen
			pttw_save_string(lptr,"RANGE");
			runType=RANGE;
			break;
		}
		else 
			++lptr2;
	} while (*lptr2);
	// check for multiple run retrievals
	if (!runType) {										// if runType is still zero, then a range of LRE RunIDs was not found
		lptr2=lptr;										// and need to check for a...
		do {											//
			if (*lptr2==0x2c) {							// comma
				pttw_save_string(lptr,"SELECTED");
				runType=SELECTED;
				break;
			}
			else 
				++lptr2;
		} while (*lptr2);
	}
	// setup for single transfer
	if (!runType) {										// but if neither were found and runType is still zero, normal run logic will occur as directed by RunLogic()
		if (!(atoi(lptr)))								// if zero was passed in, the latest execution ID will be used
			pttw_save_string(pttw_paramarr_idx("lreRUNS",1),"targetRUN");	// this value is guaranteed to be present because it's in the retrieved RUNS array
		else if (strlen(lptr)) {						// a specific LRE execution ID was requested and so this loop is going to validate/verify its presence in the array
			runCount = pttw_paramarr_len("lreRUNS");	// set up the boundaries of the FOR loop
			for (i=1;i<=runCount;i++) {
				lptr2=pttw_paramarr_idx("lreRUNS",i);
				if (!strcmp(lptr,lptr2))				// break out when the target RunID is found
					break;
			}
			if (i>runCount) {							// uh-oh - someone entered the wrong number
				pttw_error_message("Run %s not found. Verify testID in commandline parameter",lptr);
				return 0;
			}
			pttw_save_string(lptr2,"targetRUN");		// hold the one we need
		} else
			pttw_save_string(pttw_eval_string("lreRUNS_1"),"targetRUN"); // if run parameter was somehow blank, treat it as if zero was passed in
	}
	
	return 0;
}