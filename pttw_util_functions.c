#include "enums_ref.h"
#include "globals.h"
#include <stdio.h>
#include <strsafe.h>

struct parms {
    union {
        int parmCount;          // keep track of how many structures get generated
        struct parms* prev;		// points to previous member. Null for head of chain
    } u1;
    PTTW_PARM name; 			// parameter name
    unsigned char array;        // array indicator
    PTTW_STRING value;			// string value
    union {
        struct parms* next;		// points to next member. Null for end of chain.
        struct parms* head;     // point to first allocated structure
    } u2;
} pttw_parms_head;

struct parameters {
    union {
        int parmCount;          // keep track of how many structures get generated
        struct parameters* prev;		// points to previous member. Null for head of chain
    } u1;
    PTTW_PARM name; 			// parameter name
    PTTW_STRING left;			// left boundary value
    PTTW_STRING right;			// right boundary value
    PTTW_STRING ordinal;		// how many to save
    PTTW_STRING location;		// where to look (optional)
    PTTW_STRING value;			// content
    union {
        struct parameters* next;			// points to next member. Null for end of chain.
        struct parameters* head;     // point to first allocated structure
    } u2;
} pttw_parameters_head;

PSTR pttw_find_parm(PSTR target, DWORD* element) {                              // a) element == -3 - looks for the first matching parm in links list and returns the entry's address (pdp)
    struct parms* llptr;                                                        // b) element == -2 - looks for the first matching parm in links list and returns the entry's address (psp)
    int arrayFound = 0, countElements = 0, elementsCount = 0, i, len;           // c) element == -1 - count the number of array members and return them in element
    int dropAddress=0, returnAddress = 0;                                       // d) element == 0 - looks for the first matching parm in links list and returns its value
                                                                                // e) element == <n> - element specifies which array member's value to return
    len = (int)strlen(target);                                                  // need to only compare the base name - underscore number follows if array flag is set
    if (!(element))
        countElements = 0;
    else {
        if (*element == COUNTMEMBERS)
            countElements = 1;
        else
            if (*element == GETADDRESS)
                returnAddress = 1;
        else
            if (*element == DROPPARM)
                dropAddress = 1;
    }
    for (i = 0, llptr = pttw_parms_head.u2.head; i < pttw_parms_head.u1.parmCount; i++) {
        if (!strncmp(target, llptr->name, len)) {           // in order to search for "arrays" of parameters, limit string compare to length of base name (stop compare at underscore)
            if (llptr->array) {                             // confirm the parm name is part of an array...check the array character
                arrayFound = 1;                             // since all parms get found sequentially, once the first one is found, the following elements will be an array member until there aren't any more
                if (dropAddress||returnAddress)                 // need to remove parm array for grpVusers, grpId, and Script (GLE)
                    return (PSTR)llptr;
//                if (returnAddress)                          // pttw_store_parms check to see whether parm already exists
//                  return (PSTR)llptr;                     //          and so needs to be overwritten
            }
            else {
                if (!strcmp(target, llptr->name))               // make sure it's truly a match (not part of an array), so compare entire string
                    if (returnAddress)                          // pttw_store_parms check to see whether parm already exists
                        return (PSTR)llptr;                     //          and so needs to be overwritten
            }
            if ((element > 0) && (!countElements)) {           // presense of non-zero value means the call is looking for a specific occurrence
                --(*element);                                   // decrement the counter because
                if (!(*element))                                // whether element comes in as zero or counts down, exit when it is zero
                    return llptr->value;
                if (llptr->u2.next)                             // if there's another list member
                    llptr = llptr->u2.next;                     // move on to next one
                continue;
            }
            if (!element)                                       // whether element comes in as zero or counts down, exit when it is zero
                return llptr->value;
            ++elementsCount;                                    // counting the array members
            if (llptr->u2.next)                                 // fell down through the "found" logic - check for and
                llptr = llptr->u2.next;                         // move on to next one
            continue;
        }
        if (llptr->u2.next)                                     // only get here if no match found, so
            llptr = llptr->u2.next;                             // move on to next one
    }
    if ((arrayFound) && (countElements))                        // if string compare didn't find the two latest strings to be matches, the end of the array has been encountered - time to go
        *element = elementsCount;                               // pass back how many array members were encountered
    return (void*)0;                                            // if no parm was found with the name of <target>, send back a null pointer
}

