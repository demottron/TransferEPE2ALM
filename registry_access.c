#include <windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>
#include "globals.h"

// Function to get error message from system error code 
void PrintError(const char* msg, DWORD error) {
LPSTR buffer = NULL;
FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    error,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)&buffer,
    0,
    NULL
);
printf("%s: %s (Error: %ld)\n", msg, buffer, error);
LocalFree(buffer);
}
/*
// Function to lock registry key permissions 
DWORD LockRegistryKey(HKEY hKeyRoot, const char* subKey, const char* userSidString) {
HKEY hKey;
DWORD error;
PSECURITY_DESCRIPTOR pSD = NULL;
EXPLICIT_ACCESS_A ea[2] = { 0 };
PACL pNewDacl = NULL;
SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
PSID pUserSid = NULL, pUsersSid = NULL;
    PSTR lpAccountName;
    PSID pUserSid = NULL;
    char sidBuffer[256];
    DWORD sidSize = sizeof(sidBuffer);
    char domain[256];
    DWORD domainSize = sizeof(domain);
    SID_NAME_USE sidUse;

    // Open the registry key
    error = RegOpenKeyExA(hKeyRoot, subKey, 0, KEY_QUERY_VALUE, &hKey);
    if (error != ERROR_SUCCESS)
        ErrorExit(TEXT("Error opening registry key"));
    lpAccountName = pttw_eval_string("Owner");
    if (!LookupAccountNameA(NULL, lpAccountName, sidBuffer, &sidSize, domain, &domainSize, &sidUse)) {
        error = GetLastError();
        PrintError("Error getting user SID", error);
        RegCloseKey(hKey);
        return error;
    }
    
// Open or create the registry key
error = RegCreateKeyExA(hKeyRoot, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS, NULL, &hKey, NULL);
if (error != ERROR_SUCCESS) {
    PrintError("Error opening/creating key", error);
    return error;
}

// Convert user SID string to SID (or get current user's SID)
if (userSidString) {
    if (!ConvertStringSidToSidA(userSidString, &pUserSid)) {
        error = GetLastError();
        PrintError("Error converting user SID", error);
        RegCloseKey(hKey);
        return error;
    }
}
else {
    // Get current user's SID (simplified; in practice, use GetTokenInformation)
    char sidBuffer[256];
    DWORD sidSize = sizeof(sidBuffer);
    char domain[256];
    DWORD domainSize = sizeof(domain);
    SID_NAME_USE sidUse;
    if (!LookupAccountNameA(NULL, "Administrator", sidBuffer, &sidSize, domain, &domainSize, &sidUse)) {
        error = GetLastError();
        PrintError("Error getting user SID", error);
        RegCloseKey(hKey);
        return error;
    }
    pUserSid = (PSID)sidBuffer;
}

// Create SID for Users group
if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pUsersSid)) {
    error = GetLastError();
    PrintError("Error creating Users SID", error);
    if (pUserSid && userSidString) LocalFree(pUserSid);
    RegCloseKey(hKey);
    return error;
}

// Set up access entries
// 1. Grant Full Control to the specified user
ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
ea[0].grfAccessMode = SET_ACCESS;
ea[0].grfInheritance = NO_INHERITANCE;
ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
ea[0].Trustee.ptstrName = (LPSTR)pUserSid;

// 2. Deny write access to Users group
ea[1].grfAccessPermissions = KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WRITE;
ea[1].grfAccessMode = DENY_ACCESS;
ea[1].grfInheritance = NO_INHERITANCE;
ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
ea[1].Trustee.ptstrName = (LPSTR)pUsersSid;

// Create new DACL
error = SetEntriesInAclA(2, ea, NULL, &pNewDacl);
if (error != ERROR_SUCCESS) {
    PrintError("Error creating DACL", error);
    if (pUserSid && userSidString) LocalFree(pUserSid);
    if (pUsersSid) FreeSid(pUsersSid);
    RegCloseKey(hKey);
    return error;
}

// Initialize security descriptor
pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
if (!pSD) {
    error = GetLastError();
    PrintError("Error allocating security descriptor", error);
    LocalFree(pNewDacl);
    if (pUserSid && userSidString) LocalFree(pUserSid);
    if (pUsersSid) FreeSid(pUsersSid);
    RegCloseKey(hKey);
    return error;
}

if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
    error = GetLastError();
    PrintError("Error initializing security descriptor", error);
    LocalFree(pSD);
    LocalFree(pNewDacl);
    if (pUserSid && userSidString) LocalFree(pUserSid);
    if (pUsersSid) FreeSid(pUsersSid);
    RegCloseKey(hKey);
    return error;
}

// Set DACL in security descriptor
if (!SetSecurityDescriptorDacl(pSD, TRUE, pNewDacl, FALSE)) {
    error = GetLastError();
    PrintError("Error setting DACL in security descriptor", error);
    LocalFree(pSD);
    LocalFree(pNewDacl);
    if (pUserSid && userSidString) LocalFree(pUserSid);
    if (pUsersSid) FreeSid(pUsersSid);
    RegCloseKey(hKey);
    return error;
}

// Apply security descriptor to registry key
error = SetSecurityInfo(hKey, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL);
if (error != ERROR_SUCCESS) {
    PrintError("Error setting registry key security", error);
}
else {
    printf("Successfully locked registry key '%s'.\n", subKey);
    printf(" - User has Full Control.\n");
    printf(" - Users group denied write access.\n");
}

// Cleanup
LocalFree(pSD);
LocalFree(pNewDacl);
if (pUserSid && userSidString) LocalFree(pUserSid);
if (pUsersSid) FreeSid(pUsersSid);
RegCloseKey(hKey);
return error;
}

int main() {
    const char* subKey = "Software\\MyTestApp";
    const char* valueName = "TestSetting";
    const char* valueData = "Hello, Registry!";
    HKEY hKey;
    DWORD error;

    // Write test value to registry
    error = RegCreateKeyExA(HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKey, NULL);
    if (error == ERROR_SUCCESS) {
        error = RegSetValueExA(hKey, valueName, 0, REG_SZ,
            (const BYTE*)valueData, strlen(valueData) + 1);
        if (error == ERROR_SUCCESS) {
            printf("Successfully wrote '%s' to registry.\n", valueData);
        }
        else {
            PrintError("Error writing value", error);
        }
        RegCloseKey(hKey);
    }
    else {
        PrintError("Error creating key", error);
    }

    // Lock the registry key
    // Pass NULL for userSidString to use current user (simplified)
    // Replace with specific SID string (e.g., "S-1-5-21-...") for a specific user
    error = LockRegistryKey(HKEY_CURRENT_USER, subKey, NULL);
    if (error == ERROR_SUCCESS) {
        printf("Registry key permissions set successfully.\n");
    }

    return 0;
}
*/

