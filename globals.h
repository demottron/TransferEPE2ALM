#ifndef _GLOBALS_H
#define _GLOBALS_H

#ifdef _DEBUG
#define DEBUGMODE 1
#else
#define DEBUGMODE 0
#endif
//--------------------------------------------------------------------
// Include Files
#include <windows.h>
#include <winhttp.h>
//--------------------------------------------------------------------
// Global Variables
#define ProgramName			"TransferLREtoALM"
#define ProgramVersion		"1.1.9"
//--------------------------------------------------------------------
// Global Variables
#define ALMSERVER			"ALMserver"									// "oitispopstsalm1.va.gov" "vhaispopstsalm4.vha.med.va.gov"   Moved to registry 06/09/2025
//#define ALMauthBody			"<alm-authentication><user>apikey-ondejhdmlfpfkskbipdb</user><password>bmodbncchhpidofb</password></alm-authentication>"
#define ALMauthBody			"{\"clientId\":\"apikey-ondejhdmlfpfkskbipdb\", \"secret\":\"bmodbncchhpidofb\"}"
#define ALMsessionBody		"<session-parameters><client-type>TLA PGM</client-type><time-out>1<time-out></session-parameters>"
#define ALMauthInfo			"<AuthenticationInfo><Username>lre</Username></AuthenticationInfo>"
#define ALM_LWSSO_KEY		"ALM_LWSSO_KEY"
#define CRLF				"\r\n"
#define HTTPscheme			"HTTP"
#define HTTPSscheme			"HTTPS"
#define HTTPSTATUS          "httpStatus"
#define LOCATION			"location"
#define LREsecret			"Basic dmhhaXNwbmljaG90OjAwRGVzZXJ0U3Rvcm0xMQ=="
#define LRESERVER			"EPEserver"									// "vhaispopspcsvr5.vha.med.va.gov"	// OITISPOPSPCSVR1.VA.GOV   moved to registry as of 06/09/2025
#define LWSSO_COOKIE_KEY	"LWSSO_COOKIE_KEY"
#define LRE_LWSSO_KEY		"LRE_LWSSO_KEY"
#define RUNID				"RunID"
#define SCRIPT              "Script"
#define SETCOOKIE			"set-cookie"
#define STATUS				"Status"
#define SUBKEY				"SOFTWARE\\TransferLREtoALM"				// location in registry

#define AISTATE 			"almState"                                  // ALM's internal status options
#define ALMauth				"/alm-authenticate"
#define ALMbase				"/qcbin"
#define ALMisAuthenticated	"/is-authenticated"
#define ALMlogin			"/login"
#define ALMoauth			"/oauth2"
#define ALMrest				"/rest"
#define ALMruns				"/runs"
#define ALMrunsQuery		"?order-by={id[DESC]}&page-size=1"
#define ALMsession			"/site-session"
#define ALMversion			"/v2"
#define AUTHpoint			"/authentication-point"
#define Customization		"/customization"
#define Domains				"/domains"
#define Extended			"/Extended"
#define Fields				"?fields=*"
#define Groups              "/groups"
#define Logout				"/logout"
#define LREauth				"/authenticate"
#define LREbase				"/LoadTest/rest"
#define Projects			"/projects"
#define Runs				"/Runs"
#define Scripts             "/Scripts"
#define SEPARATOR           " / "
#define Tests               "/tests"
#define lreTestsQuery		"?query={id[{lreTestID}]}"
#define TestsQuery			"?query={id[{TESTID}]}&fields=id,parent-id"
#define TestConfigs			"/test-configs"
#define TestConfigsQuery	"?query={parent-id[{TESTID}]}&fields=id"
#define TestFolders			"/test-folders"
#define TestFoldersQuery	"?query={id[{PID}]}&fields=id,parent-id,name"
#define TestInstances		"/test-instances"
#define TestSets			"/test-sets"
#define TestSetsQuery		"?query={id[{TESTSETID}]}&fields=id,parent-id"
#define TestSetFolders		"/test-set-folders"
#define TestSetFoldersQuery "?query={id[{PID}]}&fields=id,parent-id,name"
#define UsedLists			"/used-lists"
#define UsedListsQuery		"?name=Status"

#define LEFTPARMDELIMINATOR		'{'
#define RIGHTPARMDELIMINATOR	'}'
#define QUADWORD				64					// used in pttw_save_int for buffersize
#define BUFFSIZE				256					// buffer size which gets allocated in pttw_util_functions
#define HTTPOKAY				"200"				// generic confirmation of success from most recent call
// despite docs saying POST returns 201, it's a 200
#define POSTSUCCESS             "201"               // a successful POST will result in a 201 indicating a Test Instance has been created
#define LWSSOIDX				"1"					// LWSSO_KEY index for set-cookie
#define QCSession				"2"					// index of the set-cookie which corresponds to QCSession
#define XSRFtoken				"4"					// index of the set-cookie which corresponds to the XSRF tkn
#define NOINDEX					0					// pttw_find_parm responds to zero by returning the first encountered parm's value
#define COUNTMEMBERS			-1					// pttw_find_parm responds to negative one by counting the parms with the same name
#define GETADDRESS				-2					// pttw_find_parm responds to negative two by returning the first encountered parm's address
#define DROPPARM                -3                  // pttw_fine_parm responds to negative three by returning the address of th
/* These four values are used by WinHttpSetTimeouts */
#define RESOLVETIMEOUT			0					// The initial value is zero, meaning no time-out (infinite).
#define CONNECTTIMEOUT			60000				// The initial value is 60,000 (60 seconds).
#define SENDTIMEOUT				30000				// The initial value is 30,000 (30 seconds).
#define RECEIVETIMEOUT			30000				// The initial value is 30,000 (30 seconds).
/* These four used by pttw_increase_timeouts */
#define RESOLVEREQUEST			0					// identify which of the four values to increase
#define CONNECTREQUEST			1
#define SENDREQUEST				2
#define RECEIVEREQUEST			3
/* defining how many times to retry (avoiding magic number in code) */
#define MAX_RETRIES				5