PSTR pttw_store_parms(PTTW_PARM name,PTTW_STRING target,PSTR ordinal) {
    static struct parms* llHead, * llptr, *llTail;                       // static data remains valid across function invocations
    struct parms* llptrOverwrite = NULL;
    int allocCtr = 0, buflen, ordOverwrite = GETADDRESS;
    char buffer[256];
    PSTR tmp = NULL;
    
    if (DEBUGMODE)
        pttw_display_parms('0');

    if (target == NULL && !ordinal) {
        llptr = (struct parms*)(pttw_find_parm(name, &ordOverwrite));
        return (char*)1;
    }

    if (atoi(ordinal) > 0)
        buflen = sprintf_s(buffer, sizeof(buffer), "%s_%s", name, ordinal);
    else
        buflen = sprintf_s(buffer, sizeof(buffer), "%s", name);

    // pass in ordFaux to only look for the parm name in order to overwrite...
    if (llptrOverwrite = (struct parms*)(pttw_find_parm(buffer, &ordOverwrite))) {
        buflen = ((int)strlen(target) + 1);
        tmp = realloc(llptrOverwrite->value, buflen);
        if (!tmp)
            return (char*)0;
        llptrOverwrite->value = tmp;
        strcpy_s(llptrOverwrite->value, buflen, target);
        return llptrOverwrite->name;
    }

    if (pttw_parms_head.u1.parmCount) {
        llTail = llptr->u2.next = malloc(sizeof(struct parms)); // generate the next slot and put the new address into current's next pointer
        if (!llptr->u2.next)							        // make sure there wasn't a memory allocation failure
            return (char*)0;
        memset(llptr->u2.next, 0, sizeof(struct parms));        // zero out the new address space
        llptr->u2.next->u1.prev = llptr;          				// establish linkage to current slot
        llptr = llptr->u2.next;				            		// make the new slot the current object
        pttw_parms_head.u1.parmCount++;
    }
    else {
        llTail = llHead = llptr = malloc(sizeof(struct parms));	// create initial slot plus holding pointers to head and tail
        if (!llptr)
            return (char *)0;
        memset(llptr, 0, sizeof(struct parms));				    // zero out the memory space
        pttw_parms_head.u1.parmCount = 1;
        pttw_parms_head.u2.head = llHead;                          // .next field points to head of chain
    }
    if (atoi(ordinal)) {
        buflen = sprintf_s(buffer, sizeof(buffer), "%s_%s", name, ordinal);
        llptr->array = 0x1;
    }
    else
        buflen = sprintf_s(buffer, sizeof(buffer), "%s", name);
    if (buflen < 0)
        return (void*)-1;
    llptr->name = malloc(++buflen);                             // need that extra byte for the NULL
    if (!llptr->name)
        return (char*)0;
    strcpy_s(llptr->name, buflen, buffer);
    llptr->value = malloc(buflen = ((int)strlen(target) + 1));
    if (!llptr->value)
        return (char*)0;
    strcpy_s(llptr->value, buflen, target);
    if (DEBUGMODE)
        pttw_display_parms('1');
    return llptr->name;
}

int pttw_check_for_parms(PSTR* buffer, PDWORD ulLen) {            // inspect string for delimiter token (parameter name)
    PSTR lptr = *buffer, lptr2, lptr3, lptr4, parmName = NULL, parmValue = NULL, tmp = NULL;
    DWORD foundLdelim = 0, foundRdelim = 0, len, leftLength = 0, leftLengthAlt =0, parmLen = 0, rc = 0, replacementLength, rightLength = 0, totalLength;

    while (*lptr != LEFTPARMDELIMINATOR) {        // locate '{'
        if (*(lptr + 1)) {                       // check for more array members
            ++lptr;
            ++leftLength;
        }
        if (!(*(lptr + 1))) {
            return 1;                             // no parm found, leave, and indicate no further converstion required
        }
    }
    foundLdelim = 1;                               // if we got here, set a marker in the sand
    lptr2 = ++lptr;                                 // establish that marker
    while (*lptr != RIGHTPARMDELIMINATOR) {        // locate '}'
        if (*(lptr + 1))                          // check for more character array elements
            ++lptr;
        if (*lptr == LEFTPARMDELIMINATOR) {        // pointer moved right, check for nested delimeters
            lptr4 = lptr2;                       // just move past the delimeter
            leftLengthAlt = leftLength;          // and account for the movement
            do {
                ++lptr2;                            // reset marker to capture inner parmName
                ++leftLengthAlt;     // prolly ditch this if leftLength can safely continue to be used
                ++leftLength;
            } while (*lptr2 != *lptr);
            ++lptr2;
            ++leftLength;
        }
        if (*lptr == RIGHTPARMDELIMINATOR) {        // pointer moved right, check again...
            foundRdelim = 1;
            break;
        }
        if (!(*(lptr + 1))) {
            if (foundLdelim) {
                ErrorExit(TEXT("Catastrophic failure looking for right parm"));     // alternatively, this could just be "<NAME> with delimeters is not a parameter
                return -1;                                                   // or a left brace without a right brace is nothing to be concerned about
            }
            else {
                return 1;                                                   // nothing to see here...go home, and don't come back
            }
        }
    }
    lptr3 = lptr;                       // find remainder of buffer in order to properly rebuild entire string
    while (*lptr3) {                     // look for trailing NULL
        if (*(lptr3 + 1)) {
            ++lptr3;
            ++rightLength;
        }
        if (!(*(lptr3 + 1))) {          // found NULL
            break;                      // time to stop
        }
    }
    if (!(parmName = malloc((len = (DWORD)(lptr - lptr2)) + 1))) {          // create some space for the parameter name
        ErrorExit(TEXT("Failure allocating memory for parmName in pttw_check_for_parms"));
        return -1;
    }
    else {
        memset(parmName, 0, len + 1);
        memcpy(parmName, lptr2, len);                                       // fill in the name
    }
    if (!(parmValue = pttw_find_parm(parmName,NOINDEX)))                    // go look it up in the linked list
        return 1;

    replacementLength = (int)strlen(parmValue);                             // find out how much space to allocate
    totalLength = leftLength + replacementLength + rightLength + 1;         // tally left, center, and right strings
    if (!(tmp = realloc(tmp, totalLength))) {                           // temporarily create a temporary buffer
        ErrorExit(TEXT("Failure allocating memory for replacement string in pttw_check_for_parms"));
        return -1;
    }
    else {
        memset(tmp, 0, totalLength);                                        // initialize the memory space
        memcpy(tmp, *buffer, leftLength);                                   // copy in the beginning of the buffer
        strcat_s(tmp, totalLength, parmValue);                              // insert the replacement text
        strcat_s(tmp, totalLength, ++lptr);                                 // attach the right side of the buffer
    }
    *buffer = tmp;                                                      // reattach to real pointer...
    *ulLen = --totalLength;
    free(parmName);

    do {
        rc = pttw_check_for_parms(buffer, ulLen);
    } while (rc != 1);

    return 0;
}