// Function to write a string value to the registry - not currently used as of 06/10/2025 - only here for possible future expansion
DWORD WriteRegistryString(HKEY hKeyRoot, const char* subKey, const char* valueName, const char* valueData) {
HKEY hKey;
DWORD dwDisposition;
DWORD error;

// Open or create the registry key
error = RegCreateKeyExA(hKeyRoot, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition);
if (error != ERROR_SUCCESS) {
    printf("Error opening/creating key: %ld\n", error);
    return error;
}

// Write the string value
error = RegSetValueExA(hKey, valueName, 0, REG_SZ, (const BYTE*)valueData, (DWORD)strlen(valueData) + 1);
if (error != ERROR_SUCCESS) {
    printf("Error writing value: %ld\n", error);
}

RegCloseKey(hKey);
return error;
}

// Function to read a string value from the registry - not especially generic...intended to retrieve EPE and ALM server names
void ReadRegistryString(HKEY hKeyRoot, const char* subKey, const char* valueName, PTTW_STRING** svrName) {
    HKEY hKey;
    char buffer[BUFFSIZE];
    DWORD dwType;
    DWORD dwSize = BUFFSIZE;
    DWORD error;

    // Open the registry key
    error = RegOpenKeyExA(hKeyRoot, subKey, 0, KEY_QUERY_VALUE, &hKey);
    if (error != ERROR_SUCCESS)
        ErrorExit(TEXT("Error opening registry key"));

    // Read the string value
    error = RegQueryValueExA(hKey, valueName, NULL, &dwType, (LPBYTE)buffer, &dwSize);
    if (error != ERROR_SUCCESS)                                                         // validate the contents of the registry value
        ErrorExit(TEXT("Error reading registry value"));
    else if (dwType != REG_SZ)
        ErrorExit(TEXT("Value is not a string\n"));                                     // don't return to initialize_program without good data

    RegCloseKey(hKey);

    if (dwSize >= BUFFSIZE)                                                               // one last check to make sure local buffer didn't just get blown
        ErrorExit(TEXT("Buffer space overflow in reading registry"));

    if (!(*svrName = malloc((size_t)(dwSize + 1))))                                       // allocate enough space for registry data
        ErrorExit(TEXT("Failure allocating space for server name"));
    else {
        memset(*svrName, 0, dwSize + 1);
        memcpy(*svrName, buffer, dwSize);                                               // copy registry data to the newly allocated server name buffer
    }
}