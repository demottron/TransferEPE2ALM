/*

	Program is capable of handling a single LRE RunID, a range of RunIDs, or a selection of RunIDs. Range is two numbers separated by a hyphen, while multiple RunIDs are to be separated by a comma.
	Both of these modalities are handled in their respective case statement below either through use of a FOR loop for a RANGE or a do-while loop to parse out the RunIDs and execute the program logic.
	Otherwise, a single RunID will run through the program logic one time.

*/
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "enums_ref.h"

/* reverse: reverse string s in place */
static void reverse(char s[]) {
	int c, i, j;
	for (i = 0, j = (int)strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/* itoa: convert n to characters in s */
static void intoa(int n, char s[]) {
	int i, sign;
	if ((sign = n) < 0) /* record sign */
		n = -n; /* make n positive */
	i = 0;
	do { /* generate digits in reverse order */
		s[i++] = n % 10 + '0'; /* get next digit */
	} while ((n /= 10) > 0); /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

int validateRunID(char *lptr) {
	int i,runCount;
	char* lptr2;

	runCount = pttw_paramarr_len("lreRUNS");		// set up the boundaries of the FOR loop
	for (i = 1; i <= runCount; i++) {				// this loop is going to validate/verify the presence of the requested IDs in the list of RUNS for this project
		lptr2 = pttw_paramarr_idx("lreRUNS", i);
		if (!strcmp(lptr, lptr2))					// target RunID is valid
			return 1;
	}
	if (i > runCount) 								// uh-oh - someone entered the wrong number
		printf("\n\nRun %s not found in EPE...skipping", lptr);

	return 0;
}

static void validateRunByID(int iStartID, int iEndID, int *ptrRunPtr) {
	int i,j;
	char szRunID[8];

	memset(szRunID, 0, sizeof(szRunID));
	for (i = iStartID, j = 0; i <= iEndID; i++, j++) {
		intoa(i, szRunID);							// convert the number to a string so that
		if (validateRunID(szRunID))					// it can be checked against the list of lreRUNS returned earlier.
			*(ptrRunPtr + j) = 1;
		else
			*(ptrRunPtr + j) = 0;
	}

}

RunLogic() {
	char *lptr,*lptr2;
	int endOfNumbers=0,i,j,iArraySize,iEndID,iStartID, *ptrRunPtr, skipThisOne;

	Get_LRE_Info();

	switch (runType) {
		case SINGLE:
			Get_LRE_Extended();
			if ((ALM_init()) < 0)
				return -1;
			if ((Get_ALM_Info()) < 0)
				return -1;
			if ((POST_test_instance_data()) < 0)
				return -1;
			if ((PUT_run_data()) < 0)
				return -1;
			break;
		case RANGE:
			lptr2=lptr=pttw_eval_string("RANGE");
			printf("\n\n RL-RANGE: %s", lptr);
			do {												// Run parameter can be a single LRE RunID, a range of RunIDs, or a selection of RunIDs. Range is two numbers separated by a...
				if (*lptr2==0x2d) {								// hyphen
					*lptr2=0;
					iStartID=atoi(lptr);
					break;
				}
				else 
					++lptr2;
			} while (*lptr2);
			lptr=++lptr2;
			iEndID=atoi(lptr);
																// Range of RunIDs may include deleted runs in LRE. Can't allow retrieval of missing data
			iArraySize = (iEndID - iStartID) + 1;				// how many records to transfer / number of integers to create
			ptrRunPtr = malloc(iArraySize);						// create array to validate each RunID
			validateRunByID(iStartID, iEndID, ptrRunPtr);		// array is initialized in function

			for (i=iStartID,j=0;i<=iEndID;i++,j++) {			// iStartID is initialized in the do-while loop at the start of case RANGE:
				if (*(ptrRunPtr+j))								// make sure it exists in LRE - array initialized in validateRunByID()
					pttw_save_int(i, "targetRUN");
				else
					continue;
				printf("\n\n RL-targetRun: %d", i);
				Get_LRE_Extended();
				if (ALM_init()<0)
					return -1;
				if (Get_ALM_Info()<0)
					return -1;
				if ((POST_test_instance_data()) < 0)
					return -1;
				if ((PUT_run_data()) < 0)
					return -1;
			}
			break;
		case SELECTED:
			lptr2=lptr=pttw_eval_string("SELECTED");
			do {												// Run parameter can be a single LRE RunID, a range of RunIDs, or a selection of RunIDs. Selection is numbers separated by a...
				do {
					skipThisOne = 0;
					if ((*lptr2==0x2c)||(*lptr2==0)) {			// comma (also check for the end of the string)
						if (*lptr2==0)
							endOfNumbers=1;						// telegraph the end of the outer DO loop
						*lptr2=0;								// replace the comma with a NULL so this string copy will work
						if (validateRunID(lptr))				// make sure it exists in LRE
							pttw_save_string(lptr, "targetRUN");	// save the string off to a parameter
						else
							skipThisOne = 1;
						++lptr2;								// prepare for the next number when this DO loop resumes
						lptr=lptr2;								// move pointer to next number
						break;									// temporarily halt the string processing and go transfer the record
					}
					else {
						++lptr2;
						if (!(*lptr2)) {
							endOfNumbers=1;						// telegraph the end of the do loop
							pttw_save_string(lptr,"targetRUN");	// save off the last number string to a parameter
						}
					}
				} while (*lptr2);
				if (skipThisOne)
					continue;
				Get_LRE_Extended();
				if (ALM_init()<0)
					return -1;
				if (Get_ALM_Info()<0)
					return -1;
				if ((POST_test_instance_data()) < 0)
					return -1;
				if ((PUT_run_data()) < 0)
					return -1;
			} while (!endOfNumbers);
			break;
	}
	return 0;
}