void pttw_display_parms(char ch) {
    struct parms* nextLink, *holdLink = NULL;
    int i,j=0,firstTime = 1;
    size_t alen, blen;
    
    alen = strlen("grpVusers");
    blen = strlen("grpId");

    for (i = 1, nextLink = pttw_parms_head.u2.head; i <= pttw_parms_head.u1.parmCount; i++) {
        if (nextLink->u2.next) {
            nextLink = nextLink->u2.next;      // move on to next one
            ++j;
        }
    }
    if (j && (j != (i-2)))
        printf("\n\n\nparmCount is %d, but actual element count is %d", i, j);

    for (i = 1, nextLink = pttw_parms_head.u2.head; i <= pttw_parms_head.u1.parmCount; i++) {
//        if (i>290)
        if ((!strncmp(nextLink->name, "grpVusers",alen))|| (!strncmp(nextLink->name, "grpId", blen))||(ch == 's'&&i>0x108)) {
            if (firstTime) {
                if (nextLink->array)
                    printf("\n%c%03d) %s\t(array)\t%s", ch, i - 1, nextLink->u1.prev->name, nextLink->u1.prev->value);
                else
                    printf("\n%c%03d) %s\t%s", ch, i - 1, nextLink->u1.prev->name, nextLink->u1.prev->value);
                //        printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
                printf("\n%c%03d) PreGROUP %p\tPrev %p\tNext %p", ch, i - 1, nextLink->u1.prev, nextLink->u1.prev->u1.prev, nextLink->u1.prev->u2.next);
                //        printf("\n***************************************************");
                firstTime = 0;
            }
            if (nextLink->array)
                printf("\n%03d. %s\t(array)\t%s", i, nextLink->name, nextLink->value);
            else
                printf("\n%03d. %s\t%s", i, nextLink->name, nextLink->value);
            //        printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
            printf("\n%c) Current %p\tPrev %p\tNext %p", ch, nextLink, nextLink->u1.prev, nextLink->u2.next);
            //        printf("\n***************************************************");
            if (nextLink) {
                if (nextLink->u1.prev == 0)
                    printf("\t\t\t$$$ HEAD is %p", nextLink);
                if (nextLink->u2.next == 0)
                    printf("\t\t\t$$$ TAIL is %p", nextLink);
                if (holdLink)
                    if ((holdLink->u2.next != nextLink) || (holdLink != nextLink->u1.prev))
                        printf("### hold (%p) -> next (%p) != next (%p) -> prev (%p) ###", holdLink, holdLink->u2.next, nextLink, nextLink->u1.prev);
            }
        }
        if (nextLink->u2.next == 0) {
            if (i < (pttw_parms_head.u1.parmCount - 1)) {
                printf("\n\n\n\n\t\t\t$$$ Possible broken chain at %p", nextLink);
                if (nextLink->array)
                    printf("\n%03d. %s\t(array)\t%s", i, nextLink->name, nextLink->value);
                else
                    printf("\n%03d. %s\t%s", i, nextLink->name, nextLink->value);
                printf("\n%c) Current %p\tPrev %p\tNext %p", ch, nextLink, nextLink->u1.prev, nextLink->u2.next);
            }
        }
        if (nextLink->u2.next) {
            holdLink = nextLink;
            nextLink = nextLink->u2.next;      // move on to next one
        }
    }
    return;
}

int pttw_drop_last_parameters(struct parameters* plptr) {          // return a pointer to the last define parameter search info

    if (plptr->u2.next)
        return -1;                                                  // there shouldn't be a pointer to next!

    if (plptr->u1.prev) {
        plptr = plptr->u1.prev;
        free(plptr->u2.next);
        plptr->u2.next = NULL;
    }
    else {
        free(plptr);
        plptr->u2.next = NULL;
    }

    if (pttw_parameters_head.u1.parmCount > 0) {
        --pttw_parameters_head.u1.parmCount;
        if (!pttw_parameters_head.u1.parmCount)         // if there's no more parameters
            pttw_parameters_head.u2.next = NULL;        // there's nothing to point at
    }
    return pttw_parameters_head.u1.parmCount;
}

int pttw_drop_parm(PTTW_PARM name) {                                // remove a parm entry in the linked list
    int ordDrop = DROPPARM;                                         // creation of this function seemed to be the best way to deal with
    struct parms* llptr = NULL;                                     // the addition of getting script name(s) in GLE which caused
                                                                    // an issue when RANGE/SELECTED Runs were requested
                                                                    // The pttw_store_parms() explicitly ignores arrays resulting in leftover links
                                                                    // thereby causing duplicate links of grpVusers and grpID
    if (llptr = (struct parms*)(pttw_find_parm(name, &ordDrop))) {  // locate the linked list entry
        if (llptr->array) {                                         // attempt to ensure pfp didn't return a false positive
            free(llptr->name);                                      // release some bytes...
            free(llptr->value);
            llptr->array = 0x0;
            if (llptr->u2.next)
                llptr->u2.next->u1.prev = llptr->u1.prev;               // reset next entry's prev pointer to current's prev pointer
            if (llptr->u1.prev)
                llptr->u1.prev->u2.next = llptr->u2.next;               // connect previous entry to replacement next
            if (!llptr->u2.next)
                pttw_store_parms(llptr->u1.prev->name, NULL, NOINDEX);  // need to inform static struct *llptr that this entry is getting deleted
            llptr->u2.next = NULL;
            llptr->u1.prev = NULL;
            free(llptr);
            if (pttw_parms_head.u1.parmCount > 0) {
                --pttw_parms_head.u1.parmCount;                     // keep track of the removal of one item
                if (!pttw_parms_head.u1.parmCount)                  // if there's no more parameters - given that this function is only to remove grpVusers and grpID entries
                    pttw_parms_head.u2.next = NULL;                 // there should never be a reason to null out the head pointer
            }                                                       // but that's defensive programming for you
        }
    }
    else {
        if (DEBUGMODE)
            printf("\n\nPFP did not return a pointer to %s\n\n",name);
        return 0;
    }

    return pttw_parms_head.u1.parmCount;
}

