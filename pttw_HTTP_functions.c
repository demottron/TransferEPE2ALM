#include <stdio.h>
#include <stdarg.h>  /* Added for va_list */
#include <string.h>  /* Added for string functions */
#include <windows.h>
#include <winhttp.h>  /* Added for WinHttp functions */
#include <strsafe.h>  /* For StringCchPrintf */
#include "globals.h"
#include <winnt.h>
#include "enums_ref.h"

/* Add pragma to link with WinHttp library */
#pragma comment(lib, "winhttp.lib")

struct header {
	union {
		int headCount;          // keep track of how many structures get generated and stored in the standalone instance of pttw_parms_head
		struct header* prev;	// points to previous member. Null for head of chain
	} u1;
	union {
		struct header* next;	// points to next member. Null for end of chain.
		struct header* head;    // pointer to tail end of linked list and stored in the standalone instance of pttw_parms_head
	} u2;                       // implementing this reimagining of the structure will require a great amount of precision and attention to detail
	union {
		PTTW_STRING value;		// string value getting passed to Web engine
		struct header* tail;    // point to first allocated structure in the standalone instance of pttw_parms_head
	} u3;
} pttw_header_head;

struct body {
	union {
		int bodyCount;          // keep track of how many structures get generated and stored in the standalone instance of pttw_parms_head
		struct body* prev;		// points to previous member. Null for head of chain
	} u1;
	union {
		struct body* next;		// points to next member. Null for end of chain.
		struct body* head;      // pointer to tail end of linked list and stored in the standalone instance of pttw_parms_head
	} u2;                       // implementing this reimagining of the structure will require a great amount of precision and attention to detail
	union {
		PTTW_STRING value;		// string value getting passed to REST engine
		struct body* tail;      // point to first allocated structure in the standalone instance of pttw_parms_head
	} u3;
} pttw_body_head;

/* Function prototypes */
int pttw_check_for_parms(PSTR *pBuffer, DWORD *cbBuffer);
void pttw_process_results(HINTERNET hRequest, PTTW_STRING scheme, LPVOID readBuffer);
LPCWSTR pttw_convert_to_wide(LPCCH src, int *len);

/*
// ErrorExit function prototype
// Implementation of ErrorExit function
void ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
*/

HINTERNET openServerConnection(void) {
	HINTERNET lreSession;
	int len = 0;
	LPCCH agent = "TransferLREtoALM";
	LPCWSTR wAgent;
	wAgent = pttw_convert_to_wide(agent,&len);
    // Use WinHttpOpen to obtain a session handle.
    lreSession=WinHttpOpen(wAgent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0); // WINHTTP_FLAG_SECURE_DEFAULTS
	if (!lreSession) {
		wprintf(L"WinHttpOpen failed with 0x%x\n", GetLastError());
		return (HINTERNET)0;
	}
	return lreSession;
}

HINTERNET createServerConnection(HINTERNET hSession, LPCWSTR ServerName, INTERNET_PORT port) {
	HINTERNET pttwSession;

	pttwSession = WinHttpConnect(hSession, ServerName, port, 0);
	if (!pttwSession) {
		wprintf(L"WinHttpConnect failed connecting to %s:%d with 0x%x\n", ServerName, port, GetLastError());
		return (HINTERNET)0;
	}
	return pttwSession;
}

