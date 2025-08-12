// TransferEPE2ALM.c : Defines the entry point for the application.
//

#include <windows.h>
#include <strsafe.h>
#include <winhttp.h>
#include "resource.h"
#include "framework.h"
#include "TransferEPE2ALM.h"

// Add pragma to link with WinHttp library
#pragma comment(lib, "winhttp.lib")

#define MAX_LOADSTRING 100

// Global Variables:
static HINSTANCE hInst;                                // current instance
static WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
static WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations
static ATOM                MyRegisterClass(HINSTANCE hInstance);
static BOOL                InitInstance(HINSTANCE, int);
static LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    LoginDialogProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    DomainProjectDialogProc(HWND, UINT, WPARAM, LPARAM);
static BOOL                ShowLoginDialog(HWND hwndParent);
static BOOL                ShowDomainProjectDialog(HWND hwndParent);

// Domain/Project dialog handling
static INT_PTR CALLBACK DomainProjectDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hDomainCombo, hProjectCombo;
    
    switch (message)
    {
    case WM_INITDIALOG:
        // Center the dialog
        {
            RECT rc;
            int xPos, yPos;
            GetWindowRect(hDlg, &rc);
            xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
            yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
            SetWindowPos(hDlg, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        
        // Get handles to combo boxes
        hDomainCombo = GetDlgItem(hDlg, IDC_DOMAIN_COMBO);
        hProjectCombo = GetDlgItem(hDlg, IDC_PROJECT_COMBO);
        
        if (hDomainCombo && hProjectCombo)
        {
            // Populate domain combo box
            SendMessage(hDomainCombo, CB_ADDSTRING, 0, (LPARAM)L"DEPT_OF_VET_AFFAIRS");
            SendMessage(hDomainCombo, CB_SETCURSEL, 0, 0);
            
            // Populate project combo box
            const WCHAR* projects[] = {
                L"AMPL_GUI", L"VAPe", L"VIA", L"VIC", L"VIRP",
                L"Vista_Utility", L"VLM", L"VPL", L"VSA_2016"
            };
            
            for(int i = 0; i < sizeof(projects)/sizeof(projects[0]); i++)
            {
                SendMessage(hProjectCombo, CB_ADDSTRING, 0, (LPARAM)projects[i]);
            }
            SendMessage(hProjectCombo, CB_SETCURSEL, 0, 0);
        }
        
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static BOOL ShowDomainProjectDialog(HWND hwndParent)
{
    INT_PTR result = DialogBox(hInst, MAKEINTRESOURCE(IDD_DOMAIN_PROJECT), hwndParent, DomainProjectDialogProc);
    
    if (result == -1)
    {
        DWORD error = GetLastError();
        WCHAR errorMsg[256];
        _snwprintf_s(errorMsg, _countof(errorMsg), _TRUNCATE, 
            L"Failed to create domain/project dialog. Error code: %lu", error);
        MessageBoxW(NULL, errorMsg, L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }
    
    return result == IDOK;
}

// Modified LoginDialogProc with credential check
static INT_PTR CALLBACK LoginDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    int xPos, yPos;
    WCHAR username[256];
    WCHAR password[256];
    
    UNREFERENCED_PARAMETER(lParam);
    
    switch (message)
    {
    case WM_INITDIALOG:
        // Center the dialog
        GetWindowRect(hDlg, &rc);
        xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
        yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
        SetWindowPos(hDlg, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemText(hDlg, IDC_USERNAME, username, 256);
            GetDlgItemText(hDlg, IDC_PASSWORD, password, 256);
            
            // Check credentials
            if (wcscmp(username, L"admin") == 0 && wcscmp(password, L"password123") == 0)
            {
                // Valid credentials, close login dialog and show domain/project dialog
                EndDialog(hDlg, IDOK);
                return ShowDomainProjectDialog(NULL) ? (INT_PTR)TRUE : (INT_PTR)FALSE;
            }
            else
            {
                // Invalid credentials, show error message
                MessageBox(hDlg, L"Invalid username or password.\nUsername should be 'admin'\nPassword should be 'password123'", 
                          L"Authentication Failed", MB_OK | MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static BOOL ShowLoginDialog(HWND hwndParent)
{
    DWORD error;
    WCHAR errorMsg[256];
    INT_PTR result;

    // Make sure hInst is set
    if (!hInst)
    {
        MessageBoxW(NULL, L"Application instance not initialized", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Show the login dialog modally
    result = DialogBoxW(hInst, MAKEINTRESOURCE(IDD_LOGIN), hwndParent, LoginDialogProc);
    
    // Check for dialog creation errors
    if (result == -1)
    {
        error = GetLastError();
        _snwprintf_s(errorMsg, _countof(errorMsg), _TRUNCATE, L"Failed to create login dialog. Error code: %lu", error);
        MessageBoxW(NULL, errorMsg, L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return result == IDOK;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TRANSFEREPE2ALM, szWindowClass, MAX_LOADSTRING);
    
    if (!MyRegisterClass(hInstance))
    {
        return FALSE;
    }

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TRANSFEREPE2ALM));
    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

static ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    ZeroMemory(&wcex, sizeof(wcex));

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRANSFEREPE2ALM));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TRANSFEREPE2ALM);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;  // Store instance handle in our global variable

    // Show login dialog first and exit if canceled
    if (!ShowLoginDialog(NULL))
    {
        return FALSE;  // Exit application if login is canceled
    }

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}