void pttw_dump_parm_pointers(void) {
    struct parms* nextLink;
    int i;
    for (i = 0, nextLink = pttw_parms_head.u2.head; i < pttw_parms_head.u1.parmCount; i++) {
        free(nextLink->name);               // dump the name of the parm
        free(nextLink->value);              // dump the value of the parm
        nextLink->array = 0x0;              // clear the array flag just for completeness' sake
        if (nextLink->u2.next) {
            nextLink = nextLink->u2.next;   // move on to next one
            free(nextLink->u1.prev);        // free the previous spot
        }
    }
    return;
}

void pttw_dump_parameters_pointer(void) {
    struct parameters* nextLink = NULL;
    int i;
    for (i = 0, nextLink = pttw_parameters_head.u2.head; i < pttw_parameters_head.u1.parmCount; i++) {
        free(nextLink->name);               // dump the name of the parameter
        free(nextLink->value);              // dump the value of the parameter
        if (nextLink->u2.next) {
            nextLink = nextLink->u2.next;      // move on to next one
            free(nextLink->u1.prev);     // free the previous spot
        }
    }
    return;
}

int pttw_extract_header_data(HINTERNET hRequest, PTTW_STRING scheme, struct parameters* plptr) {
    /* for use with Build 20348 or greater
        //typedef union _WINHTTP_HEADER_NAME {
        //	PCWSTR pwszName;
        //	PCSTR  pszName;
        //} WINHTTP_HEADER_NAME, * PWINHTTP_HEADER_NAME;
        //typedef struct _WINHTTP_EXTENDED_HEADER {
        //	union {
        //		PCWSTR pwszName;
        //		PCSTR  pszName;
        //	};
        //	union {
        //		PCWSTR pwszValue;
        //		PCSTR  pszValue;
        //	};
        //} WINHTTP_EXTENDED_HEADER, * PWINHTTP_EXTENDED_HEADER;
        //WINHTTP_EXTENDED_HEADER ppHeaders;
        //WINHTTP_HEADER_NAME pHeaderName = { NULL };
            unsigned int uiCodePage;
        int	pdwBufferLength, pdwHeadersCount;
        unsigned long long ullFlags;
        int pdwIndex;
        PSTR pBuffer;

    */
    BOOL bResults;
    DWORD dwInfoLevel = WINHTTP_QUERY_CUSTOM, dwHeaderIndex = 0, len = 0, lpdwBufferLength = 0, pdwHeaderValueLen = 0;
    PSTR lpCookieName = NULL, pbHeaderValue = NULL, lpOutBuffer = NULL;
    LPCWSTR wHeaderName = WINHTTP_HEADER_NAME_BY_INDEX;

    if (bShowHeaders)
        pttw_inspect_header_data(hRequest, scheme, plptr);

    if (!strcmp(scheme, "HTTP"))
        dwHeaderIndex = 1;                                                          // LRE (until Gil upgrades to HTTPS)
    else
        dwHeaderIndex = atoi(plptr->ordinal);                                       // ALM set-cookie index

    if (!_stricmp(plptr->name, LOCATION))
        dwInfoLevel = WINHTTP_QUERY_LOCATION;
    else if (!_stricmp(plptr->name, STATUS))
        dwInfoLevel = WINHTTP_QUERY_STATUS_CODE;
    else
        wHeaderName = pttw_convert_to_wide(plptr->name, &len);                      // this can only be valid when dwInfoLevel set to WINHTTP_QUERY_CUSTOM (default value)

// getNextCookie:
    WinHttpQueryHeaders(hRequest, dwInfoLevel, wHeaderName, WINHTTP_NO_OUTPUT_BUFFER, &lpdwBufferLength, &dwHeaderIndex); // find out how long buffer needs to be
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) { // Allocate memory for the buffer. This is not an error - it is expected behavior
        if (!(lpOutBuffer = malloc(lpdwBufferLength + 1))) { // plus one to terminate the string
            ErrorExit(TEXT("Failure to allocate memory for lpOutBuffer"));
            return -1;
        }
        bResults = WinHttpQueryHeaders(hRequest, dwInfoLevel, wHeaderName, lpOutBuffer, &lpdwBufferLength, &dwHeaderIndex); // Now, use WinHttpQueryHeaders to retrieve the header.
        if (bResults) {
            pdwHeaderValueLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, pbHeaderValue, pdwHeaderValueLen, NULL, NULL);
            if (pdwHeaderValueLen) {
                if (!(pbHeaderValue = malloc(pdwHeaderValueLen+1))) {
                    ErrorExit(TEXT("Failure to allocate memory for pbHeaderValue"));
                    return -1;
                }
                if (!WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, pbHeaderValue, pdwHeaderValueLen, NULL, NULL)) {
                    ErrorExit(TEXT("2nd WideCharToMultiByte failure\n"));
                    return -1;
                }
            }
        // this comparison assumes the order of the returned headers is the same order as the searches were defined
        // which is obviously ridiculous. However, so far, it seems to be working correctly which is a relief because
        // syncing up the returned headers with the order of the defined searches would be problematic.
        // Although it would probably just require searching through the parameter list after a successful call to WinHttpQueryHeaders
        // and after processing a list element, not destroying that element. Call pttw_dump_parameters_pointer once all the headers have been handled.
            //if dwInfoLevel == WINHTTP_QUERY_CUSTOM)
            //    if (_strnicmp(plptr->left, pbHeaderValue, ((int)strlen(plptr->left) - 1))) { // unknown in which case "Set-Cookie:" entry will be returned
            //        free(lpOutBuffer);                  // in any case, dump the buffer
            //        goto getNextCookie;                 // and go get the next header record
            //    }
            pttw_extract_cookies(&pbHeaderValue, plptr);           // pbHeaderValue is freed in peS()
        }                                                                               // no apparent need for returning the name of the cookie
        free(lpOutBuffer);
    }

    if (plptr->u1.prev)
        return 1;
    else
        return 0;
    /*	For use when the rest of the VA has caught up with Build 20348 - this function is not found in the currently deployed winhttp.dll

        dwInfoLevel = WINHTTP_QUERY_CUSTOM || WINHTTP_QUERY_FLAG_WIRE_ENCODING;
        ullFlags = 0;
        uiCodePage = 0;
        pdwIndex = 1;									// LWSSO_COOKIE_KEY has two occurrences
        strcpy_s(pHeaderName.pszName, (int)strlen(LWSSO_COOKIE_KEY), LWSSO_COOKIE_KEY);
        pBuffer = NULL;
        pdwBufferLength = 0;
        memset(&ppHeaders, 0, sizeof(struct _WINHTTP_EXTENDED_HEADER));

        WinHttpQueryHeadersEx(hRequest, dwInfoLevel, ullFlags, uiCodePage, &pdwIndex, &pHeaderName, pBuffer, &pdwBufferLength, &ppHeaders, &pdwHeadersCount);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) { // Allocate memory for the buffer.
            if (!(pBuffer = malloc(pdwBufferLength))) {
                ErrorExit(TEXT("Failure to allocate memory for lpOutBuffer"));
                rc = -1;
                goto closeRequestHandle;
            }
            if (!(bResults = WinHttpQueryHeadersEx(hRequest, dwInfoLevel, ullFlags, uiCodePage, &pdwIndex, &pHeaderName, pBuffer, &pdwBufferLength, &ppHeaders, &pdwHeadersCount))) {
                ErrorExit(TEXT("WinHttpQueryHeadersEx failure"));
                rc = -1;
                goto closeRequestHandle;
            }
        }
    */
}

