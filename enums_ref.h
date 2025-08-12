#pragma once
extern int bShowHeaders;
extern int misCount;
//extern int paramDefined;
extern int runType;											// global variable for a flag which identifies 
extern int saveCookie;										// hokey flag to capture the initial LWSSO_COOKIE_KEY
extern char* holdProject;									// global pointer to save the name of the target project for validating the TESTID and TESTSETID are at least in the correct ALM project
extern char* XML;
extern char* targetKey;

extern enum eType { SINGLE, RANGE, SELECTED } eType;							// how the LRE RunID(s) were entered

/* character arrays for handling commandline parameters */
extern char* parms[];
extern enum eParms { PROJECT, OWNER, TESTTYPE, RUN, TESTID, TESTSETID, PSTATUS, ENDPARMS } eParms;

extern char* ttype[];
extern enum eTtype { BASELINE, ENDURANCE, LOAD, SMOKE, ENDTYPE } eTtype;

extern char* tstatus[];
extern enum eTstatus { SCOMPLETED, FINDINGS, BLOCKED, FAILED, NA, NORUN, NOTCOMPLETED, PASSED, ENDSTATUS } eTstatus;

extern char* rtype[];
enum eRtype { GET, POST, PUT, rDELETE } eRtype; 