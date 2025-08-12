#pragma once
int bShowHeaders = 0;
int misCount = 0;
int runType = 0;									// global variable for a flag which identifies
int saveCookie = 0;									// hokey flag to capture the initial LWSSO_COOKIE_KEY
PSTR holdProject;									// global pointer to save the name of the target project for validating the TESTID and TESTSETID are at least in the correct ALM project
LPCCH XML = "application/xml";
PSTR targetKey;										// needs to be set to either LRE_LWSSO_KEY or ALM_LWSSO_KEY. After they've been captured, set to NULL

enum eType { SINGLE, RANGE, SELECTED } eType;						// how the LRE RunID(s) were entered

/* character arrays for handling commandline parameters */
PSTR parms[] = { "Project","Owner","TestType","Run","TestID","TestSetID","Status" };
enum eParms { PROJECT, OWNER, TESTTYPE, RUN, TESTID, TESTSETID, PSTATUS, ENDPARMS } eParms;

PSTR ttype[] = { "baseline", "endurance", "increasing load", "smoke" };
enum eTtype { BASELINE, ENDURANCE, LOAD, SMOKE, ENDTYPE } eTtype;

PSTR tstatus[] = {"Completed", "Completed With Findings", "Blocked", "Failed", "N/A", "No Run", "Not Completed", "Passed"};
enum eTstatus {SCOMPLETED, FINDINGS, BLOCKED, FAILED, NA, NORUN, NOTCOMPLETED, PASSED, ENDSTATUS } eTstatus;

PSTR rtype[] = { "GET", "POST", "PUT", "DELETE" };
enum eRtype { GET, POST, PUT, rDELETE }; 