int pttw_inspect_header_data(HINTERNET hRequest, PTTW_STRING scheme, struct parameters* plptr) {
    BOOL bResults;
    DWORD dwInfoLevel = WINHTTP_QUERY_RAW_HEADERS_CRLF, dwHeaderIndex = 0, len = 0, lpdwBufferLength = 0, pdwHeaderValueLen = 0;
    PSTR lpCookieName = NULL, pbHeaderValue = NULL, lpOutBuffer = NULL;
    LPCWSTR wHeaderName = WINHTTP_HEADER_NAME_BY_INDEX;
    
    if (!strcmp(scheme, "HTTP"))
        dwHeaderIndex = 1;                                                          // LRE (until Gil upgrades to HTTPS)
    else
        dwHeaderIndex = atoi(plptr->ordinal);                                       // ALM set-cookie index
    
    // get all headers:

    WinHttpQueryHeaders(hRequest, dwInfoLevel, wHeaderName, WINHTTP_NO_OUTPUT_BUFFER, &lpdwBufferLength, &dwHeaderIndex); // find out how long buffer needs to be
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) { // Allocate memory for the buffer. This is not an error - it is expected behavior
        if (!(lpOutBuffer = malloc(lpdwBufferLength + 1))) { // plus one to terminate the string
            ErrorExit(TEXT("Failure to allocate memory for lpOutBuffer"));
            return -1;
        }
        bResults = WinHttpQueryHeaders(hRequest, dwInfoLevel, wHeaderName, lpOutBuffer, &lpdwBufferLength, &dwHeaderIndex); // Now, use WinHttpQueryHeaders to retrieve the header.
        if (bResults) {
            pdwHeaderValueLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, pbHeaderValue, pdwHeaderValueLen, NULL, NULL);
            if (pdwHeaderValueLen) {
                if (!(pbHeaderValue = malloc(pdwHeaderValueLen + 1))) {
                    ErrorExit(TEXT("Failure to allocate memory for pbHeaderValue"));
                    return -1;
                }
                if (!WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)lpOutBuffer, -1, pbHeaderValue, pdwHeaderValueLen, NULL, NULL)) {
                    ErrorExit(TEXT("2nd WideCharToMultiByte failure\n"));
                    return -1;
                }
            }
            printf("\nHeader contents: \n%s", lpOutBuffer);
        }                                                                               // no apparent need for returning the name of the cookie
        free(lpOutBuffer);
    }

    if (plptr->u1.prev)
        return 1;
    else
        return 0;
}