int pttw_increase_timeouts(HINTERNET hSession, int tOption) {
	BOOL bResults;
	static int nResolveTimeout = RESOLVETIMEOUT, nConnectTimeout = CONNECTTIMEOUT, nSendTimeout = SENDTIMEOUT, nReceiveTimeout = RECEIVETIMEOUT;

	switch (tOption) {
		case RESOLVEREQUEST:												// according to https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpsettimeouts name resolution is already infinite, doesn't make any sense to increase it...
			break;
		case CONNECTREQUEST:												// CONNECT timings are not an issue, so timeout from WinHttpOpen not trapped
			nConnectTimeout += CONNECTTIMEOUT;
			break;
		case SENDREQUEST:													// 1.1.4 DBMS slowness workaround
			nSendTimeout += SENDTIMEOUT;									// only SEND and RECEIVE timeout checks implemented in pttw_send_rest
			break;
		case RECEIVEREQUEST:												// 1.1.4 DBMS slowness workaround
			nReceiveTimeout += RECEIVETIMEOUT;								// only SEND and RECEIVE timeout checks implemented in pttw_send_rest
			break;
		default:
			break;
	}

	if (!(bResults = WinHttpSetTimeouts(hSession, nResolveTimeout, nConnectTimeout, nSendTimeout, nReceiveTimeout)))
		ErrorExit(TEXT("Failure in pttw_increase_timeouts"));
    
    return bResults;
}