#ifdef _DEBUG
#define PRODUCTION 0								// 0 for Development/Test, 1 for Production - will copy the appropriate #define values below for domain and project
#else
#define PRODUCTION 1
#endif                                              // moved into Production 2024/01/24 circa 1230 Eastern
/* use the following two #defines for safety check */
#define ALMDOMAINNAME_TEST "VA_PROJECTS"			// test domain name went away with upgrade to 17 (8/27)
#define ALMDOMAINNAME_PROD "VA_PROJECTS"			// known Production domain name

#define COMPLETED "Completed"						// overly complex use of a variable to set the test status in ALM

/* The following defines are used in the ordinal filters for selecting the target domain and project in their respective systems */
#define LREDEFAULT							"1"		// LRE Domain Test (07/16/2024: project in this domain was deleted and so effectively no longer exists)
#define LREDEPT_OF_VET_AFFAIRS				"1"		// LRE Domain Production (07/16/2024: only project to be returned by /domains)
#define ALMDEFAULT							"1"		// ALM Domain Test - this value is based upon what my ID can see - 
#define ALMVA_PROJECTS						"1"		// ALM Domain Production - this value is what the API key can see. Personal accounts may be able to see more.
#define ALMPERFORMANCE_TEST_PROJECTS_COPY	"1"		// ALM Project Test - no longer exists (dropped when moving to 17.
#define ALMPERFORMANCE_TEST_PROJECTS		"1"		// ALM Project Production
#define NOORDINAL							"0"		// pttw_store_parms needs to know there's only one parm incoming
#define cGETADDRESS							"-2"	// pttw_store_parms needs only to check whether parm already exists
/* end of domain/project #defines */

int ALM_init();
int end_Program();
int Get_ALM_Info();
int Get_LRE_Info();
int Get_LRE_Extended();
int initialize_Program(int, char* []);
int POST_test_instance_data();
int PUT_run_data();
int RunLogic();

/*  Performance Test Team Web Rest */
#define HEADERS							"HEADERS"
#define ENDHEADER						"ENDHEADER"
#define LAST							0
#define SEARCH_FILTERS					"FILTERS"

typedef int PTTW_INT;
typedef PSTR PTTW_NAME;
typedef PSTR PTTW_PARM;
typedef PSTR PTTW_STRING;
typedef PSTR PTTW_URL;
typedef PSTR PTTW_TYPE;
typedef PSTR* PTTW_PARM_LIST;

PTTW_STRING* ALMserver;									// added 06/10/2025 in support of using registry instead of hardcoded in this header file
PTTW_STRING* LREserver;									// added 06/10/2025 in support of using registry instead of hardcoded in this header file

void ErrorExit(LPTSTR);
PSTR pttw_build_restPath(PSTR, PSTR, ...);
int pttw_check_for_parms(PSTR*, PDWORD);
LPCWSTR pttw_convert_to_wide(LPCCH,int *);
void pttw_display_parms(char);
int pttw_drop_last_parameters(struct parameters* );
int pttw_drop_parm(PSTR);
void pttw_dump_parm_pointers(void);
int pttw_error_message(PTTW_STRING, PTTW_STRING, ...);
PSTR pttw_eval_string(PSTR);
int pttw_extract_cookies(PSTR*, struct parameters*);
PSTR pttw_find_parm(PSTR,DWORD*);
int pttw_get_wide_count(LPCCH);
struct parameters* pttw_get_parameters(void);
int pttw_inspect_header_data(HINTERNET, PTTW_STRING, struct parameters*);
char* pttw_paramarr_idx(PTTW_PARM, PTTW_INT);
int pttw_paramarr_len(PTTW_STRING);
int pttw_rest(PTTW_STRING, PTTW_STRING*, PTTW_URL, PSTR, PTTW_TYPE, ...);
int pttw_save_int(PTTW_INT, PTTW_STRING);
//int pttw_save_int(PTTW_INT, PTTW_STRING, PTTW_INT);
int pttw_register_save_parameter(PTTW_NAME, ...);
PSTR pttw_save_string(PTTW_STRING, PTTW_PARM);
void pttw_process_results(HINTERNET, PTTW_STRING, LPVOID);
int pttw_set_debug_message(int, int);
int pttw_set_max_html_param_len(PTTW_STRING);
PSTR pttw_toupper(PSTR);
void ReadRegistryString(HKEY hKeyRoot, const char*, const char*, PTTW_STRING**);
#endif // _GLOBALS_H