int pttw_extract_body_data(struct parameters* plptr, PSTR readbuffer, PSTR left, PSTR right, PSTR ordinal) {            // inspect returned data for registered parameter name
    PSTR lptr = readbuffer, lptr2 = NULL, lptr3 = NULL, lptr4 = NULL;                                                                 // save found data into parms structure
    DWORD buflen = 0, foundLdelim = 0, foundRdelim = 0, i, leftLength = 0, len, match = 0, ord = 0, parmLen = 0, parameterOccurrence = 0, rightLength = 0, saveAll = 0;

    if (left)
        leftLength = (int)strlen(left);
    else
        leftLength = 0;
    if (right)
        rightLength = (int)strlen(right);
    else
        rightLength = 0;
    buflen = (int)(strlen(readbuffer));
    if (!(_stricmp(ordinal, "all"))) {
        saveAll = 1;
        if (!(lptr4 = malloc(BUFFSIZE))) {
            ErrorExit(TEXT("Failure allocating memory for sprintf buffer in pttw_extract_body_data"));
            return -1;
        }
    }
    else {
        ord=atoi(ordinal);
    }

FindAll:
    do {
        while (*lptr != *left) {                    // locate first character of left boundary
            if (*(lptr + 1)) {                      // check for more array members (not the end of the buffer)
                ++lptr;
            }
            if (!(*(lptr + 1))) {                   // reaching the end of the buffer,
                return 1;                              // no match as been found, prepare to leave this function
            }
        }                                           // if the first character of the boundary matches up with current location in buffer
        for (match = i = 0; i < leftLength; i++) {  // see if the rest of the boundary string matches up with subsequent buffer bytes
            if (*(lptr + i) != *(left + i)) {       // if no match at current location, quit looking here and move on
                break;
            }
        }
        if (i == leftLength) {                      // however, if the above for loop went to fruition,
            match = 1;                              // set a flag
            lptr = lptr + i;                        // move pointer to end of left boundary
            break;                                  // and exit inner while loop
        }
        ++lptr;                                     // no match - continue moving right through buffer
    } while (1);                                    // keep going through buffer until manually exited from inner loop only if the for loop went to completion
    foundLdelim = 1;                              // if we got here, set a marker in the sand
    lptr2 = lptr;                                 // establish that marker because if the right boundary is found, substring starting here needs to be extracted
    if (match&&right) {
        do {
            while (*lptr != *right) {                       // see if right boundary exists
                if (*(lptr + 1))                            // check for more array members
                    ++lptr;
                if (!(*(lptr + 1))) {                       // reaching the end of the buffer,
                    return 1;                               // no match as been found, leave this function
                }
            }
            lptr3 = lptr;
            for (match = i = 0; i < rightLength; i++) {     // see if the rest of the boundary string matches up with subsequent buffer bytes
                if (*(lptr + i) != *(right + i)) {           // if no match at current location, quit looking here and move on
                    break;
                }
            }
            if (i == rightLength) {                      // however, if the above for loop went to fruition,
                lptr += rightLength;
                match = 1;                              // set a flag
                break;                                  // and exit inner while loop
            }
            ++lptr;                                     // no match - continue moving right through buffer
        } while (1);
        foundRdelim = 1;
    }
    if (match) {
        ++parameterOccurrence;
        if (!right) {
            len = (int)((readbuffer + buflen) - lptr2);
            if (!(plptr->value = malloc(len + 1))) {          // create some space for the data
                ErrorExit(TEXT("Failure allocating memory for parmValue in pttw_extract_body_data"));
                return -1;
            }
        }
        else
            if (!(plptr->value = malloc((len = (DWORD)(lptr3 - lptr2)) + 1))) {          // create some space for the data
                ErrorExit(TEXT("Failure allocating memory for parmValue in pttw_extract_body_data"));
                return -1;
            }
        memset(plptr->value, 0, len + 1);
        memcpy(plptr->value, lptr2, len);                                           // gather the data
        if (saveAll) {
            sprintf_s(lptr4, BUFFSIZE, "%d", parameterOccurrence);                  // create the array name -> <parm>_x
            pttw_store_parms(plptr->name, plptr->value, lptr4);
            goto FindAll;
        }
        else {
            if ((ord)&&(parameterOccurrence != ord))                               // if a specific occurrence was requested and this ain't it
                goto FindAll;                                                      // continue searching
        }
    } else
        return 1;

    if (!saveAll)
        pttw_store_parms(plptr->name, plptr->value, NOORDINAL);

    return 0;
}

struct parameters* pttw_get_parameters(void) {          // return a pointer to the last defined parameter search info
    struct parameters* plptr;                           // NOTE* all of these for loops to get to end of linked lists can be done away with by adding
    int i;                                              // a pointer to the last element in the list somewhere
    for (i = 0, plptr = pttw_parameters_head.u2.head; i < pttw_parameters_head.u1.parmCount; i++) {
        if (plptr->u2.next)
            plptr = plptr->u2.next;                   // move on to next one
    }
    if (!plptr)                                         // this should never happen
        return (void*)0;                                // but trap it if it does

//    if (plptr->name)
    return plptr;
}

void pttw_process_results(HINTERNET hRequest, PTTW_STRING scheme, LPVOID readBuffer) {
    struct parameters* plptr = NULL;
    DWORD dwProcessAgain = 0;

    if (!(plptr = pttw_get_parameters()))
        return;

    if (!_stricmp(plptr->location, "headers")) {                        // go pull the cookie info out of the returned headers
        if (pttw_extract_header_data(hRequest, scheme, plptr))
            dwProcessAgain = 1;
    }
    else {
        if (!readBuffer)
            return;
        pttw_extract_body_data(plptr, readBuffer, plptr->left, plptr->right, plptr->ordinal);
    //    if (!(pttw_extract_body_data(plptr, readBuffer, plptr->left, plptr->right, plptr->ordinal)))
    //        dwProcessAgain = 1;
    }

    //if (dwProcessAgain) {
    //    if (pttw_drop_last_parameters(plptr))
    //        pttw_process_results(hRequest, scheme, readBuffer);
    //}
    //else
    //    pttw_drop_last_parameters(plptr);
    pttw_drop_last_parameters(plptr);
    pttw_process_results(hRequest, scheme, readBuffer);
    
    return;
}