int pttw_send_rest(PTTW_STRING scheme, PTTW_STRING* svr, PTTW_TYPE rest_type, PSTR readBuffer, PTTW_URL url, struct header* lhptr, struct body* lbptr) {
	BOOL bResults;
	DWORD ulBytesAvailable = 0, ulBytesRead = 0, ulDataLen = 0, dwFlags = 0, dwOptionData = 0, dwOptionDataSize = sizeof(DWORD), ulHeaderLen = 0, ulStrLen = 0, ulTotalBytes = 0, ulTotalLen = 0;
	int cchWideChar = 0, iCharLen = 0, iIncreaseTimeout = 0, len = 0, offset = 0, rbOffset = 0, rc = 0, iRetryReceiveRequest = 0, iRetrySendRequest = 0, iTimeOutType = 0;
	LPCWSTR wAcceptType = NULL;
	LPCWSTR wAcceptTypes[] = { wAcceptType, NULL };
	LPCWSTR wHeaders = NULL, wRestType = NULL, wSvr = NULL, wUrl = NULL;
	PSTR data = NULL, headerBuffer = NULL, tmp = NULL, tmp2 = NULL;
	HINTERNET lreSession=NULL, lreConnect=NULL, hRequest=NULL;
	INTERNET_PORT port;

	wAcceptType = pttw_convert_to_wide(XML,&len);
	memcpy((void*)(wAcceptTypes), &wAcceptType, sizeof(LPCWSTR));
	wRestType = pttw_convert_to_wide(rest_type, &len);
	wUrl = pttw_convert_to_wide(url, &len);
	wSvr = pttw_convert_to_wide((LPCCH)svr, &len);

	if (lhptr != NULL) {									// header structure is where/how the headers is/are sent to REST engine
		do {
			if (!(lhptr->u3.value))
				break;
			ulHeaderLen += (ulStrLen = (unsigned long)strlen(lhptr->u3.value));
			headerBuffer = realloc(headerBuffer, ulHeaderLen + 1);
			if (headerBuffer) {				// allocate enough space for header PRN
				memset(headerBuffer + offset, 0, ulStrLen + 1);
				strcat_s(headerBuffer, ulHeaderLen + 1, lhptr->u3.value);
				offset += ulStrLen;
			}
			else {
				ErrorExit(TEXT("Failure allocating space for header buffer"));
				return -1;
			}
			if (lhptr->u2.next)
				lhptr = lhptr->u2.next;
			else
				break;
		} while (1);
		do {															// lbptr is at the end of the chain from the above loop, ergo, lbptr->next is null
			free(lhptr->u3.value);										// get rid of the data pointer
			lhptr->u3.value = NULL;
			if (lhptr->u1.prev) {										// if there's a previous element, move there
				lhptr = lhptr->u1.prev;
				free(lhptr->u2.next);
				lhptr->u2.next = NULL;
			}
			else
				break;													// no further elements, stop looping
		} while (1);
		free(lhptr);													// and free the main lbptr pointer
		lhptr = NULL;
		pttw_header_head.u1.headCount = 0;								// clear the count of body parts
		pttw_header_head.u2.head = NULL;								// .next field points to head of chain
		pttw_header_head.u3.tail = NULL;								// .tail shows the end
	}
	iCharLen = (int)ulHeaderLen;
	if (pttw_check_for_parms(&headerBuffer, &ulHeaderLen) < 0) {		// pull parameter data which is supposed to be sent in as header info
		ErrorExit(TEXT("Header parameter conversion failure in pttw_send_rest"));
		rc = -1;
		goto RestExit;
	}
	wHeaders = pttw_convert_to_wide(headerBuffer, &ulHeaderLen);
	free(headerBuffer);
	headerBuffer = NULL;

	offset = 0;												// rest offset for use below
	if (lbptr != NULL) {									// body structure is where/how the information such as authentication information is sent to REST engine
		do {
			if (!(lbptr->u3.value))
				break;

			ulDataLen += (ulStrLen = (unsigned long)strlen(lbptr->u3.value));
			data = realloc(data, ulDataLen + 1);
			if (data) {				// allocate enough space for body part
				memset(data + offset, 0, ulStrLen + 1);
				strcat_s(data, ulDataLen + 1, lbptr->u3.value);
				offset += ulStrLen;
			}
			else {
				ErrorExit(TEXT("Failure allocating space for data buffer"));
				return -1;
			}
			if (lbptr->u2.next)
				lbptr = lbptr->u2.next;
			else
				break;
		} while (1);
		do {															// lbptr is at the end of the chain from the above loop, ergo, lbptr->next is null
			free(lbptr->u3.value);										// get rid of the data pointer
			lbptr->u3.value = NULL;
			if (lbptr->u1.prev) {										// if there's a previous element, move there
				lbptr = lbptr->u1.prev;
				free(lbptr->u2.next);
				lbptr->u2.next = NULL;
			}
			else
				break;													// no further elements, stop looping
		} while (1);
		free(lbptr);													// and free the main lbptr pointer
		lbptr = NULL;
		pttw_body_head.u1.bodyCount = 0;								// clear the count of body parts
		pttw_body_head.u2.head = NULL;									// .next field points to head of chain
		pttw_body_head.u3.tail = NULL;									// .tail shows the end

		if (pttw_check_for_parms(&data, &ulDataLen) < 0) {		// pull parameter data which is supposed to be sent in as header info
			ErrorExit(TEXT("Data parameter conversion failure in pttw_send_rest"));
			rc = -1;
			goto RestExit;
		}
		ulTotalLen = ulDataLen;											// set total length after ulDataLen checks for parms
	}

	retryOperation:
	if (!(lreSession = openServerConnection())) {
		ErrorExit(TEXT("openServerConnection failure"));
		rc = -1;
		goto RestExit;
	}
	
	if (iIncreaseTimeout) {												// 1.1.4 DBMS slowness workaround
		iIncreaseTimeout = 0;
		pttw_increase_timeouts(lreSession, iTimeOutType);
	}

	if (DEBUGMODE) {
		if (WinHttpQueryOption(lreSession, WINHTTP_OPTION_SEND_TIMEOUT, &dwOptionData, &dwOptionDataSize))
			printf("\n\nSend timeout: %u ms", dwOptionData);
		if (WinHttpQueryOption(lreSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &dwOptionData, &dwOptionDataSize))
			printf("\nReceive timeout: %u ms", dwOptionData);
		if (WinHttpQueryOption(lreSession, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &dwOptionData, &dwOptionDataSize))
			printf("\nReceive response timeout: %u ms", dwOptionData);
	}

	if (!strcmp(scheme, "HTTP"))
		port = INTERNET_DEFAULT_HTTP_PORT;
	else {
		port = INTERNET_DEFAULT_HTTPS_PORT;
		dwFlags = WINHTTP_FLAG_SECURE;
	}
	if (!(lreConnect = createServerConnection(lreSession,wSvr,port))){
		ErrorExit(TEXT("createServerConnection failure"));
		rc=-1;
		goto closeSessionExit;
	}

	if (!(hRequest = WinHttpOpenRequest(lreConnect, wRestType, wUrl, NULL, WINHTTP_NO_REFERER, wAcceptTypes, dwFlags))) {
		ErrorExit(TEXT("WinHttpOpenRequest failure"));
		rc= -1;
		goto closeConnectHandle;
	}

	if (!(bResults = WinHttpSendRequest(hRequest, wHeaders, -1, data, ulDataLen, ulTotalLen, 0))) { // 1.1.4 DBMS slowness workaround
		if (GetLastError() == ERROR_WINHTTP_TIMEOUT) {
			if (iRetrySendRequest++ < MAX_RETRIES) {
				iIncreaseTimeout = 1;
				iTimeOutType = SENDREQUEST;
				printf("\n\nSend request for %s timed out, retrying...", url);
				goto retryOperation;
			}
			else
				printf("\n\nExhausted send request attempts for %s, exiting...", url);			// 1.1.4 DBMS slowness workaround
		}
		ErrorExit(TEXT("WinHttpSendRequest failure"));
		rc = -1;
		goto closeRequestHandle;
	}

	if (!(bResults = WinHttpReceiveResponse(hRequest, NULL))) {								// 1.1.4 DBMS slowness workaround
		if (GetLastError() == ERROR_WINHTTP_TIMEOUT) {
			if (iRetryReceiveRequest++ < MAX_RETRIES) {
				iIncreaseTimeout = 1;
				iTimeOutType = RECEIVEREQUEST;
				printf("\n\nReceive request for %s timed out, retrying...", url);
				goto retryOperation;
			}
			else
				printf("\n\nExhausted receive request attempts for %s, exiting...", url);	// 1.1.4 DBMS slowness workaround 
		}
		ErrorExit(TEXT("WinHttpReceiveResponse failure"));
		rc = -1;
		goto closeRequestHandle;
	}

	offset = 0;
	do {
		if (!(bResults = WinHttpQueryDataAvailable(hRequest, &ulBytesAvailable))) {
			ErrorExit(TEXT("WinHttpQueryDataAvailable failure"));
			rc = -1;
			goto closeRequestHandle;
		}
		else {
			if (ulBytesAvailable) {
				readBuffer = realloc(readBuffer, ulBytesAvailable + offset + 1);
				if (!readBuffer) {				// allocate enough space for header PRN
					ErrorExit(TEXT("Failure allocating space for headerBuffer in pttw_send_rest"));
					return -1;
				}
				else {
					memset(readBuffer + offset, 0, ulBytesAvailable + 1);
					if (!(bResults = WinHttpReadData(hRequest, (LPVOID)(readBuffer + offset), ulBytesAvailable, &ulBytesAvailable))) {
						ErrorExit(TEXT("WinHttpReadData failure"));
						rc = -1;
						goto closeRequestHandle;
					}
					offset += ulBytesAvailable;
				}
			}
		}
	} while (ulBytesAvailable > 0);

	pttw_process_results(hRequest, scheme, readBuffer);							// psr will take care of searching the payload and storing the desired data, if any, in the links structure

closeRequestHandle:
	if (!(bResults = WinHttpCloseHandle(hRequest))) {
		ErrorExit(TEXT("WinHttpCloseRequest failure"));
		rc = -1;
	}
closeConnectHandle:
	if (!(bResults = WinHttpCloseHandle(lreConnect))) {
		ErrorExit(TEXT("WinHttpCloseConnect failure"));
		rc = -1;
	}
closeSessionExit:
	if (!(bResults = WinHttpCloseHandle(lreSession))) {
		ErrorExit(TEXT("WinHttpCloseSession failure"));
		rc = -1;
	}
RestExit:

	return rc;
}

