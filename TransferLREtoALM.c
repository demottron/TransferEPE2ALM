// TransferLREtoALM.c : This file contains the 'main' function. Program execution begins and ends here.
//
/*
    program:		TransferLREtoALM
    version:		1.1.9
    savetimestamp:	1750968219
	purpose:		pulls test run data out of LRE using REST API calls and inserts into ALM's corresponding fields creating a Test Instance recordset
    history:        version 1.0.0 was initial which didn't write to production.
                    version 1.1.0 program was authorized into production ALM project database
                    version 1.1.1 bug fix
                    version 1.1.2 bug fix
                    version 1.1.3 bug fix
                    version 1.1.4 workaround for DBMS issue
                    version 1.1.5 DEFAULT domain no longer contains a project and so /domains call only receives main domain
                    version 1.1.6 ALM server updated to v17.0.1. New VM was created to house it. Much of the ALM documentation surrounding tokens is misleading or wrong
                    version 1.1.7 moved EPE and ALM server names to registry HKEY_CURRENT_USER\Software\TransferLREtoALM (LR recently updated to EPE 25.1 - which is irrelevant)
                    version 1.1.8 minor bug stemming from project name validation logic where foundProject flag wasn't reset after TestID folder name search
                    version 1.1.9 update reference to LoadRunner server as EPE
    update:         1.1.9 (1750968219) forgot to change visible references of version and the LR server (EPE)
                    1.1.8 (1750878125) set foundProject back to 0 in Get_ALM_Info.c(137)
                    1.1.7 (1749565121) Added registry_access.c which reads from HKEY_CURRENT_USER\Software\TransferLREtoALM in order to retrieve EPE and ALM server names
                    1.1.6 (1726863349) New server name is oitispopstsalm1.va.gov. Due to incorrect/misleading documentation a vastly greater amount of work was required
                            fix:    LWSSO token has to be captured and sent back in. QCSession doesn't change. XSRF checking is disabled.
                    1.1.5 (1721224017) There is now only the one active domain, so /domains only returns one piece of data instead of an array
                            fix:    changed LREDEPT_OF_VET_AFFAIRS #define to '1' in case of changes in the future. This instead calling with NOORDINAL.
                    1.1.4 (1718661325) <project>/Runs call taking excessive amount of time to complete. 
                            fix:    Added pttw_increase_timeouts() to add time to wait for call to complete.
                    1.1.3 (1716343200) reason: run_logic did not check for presence of requested LRE RunID. Loop would insert non-existent record into ALM using previous record's data                resulting in bad timestamps.
                            fix:    added check for valid RunID in LRE.
                    1.1.2 (1709148349) reason: script name concatenation code was not sending in the correct buffer size for lptr5. strcat_s failed on safety check.
                            fix:    added the offset value to existing buflen parameter
                    1.1.1 (1709065191) reason: certain schedular configuration results in multiple <Vusers></Vusers> items which did not align with <ID></ID> items resulting in mismatched parameter pairs
                            fix:    added check for null pointer from call to retrieve <grpId> parameter's value

    Program is executed from a batch file (tla.bat)
    :: Prompt for Project Name  - the LRE project folder name which must match the ALM project name
    :: Prompt for LRE RunID     - value of the scenario execution number. Input as described below
    :: Prompt for USERNAME      - the Active Directory ID under which the Test Instance will be created in ALM
    :: Prompt for TestID        - Test ID for the Test found in the Test Plan tab in ALM
    :: Prompt for TestSetID     - Test Set ID for the test set found in the Test Lab tab in ALM
    :: Prompt for TestType      - BASELINE, ENDURANCE, LOAD, or SMOKE.
    :: Prompt for Status        - COMPLETED, COMPLETED WITH FINDINGS, BLOCKED, FAILED, NA, NO RUN, NOT COMPLETED, or PASSED

    ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! 
    ATTENTION!                                                                                                                                                ATTENTION! 
    ATTENTION!  There is a data integrity issue within LRE's DBMS stemming from migration from previous versions of LRE. The result is some older data is     ATTENTION! 
    ATTENTION!  not present and therefore not able to be transferred by this program. Will likely cause program abend.                                        ATTENTION! 
    ATTENTION!                                                                                                                                                ATTENTION! 
    ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! ATTENTION! 

	Program is capable of handling a single LRE RunID, a range of RunIDs, or a selection of RunIDs. Range is two numbers separated by a hyphen, while multiple RunIDs are to be separated by a comma.
	Both of these modalities are handled in their respective case statement found in RunLogic either through use of a FOR loop for a RANGE or a do-while loop to parse out the RunIDs and execute the program logic.
	Otherwise, a single RunID will run through the program logic one time.

    Main data structures are linked lists of body, parms and parameters. Body is to manage the data sent in to REST API as needed (not implemented). Parms are everything to be saved (LoadRunner placeholders) and are created in pttw_store_parms(). Ordinal=ALL is supported by creating parms names appended with "underscore number" thereby replicating the array functionality of LoadRunner. Parameters are a linked list of elements created in pttw_register_parameter_search() in order to correlate strings of data recovered out of data returned from server the names and values of which are then saved off into the parms list. Post processing of server data is done in reverse parameter order (last entry first) such that the data is saved off as a parms element thence the parameter element is destroyed. Both functions declare their respective structures as static so that the lists remain valid across function calls.

    "body": (not implemented) 

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
            PTTW_STRING value;		    // string value
            struct body* tail;          // point to first allocated structure in the standalone instance of pttw_parms_head
        } u3;
    } pttw_body_head;

    "parms":

    struct parms {
        union {
            int parmCount;          // keep track of how many structures get generated and stored in the standalone instance of pttw_parms_head
            struct parms* prev;		// points to previous member. Null for head of chain
        } u1;
        union {                     // as of 01/11/2024, this union is notional, but seems like a great idea as it will do away with some of the looping
            PTTW_PARM name; 		// parm name
            unsigned char array;    // array indicator
            struct parms* head;     // point to first allocated structure in the standalone instance of pttw_parms_head
        } u2;
        union {
            struct parms* next;		// points to next member. Null for end of chain.
            struct parms* tail;     // pointer to tail end of linked list and stored in the standalone instance of pttw_parms_head
        } u3;                       // implementing this reimagining of the structure will require a great amount of precision and attention to detail
        PTTW_STRING value;		    // string value
    } pttw_parms_head;

    "parameters":

    struct parameters {
        union {
            int parmCount;          // keep track of how many structures get registered for a given rest call. Only valid in the pttw_parameters_head structure instance.
            struct parameters* prev;// points to previous member. Null for head of chain
        } u1;
        union {
            PTTW_PARM name; 		// parameter name
            struct parameters* head;// point to first allocated structure. Only valid in the pttw_parameters_head structure instance.
        } u2;
        union {                     // as of 01/11/2024, this union is notional, but seems like a great idea as it will do away with some of the looping
            struct parameters* next;// points to next member. Null for end of chain.
            struct parameters* tail;// pointer to tail end of linked list and only valid in the pttw_parameters_head structure instance.
        } u3;
        PTTW_STRING left;			// left boundary value
        PTTW_STRING right;			// right boundary value
        PTTW_STRING ordinal;		// how many parms to save off when processing server data
        PTTW_STRING location;		// where to look (optional) ("headers" or "body")
        PTTW_STRING value;			// content
    } pttw_parameters_head;

*/
#include <stdio.h>
#include "globals.h"

int main(int argc,char *argv[])
{
    printf("\n\n\t%s %s",ProgramName,ProgramVersion);
    printf("\n\tTim Nichols");
    printf("\n\tCopyright 2024-2025\n");

    if (initialize_Program(argc, argv) < 0) {
        printf("Failure in initialize_Program");
            return -1;
    }
	RunLogic();
	end_Program();
}