int pttw_register_parameter_search(PTTW_PARM name, PTTW_STRING left, PTTW_STRING right, PTTW_STRING ordinal, PTTW_STRING location) {
    static struct parameters* plHead, * plptr;
    int buflen,i;
    PSTR lptr;

    lptr = malloc(BUFFSIZE);

    if (pttw_parameters_head.u1.parmCount) {
        for (i = 0, plptr = pttw_parameters_head.u2.head; i < pttw_parameters_head.u1.parmCount; i++) {     // can't think of a reason to have duplicate parameters
//            if (!strcmp(name, llptr->name))                            // so if there's already one with the same name, abort. TestID != TESTID
//               return -1;
            if (plptr->u2.next)                                         // check for existence of next one
                plptr = plptr->u2.next;                                 // if so, go there
        }
        plptr->u2.next = malloc(sizeof(struct parameters));         	// generate the next slot and put the new address into current's next pointer
        if (!plptr->u2.next)							                // make sure there wasn't a memory allocation failure
            return -1;
        memset(plptr->u2.next, 0, sizeof(struct parameters));           // zero out the new address space
        plptr->u2.next->u1.prev = plptr;          				        // establish linkage to current slot
        plptr = plptr->u2.next;				            		        // make the new slot the current object
        pttw_parameters_head.u1.parmCount++;
    }
    else {
        plHead = plptr = malloc(sizeof(struct parameters));	            // create initial slot plus holding pointer to head
        if (!plptr)
            return -1;
        memset(plptr, 0, sizeof(struct parameters));   				    // zero out the memory space
        pttw_parameters_head.u1.parmCount = 1;                            // enumerate the number of searches
        pttw_parameters_head.u2.head = plHead;                          // .next field points to head of chain
    }
    plptr->name = malloc(buflen = ((int)strlen(name) + 1));             // record the parameter name
    if (!plptr->name)
        return -1;
    strcpy_s(plptr->name, buflen, name);
    plptr->left = malloc(buflen = ((int)strlen(left) + 1));             // record the left boundary
    if (!plptr->left)
        return -1;
    strcpy_s(plptr->left, buflen, left);
    if (right) {
        plptr->right = malloc(buflen = ((int)strlen(right) + 1));           // record the right boundary
        if (!plptr->right)
            return -1;
        strcpy_s(plptr->right, buflen, right);
    }
    buflen = sprintf_s(lptr, BUFFSIZE, "%s", ordinal);
    if (buflen < 0)
        return -1;
    if (pttw_check_for_parms(&lptr, &(DWORD)buflen) < 0) {
        ErrorExit(TEXT("parameter conversion failure in pttw_register_parameter_search"));
        return -1;
    }
    plptr->ordinal = malloc(++buflen);       // record how many to save
    if (!plptr->ordinal)
        return -1;
    strcpy_s(plptr->ordinal, buflen, lptr);
    free(lptr);
    plptr->location = malloc(buflen = ((int)strlen(location) + 1));     // record where to search
    if (!plptr->location)
        return -1;
    strcpy_s(plptr->location, buflen, location);
    
    return 0;
}

PSTR pttw_build_restPath(PSTR base, PSTR path, ...) {
    size_t buflen;
    PSTR lptr, restPath=NULL;
    va_list ap;
    va_start(ap, path);							                               // Initialize to last known good parameter

    buflen = strlen(base) + strlen(path) + 4;
    if (restPath = malloc(buflen))                                             // allocate enough space for KNOWN components as well as "://" and a null byte
        sprintf_s(restPath, buflen, "%s%s", base, path);
    else
        return (void*)(-1);

    do {
        lptr = va_arg(ap, char*);	                                		    // starting point was path
        if (!lptr)
            break;

        buflen += (int)strlen(lptr);
        if (restPath = realloc(restPath, buflen))                               // allocate enough space for components as well as "://" and a null byte
            strcat_s(restPath, buflen, lptr);
        else
            return (void*)(-1);
    } while (1);
    va_end(ap);
    if (pttw_check_for_parms(&restPath, &(DWORD)buflen) < 0) {
        ErrorExit(TEXT("parameter conversion failure in pttw_build_restPath"));
        return (void*)(-1);
    }
    return restPath;
}

PSTR pttw_paramarr_idx(PTTW_PARM a, PTTW_INT b) {
    return pttw_find_parm(a, &b);                   // passing in a non-zero value causes pfp to return the value for that particular array member
}

int pttw_paramarr_len(PTTW_STRING a) {
    DWORD parmCount = COUNTMEMBERS;

    pttw_find_parm(a, &parmCount);                  // passing in negative one causes pfp to count the number of array members

    return parmCount;
}

int pttw_save_int(PTTW_INT a, PTTW_STRING b) {
    char source[QUADWORD];
    sprintf_s(source, QUADWORD, "%ld", a);
    pttw_store_parms(b, source, NOORDINAL);
	return 0;
}

int pttw_register_save_parameter(PTTW_NAME a, PTTW_STRING b, PTTW_STRING c, PTTW_STRING d, PTTW_STRING e) {
    return pttw_register_parameter_search(a, b, c, d, e);
}

PSTR pttw_save_string(PTTW_STRING a, PTTW_PARM b) {
    return  pttw_store_parms(b,a,NOORDINAL);
}

PSTR pttw_toupper(PSTR source) {
    PSTR target;
    if (*source) {
        target = source;                    // save the starting location
    }
    else
        return (VOID *)NULL;
    while (*source) {
        if ((*source>96)&&(*source<123))
            *source -= 0x20;                    // change the character to uppercase by translating down to the uppercase region
        source++;
    }
    return target;
}

PSTR pttw_eval_string(PSTR a) {
    return pttw_find_parm(a,NOINDEX);               // passing in zero causes pfp to return the value of the first matching parm name
}

/* ErrorExit likely replaces this VUGen analogue */
pttw_error_message(PTTW_STRING format, PTTW_STRING source, ...) {
    return 0;
}

int pttw_get_wide_count(LPCCH source) {
    int cchWideChar;
       
    if (cchWideChar = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, source, -1, NULL, 0))
        return cchWideChar*2;                                                   // stupid function call doesn't return the true byte count
    else                                                                        // which seems absolutely ridiculous given the purpose of the function
        return 0;                                                               // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
}                                                                               // not that it'll happen, but the true width of a character could be 3 bytes rather than just 2