int pttw_rest(PTTW_STRING scheme, PTTW_STRING* svr, PTTW_URL url, PSTR readBuffer, PTTW_TYPE type, ...) {
	unsigned char endheaderFound = 0, headerFound = 0, lastParameterFound = 0;
	unsigned long bodyCounter = 0, headCounter = 0, len = 0;
	struct header* lhptr = NULL;
	struct body* lbptr = NULL;
	PSTR lptr;
	va_list ap;

	va_start(ap, type);							// Initialize to last known good parameter

	do {
		lptr = va_arg(ap, char*);				// starting point was call type
		if (!lptr)
			break;

		if (!_stricmp(lptr, "headers")) {		// using case insensitive comparison just in case...
			headerFound = 1;					// but since this program probably isn't going to be maintained by anyone other than the author
			continue;							// it's not likely that any one else is going to inadvertently mix-case these keywords
		}
		if (!_stricmp(lptr, "endheader")) {
			endheaderFound = 1;
			continue;
		}
		if (!_stricmp(lptr, "last")) {
			lastParameterFound = 1;
			break;
		}
		if (!headerFound) {						// the BODY data in pttw_rest calls needs to be defined prior to the headers because of this if statement
			if (bodyCounter) {
				lbptr->u2.next = malloc(sizeof(struct body));
				if (!lbptr->u2.next)       	// generate the next slot and put the new address into current's next pointer
					return -1;										        // make sure there wasn't a memory allocation failure
				bodyCounter++;
				memset(lbptr->u2.next, 0, sizeof(struct body));			    // zero out the new address space
				lbptr->u2.next->u1.prev = lbptr;          					// establish linkage to current slot
				lbptr = lbptr->u2.next;				            			// make the new slot the current object
				pttw_body_head.u1.bodyCount = bodyCounter;					// keep track of how many body parts are in existence
				pttw_body_head.u3.tail = lbptr;								// bump tail to newly created element
			}
			else {
				lbptr = malloc(sizeof(struct body));
				if (!lbptr)					// create initial slot
					return -1;
				memset(lbptr, 0, sizeof(struct body));						// zero out the memory space
				bodyCounter = 1;
				pttw_body_head.u1.bodyCount = bodyCounter;					// begin initial count of body parts
				pttw_body_head.u2.head = lbptr;								// .next field points to head of chain
				pttw_body_head.u3.tail = lbptr;								// .tail shows the end
			}
			len = (int)strlen(lptr);
			lbptr->u3.value = malloc(len + 1);
			if (lbptr->u3.value) {	// allocate space for the body data
				memset(lbptr->u3.value, 0, len + 1);
				memcpy(lbptr->u3.value, lptr, len);							// copy over this parameter to this address space
			}
			else
				return -1;
			continue;
		}
		if (headCounter) {
			lhptr->u2.next = malloc(sizeof(struct header));
			if (!lhptr->u2.next)     	// generate the next slot and put the new address into current's next pointer
				return -1;										        // make sure there wasn't a memory allocation failure
			headCounter++;
			memset(lhptr->u2.next, 0, sizeof(struct header));		    // zero out the new address space
			lhptr->u2.next->u1.prev = lhptr;          					// establish linkage to current slot
			lhptr = lhptr->u2.next;				            			// make the new slot the current object
			pttw_header_head.u1.headCount = headCounter;				// keep track of how many body parts are in existence
			pttw_header_head.u3.tail = lhptr;							// bump tail to newly created element
		}
		else {
			lhptr = malloc(sizeof(struct header));
			if (!lhptr)					// create initial slot
				return -1;
			memset(lhptr, 0, sizeof(struct header));						// zero out the memory space
			headCounter = 1;
			pttw_header_head.u1.headCount = headCounter;					// begin initial count of body parts
			pttw_header_head.u2.head = lhptr;								// .next field points to head of chain
			pttw_header_head.u3.tail = lhptr;								// .tail shows the end
		}
		len = (int)strlen(lptr);
		lhptr->u3.value = malloc(len + 1);
		if (lhptr->u3.value) {	// allocate space for the header data
			memset(lhptr->u3.value, 0, len + 1);
			memcpy(lhptr->u3.value, lptr, len);							// copy over this parameter to this address space
		}
		else
			return -1;
	} while (1);
	va_end(ap);
	return pttw_send_rest(scheme, svr, type, readBuffer, url, (struct header*)(pttw_header_head.u2.head),(struct body*)(pttw_body_head.u2.head));
}