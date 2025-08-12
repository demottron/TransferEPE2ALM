#include <stdio.h>
#include "globals.h"
#include "enums_ref.h"

end_Program()
{
	PSTR readBuffer = NULL, restPath = NULL;

/*	if (DEBUGMODE)
		printf("\n\nDELETing /site-session...");

	restPath = pttw_build_restPath(ALMbase, ALMrest, ALMsession, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[rDELETE],
		HEADERS,
			"COOKIE: ","LWSSO_COOKIE_KEY={ALM_LWSSO_KEY}; ", "QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;
*/
	if (DEBUGMODE)
		printf("\n\nCalling /logout...");

	restPath = pttw_build_restPath(ALMbase, AUTHpoint, Logout, NULL);
	pttw_rest(HTTPSscheme, ALMserver, restPath, readBuffer, rtype[POST],
		HEADERS,
			"COOKIE: ", "LWSSO_COOKIE_KEY={ALM_LWSSO_KEY};QCSession={QCSession}", CRLF, ENDHEADER,
		LAST);
	free(restPath);
	free(readBuffer);
	restPath = readBuffer = NULL;

	if (DEBUGMODE)
		printf("\n\nLOGGING out of EPE...");

	restPath = pttw_build_restPath(LREbase, AUTHpoint, Logout, NULL);
	pttw_rest(HTTPscheme, LREserver, restPath, readBuffer, rtype[GET],	
		HEADERS,
			"COOKIE: ","LWSSO_COOKIE_KEY={LRE_LWSSO_KEY}", CRLF, ENDHEADER,
        LAST);
	free(restPath);
	free(readBuffer);
	
	pttw_dump_parm_pointers();

	free(ALMserver);																// release the space holding the server names
	free(LREserver);

	return 0;
}