LPCWSTR pttw_convert_to_wide(LPCCH source,int *wideLen) {
    int cchWideChar;
    LPCWSTR wTarget;
    if (cchWideChar = (pttw_get_wide_count(source))) {
        wTarget = malloc(cchWideChar);
        *wideLen = cchWideChar;
        if (MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, source, -1, (LPWSTR)wTarget, cchWideChar))
            return wTarget;
        else
            return (LPCWSTR)0;
    }
    else
        return (LPCWSTR)0;
}

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
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

int pttw_extract_cookies(PSTR *pbCookieValue, struct parameters *plptr) {
    PSTR lptr = *pbCookieValue, lptr2 = NULL,lptr3 = NULL,lptr4 = NULL;
    DWORD len = 0;

    if (!_stricmp(plptr->name, "set-cookie")) {
        if (!(lptr3 = malloc((len = (int)(strlen(plptr->left))) + 1))) {                     // allocate space for name of token
            free((void*)(*pbCookieValue));                                              // allocated in pttw_extract_header_data
            ErrorExit(TEXT("Failure allocating memory for Set-Cookie data"));
            return -1;
        }
        lptr2 = lptr;                                                                   // save start of token name
        while (*lptr != 0x3d) {                                                         // locate end of name '='
            if (*(lptr + 1))
                ++lptr;
            if (!(*(lptr + 1))) {
                free((void*)(*pbCookieValue));                                          // allocated in pttw_extract_header_data
                ErrorExit(TEXT("Catastrophic failure processing LWSSO_COOKIE_KEY"));
                return -1;
            }
        }
        memset(lptr3, 0, len + 1);
        memcpy(lptr3, plptr->left, len);                                                // save the name for later

        ++lptr;                                                                         // move to start of token
        lptr2 = lptr;
        while (*lptr != 0x3b) {                                                         // locate end of token
              if (*(lptr))                                                              // make sure we're not stepping off into uncharted waters
                  ++lptr;
              if (!(*(lptr + 1))) {
                  free((void*)(*pbCookieValue));                                          // allocated in pttw_extract_header_data
                  ErrorExit(TEXT("No terminating character found for LWSSO_COOKIE_KEY"));
                  return -1;
              }
        }
    }
    else 
        if (!_stricmp(plptr->name, LOCATION)) {
            if (!(plptr->name = (PTTW_PARM)realloc(plptr->name,len = (int)(strlen(RUNID))) + 1)) {      // allocate space for RunID parm
                free((void*)(*pbCookieValue));                                                          // allocated in pttw_extract_header_data
                ErrorExit(TEXT("Failure allocating memory for Set-Cookie data"));
                return -1;
            }
            else {
                memset(plptr->name, 0, len + 1);
                memcpy(plptr->name, RUNID, len);                                                        // save the name for later
            }
            len = 0;
//        
            pttw_extract_body_data(plptr, lptr, plptr->left, plptr->right, plptr->ordinal);
            //do {                                                                            // locate end of location URL
            //    if (*lptr == 0x00)
            //        break;                 // backup to the carriage return character
            //    if (*lptr)                                                            // make sure we're not stepping off into uncharted waters
            //        ++lptr;
            //    //if (!(*(lptr + 1))) {                                                       // somehow got to end of buffer...no idea what happened...giving up
            //    //    free((void*)(*pbCookieValue));                                          // allocated in pttw_extract_header_data
            //    //    ErrorExit(TEXT("No terminating character found for location: cookie"));
            //    //    return -1;
            //    //}
            //} while (*lptr);
        }
    else 
        if (!_stricmp(plptr->name, STATUS)) {                                                           // look for the HTTP return code preceded by "Status"
            if (!(lptr3 = malloc((len = (int)(strlen(HTTPSTATUS))) + 1))) {                             // allocate space for POST status
                free((void*)(*pbCookieValue));                                                          // allocated in pttw_extract_header_data
                ErrorExit(TEXT("Failure allocating memory for Status data"));
                return -1;
            }
            else {
                memset(lptr3, 0, len + 1);                                                              // calling this parm "Status" conflicts with the input status
                memcpy(lptr3, HTTPSTATUS, len);                                                         // save a unique name for later
            }
            lptr2 = lptr;
            do {                                                                            // locate end of location URL
                if (*lptr == 0x0)
                    break;                                                                  // backup to the carriage return character
                if (*lptr)                                                                  // make sure we're not stepping off into uncharted waters
                    ++lptr;
            } while (*lptr);
        }
    if (len) {
        if (!(lptr4 = malloc((len = (DWORD)(lptr - lptr2)) + 1))) {
            free((void*)(*pbCookieValue));                                                  // allocated in pttw_extract_header_data
            ErrorExit(TEXT("Failure allocating memory for location URL"));
            return -1;
        }
        else {
            memset(lptr4, 0, len + 1);
            memcpy(lptr4, lptr2, len);
        }
    }
    else
        return 0;

//    if ((_stricmp(plptr->name, "set-cookie"))&&(_stricmp(plptr->ordinal,LWSSOIDX)))
    if (!(_stricmp(plptr->name, "set-cookie")) && !(_stricmp(plptr->ordinal, LWSSOIDX)))
        pttw_save_string(lptr4, targetKey);
    else
        pttw_save_string(lptr4, lptr3);                                             // save off either the Test Instance RunID or the status of 201 which proves the POST succeeded
    free(lptr3);
    free(lptr4);
    free ((void*)(*pbCookieValue));                                                 // allocated in pttw_extract_header_data

    return 1;
}