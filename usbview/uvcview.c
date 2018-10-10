/*++

Copyright (c) 1997-2011 Microsoft Corporation

Module Name:

USBVIEW.C

Abstract:

This is the GUI goop for the USBVIEW application.

Environment:

user mode

Revision History:

04-25-97 : created
11-20-02 : minor changes to support more reporting options
04/13/2005 : major bug fixing
07/01/2008 : add UVC 1.1 support and move to Dev branch

--*/

/*****************************************************************************
I N C L U D E S
*****************************************************************************/

#include "resource.h"
#include "uvcview.h"
#include "h264.h"
#include "xmlhelper.h"
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <commdlg.h>




/*****************************************************************************
D E F I N E S
*****************************************************************************/

// window control defines
//
#define SIZEBAR             0
#define WINDOWSCALEFACTOR   15
#define LISTNAMESIZE		256

/*****************************************************************************
 L O C A L  T Y P E D E F S
*****************************************************************************/
typedef struct _TREEITEMINFO
{
    struct _TREEITEMINFO *Next;
    USHORT Depth;
    PCHAR Name;

} TREEITEMINFO, *PTREEITEMINFO;

//! contains usb specific data 
/*! Contains most importantly contains usb serial number, drive letter and weather this drive is writable.
Device path is not true device path and should not be used to write files. deviceDetailData can be found 
on microsoft documentation website. 
*/
typedef struct ty_TUSB_Device
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceDetailData;
	char                                devicePath[300];
	char								serialnum[300];
	char								letter[2];
	int									is_writeable;

}TUSB_Device;

typedef struct _letter_num {
	int								num;
	int								is_writeable;
	char							letter[2];
	

}Letter_num;



/*****************************************************************************
L O C A L    E N U M S
*****************************************************************************/

typedef enum _USBVIEW_SAVE_FILE_TYPE
{
    UsbViewNone = 0,
    UsbViewXmlFile,
    UsbViewTxtFile
} USBVIEW_SAVE_FILE_TYPE;

/*****************************************************************************
L O C A L    F U N C T I O N    P R O T O T Y P E S
*****************************************************************************/

int WINAPI
WinMain (
         _In_ HINSTANCE hInstance,
         _In_opt_ HINSTANCE hPrevInstance,
         _In_ LPSTR lpszCmdLine,
         _In_ int nCmdShow
         );

BOOL
CreateMainWindow (
                  int nCmdShow
                  );

VOID CollectUsbName(PVOID info, char* usb_name, char* serial_num, int max_str_len);

VOID
ResizeWindows (
               BOOL    bSizeBar,
               int     BarLocation
               );

LRESULT CALLBACK
MainDlgProc (
             HWND   hwnd,
             UINT   uMsg,
             WPARAM wParam,
             LPARAM lParam
             );

BOOL
USBView_OnInitDialog (
                      HWND    hWnd,
                      HWND    hWndFocus,
                      LPARAM  lParam
                      );

VOID
USBView_OnClose (
                 HWND hWnd
                 );

VOID
USBView_OnCommand (
                   HWND hWnd,
                   WPARAM  id,
                   HWND hwndCtl,
                   UINT codeNotify
                   );

VOID
USBView_OnLButtonDown (
                       HWND hWnd,
                       BOOL fDoubleClick,
                       int  x,
                       int  y,
                       UINT keyFlags
                       );

VOID
USBView_OnLButtonUp (
                     HWND hWnd,
                     int  x,
                     int  y,
                     UINT keyFlags
                     );

VOID
USBView_OnMouseMove (
                     HWND hWnd,
                     int  x,
                     int  y,
                     UINT keyFlags
                     );

VOID
USBView_OnSize (
                HWND hWnd,
                UINT state,
                int  cx,
                int  cy
                );

LRESULT
USBView_OnNotify (
                  HWND    hWnd,
                  int     DlgItem,
                  LPNMHDR lpNMHdr
                  );

BOOL
USBView_OnDeviceChange (
                        HWND  hwnd,
                        UINT  uEvent,
                        DWORD dwEventData
                        );

//void USBView_SetCursor(
//	HWND hwnd
//
VOID DestroyTree (VOID);

VOID DestroyList(VOID);

VOID RefreshTree (VOID);

VOID RefreshList(VOID);

LRESULT CALLBACK
AboutDlgProc (
              HWND   hwnd,
              UINT   uMsg,
              WPARAM wParam,
              LPARAM lParam
              );

VOID
WalkTree (
          _In_ HTREEITEM        hTreeItem,
          _In_ LPFNTREECALLBACK lpfnTreeCallback,
          _In_opt_ PVOID            pContext
          );

VOID
ExpandItem (
            HWND      hTreeWnd,
            HTREEITEM hTreeItem,
            PVOID     pContext
            );

VOID
AddItemInformationToFile(
            HWND hTreeWnd,
            HTREEITEM hTreeItem,
            PVOID pContext,
			char* additional_text
        );

DWORD
DisplayLastError(
          _Inout_updates_bytes_(count) char    *szString,
          int     count);

VOID AddItemInformationToXmlView(
    HWND hTreeWnd,
    HTREEITEM hTreeItem,
    PVOID pContext
    );
HRESULT InitializeConsole();
VOID UnInitializeConsole();
BOOL IsStdOutFile();
VOID DisplayMessage(DWORD dwMsgId, ...);
VOID PrintString(LPTSTR lpszString);
LPTSTR WStringToAnsiString(LPWSTR lpwszString);
VOID WaitForKeyPress();
BOOL ProcessCommandLine();
HRESULT ProcessCommandSaveFile(LPTSTR szFileName, DWORD dwCreationDisposition, USBVIEW_SAVE_FILE_TYPE fileType);
HRESULT SaveAllInformationAsText(LPTSTR lpstrTextFileName, DWORD dwCreationDisposition);
HRESULT SaveAllInformationAsXml(LPTSTR lpstrTextFileName , DWORD dwCreationDisposition);
char * CreateAdditionalText(char* listname);
char* GetUSBDrive_filepath(char* name);
int GetUSBDevices(TUSB_Device *devList[], int size);
VOID CollectUsbName(PVOID info, char* usb_name, char* serial_num, int max_str_len);
VOID CreateListItems(HWND hTreeWnd, HTREEITEM hTreeItem, PVOID pContext);
/*****************************************************************************
G L O B A L S
*****************************************************************************/
BOOL gDoConfigDesc = TRUE;
BOOL gDoAnnotation = TRUE;
BOOL gLogDebug     = FALSE;
int  TotalHubs     = 0;

extern DEVICE_GUID_LIST gHubList;
extern DEVICE_GUID_LIST gDeviceList;

/*****************************************************************************
G L O B A L S    P R I V A T E    T O    T H I S    F I L E
*****************************************************************************/

HINSTANCE       ghInstance       = NULL; 
HWND            ghMainWnd        = NULL;
HWND            ghTreeWnd        = NULL;
HWND            ghEditWnd        = NULL;
HWND            ghStatusWnd      = NULL;
HMENU           ghMainMenu       = NULL;
HTREEITEM       ghTreeRoot       = NULL;
HCURSOR         ghSplitCursor    = NULL;
HDEVNOTIFY      gNotifyDevHandle = NULL;
HDEVNOTIFY      gNotifyHubHandle = NULL;
HANDLE          ghStdOut         = NULL;

BOOL            gbConsoleFile  = FALSE;
BOOL            gbConsoleInitialized = FALSE;
BOOL            gbButtonDown     = FALSE;
BOOL            gDoAutoRefresh   = TRUE;


HWND			ghListWnd		 = NULL;
int				numUsbConnected  = 0;		//! is used for the status window
HINSTANCE       ghInstanceL		 = NULL;
char*			SelectedUsbName  = NULL;	//! is a pointer to the currently selected usb name. is NULL if none is selected.
TUSB_Device*	myList[26];					//! contains info of each usb contained in TUSB_Device Structure
int				DeviceCount = 0;			//! number of devices contained in myList
HWND			ghSaveWnd		 = NULL;
HWND			ghButton		 = NULL;
HWND			ghButton2		 = NULL;

int				is_clicked = 0;
int				move_area = 10;

int             gBarLocation     = 0;
int             giGoodDevice     = 0;
int             giBadDevice      = 0;
int             giComputer       = 0;
int             giHub            = 0;
int             giNoDevice       = 0;
int             giGoodSsDevice   = 0;
int             giNoSsDevice     = 0;
HCURSOR			standard_cursor = NULL;

/*****************************************************************************

WinMain()

*****************************************************************************/

//! The entry point into this application
/*! Does normal windows desktop initilization. 
Notably to this specific application WinMain calls CreateTextBuffer to 
initilize editwnd's text buffer
*/

int WINAPI
WinMain (
         _In_ HINSTANCE hInstance,
         _In_opt_ HINSTANCE hPrevInstance,
         _In_ LPSTR lpszCmdLine,
         _In_ int nCmdShow
         )
{

	
    MSG     msg;
    HACCEL  hAccel;
    int retStatus = 0;
	
	//AllocConsole();
	//AttachConsole(GetCurrentProcessId());
	//freopen("CON", "w", stdout);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdLine);

    InitXmlHelper();

    ghInstance = hInstance;

    ghSplitCursor = LoadCursor(ghInstance,
        MAKEINTRESOURCE(IDC_SPLIT));

    if (!ghSplitCursor)
    {
        OOPS();
        return retStatus;
    }

    hAccel = LoadAccelerators(ghInstance,
        MAKEINTRESOURCE(IDACCEL));

    if (!hAccel)
    {
        OOPS();
        return retStatus;
    }

    if (!CreateTextBuffer())
    {
        return retStatus;
    }
	
	
	if (!ProcessCommandLine())
    {
        // There were no command line flags, open GUI
        if (CreateMainWindow(nCmdShow))
        {
			

            while (GetMessage(&msg, NULL, 0, 0))
            {
				
                if (!TranslateAccelerator(ghMainWnd,
                            hAccel,
                            &msg) &&
                        !IsDialogMessage(ghMainWnd,
                            &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            retStatus = 1;
        }
    }

    DestroyTextBuffer();

    ReleaseXmlWriter();

    CHECKFORLEAKS();

    return retStatus;
}


/*****************************************************************************

ProcessCommandLine()

Parses the command line and takes appropriate actions. Returns FALSE If there is no action to
perform
*****************************************************************************/
BOOL ProcessCommandLine()
{
    LPWSTR *szArgList = NULL;
    LPTSTR szArg = NULL;
    LPTSTR szAnsiArg= NULL;
    BOOL quietMode = FALSE;

    HRESULT hr = S_OK;
    DWORD dwCreationDisposition = CREATE_NEW;
    USBVIEW_SAVE_FILE_TYPE fileType = UsbViewNone;

    int nArgs = 0;
    int i = 0;
    BOOL bStatus = FALSE;
    BOOL bStopArgProcessing = FALSE;

    szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    // If there are no arguments we return false
    bStatus = (nArgs > 1)? TRUE:FALSE;

    if (NULL != szArgList)
    {
        if (nArgs > 1)
        {
            // If there are arguments, initialize console for ouput
            InitializeConsole();
        }

        for (i = 1; (i < nArgs) && (bStopArgProcessing == FALSE); i++)
        {
            // Convert argument to ANSI string for futher processing

            szAnsiArg = WStringToAnsiString(szArgList[i]);

            if(NULL == szAnsiArg)
            {
                DisplayMessage(IDS_USBVIEW_INVALIDARG, szAnsiArg);
                DisplayMessage(IDS_USBVIEW_USAGE);
                break;
            }

            if (0 == _stricmp(szAnsiArg, "/?"))
            {
                DisplayMessage(IDS_USBVIEW_USAGE);
                break;
            }
            else if (NULL != StrStrI(szAnsiArg, "/saveall:"))
            {
                fileType = UsbViewTxtFile;
            }
            else if (NULL != StrStrI(szAnsiArg, "/savexml:"))
            {
                fileType = UsbViewXmlFile;
            }
            else if (0 == _stricmp(szAnsiArg, "/f"))
            {
                dwCreationDisposition = CREATE_ALWAYS;
            }
            else if (0 == _stricmp(szAnsiArg, "/q"))
            {
                quietMode = TRUE;
            }
            else
            {
                DisplayMessage(IDS_USBVIEW_INVALIDARG, szAnsiArg);
                DisplayMessage(IDS_USBVIEW_USAGE);
                bStopArgProcessing = TRUE;
            }

            if (fileType != UsbViewNone)
            {
                // Save view information as to file
                szArg = strchr(szAnsiArg, ':');

                if (NULL == szArg || strlen(szArg) == 1)
                {
                    // No ':' or just a ':'
                    DisplayMessage(IDS_USBVIEW_INVALID_FILENAME, szAnsiArg);
                    DisplayMessage(IDS_USBVIEW_USAGE);
                    bStopArgProcessing = TRUE;
                }
                else
                {
                    hr = ProcessCommandSaveFile(szArg + 1, dwCreationDisposition, fileType);

                    if (FAILED(hr))
                    {
                        // No more processing
                        bStopArgProcessing = TRUE;
                    }

                    fileType = UsbViewNone;
                }
            }

            if (NULL != szAnsiArg)
            {
                LocalFree(szAnsiArg);
            }

        }

        if(!quietMode)
        {
            WaitForKeyPress();
        }

        if (gbConsoleInitialized)
        {
            UnInitializeConsole();
        }

        LocalFree(szArgList);
    }
    return bStatus;
}


/*****************************************************************************

ProcessCommandSaveFile()

Process the save file command line

*****************************************************************************/
HRESULT ProcessCommandSaveFile(LPTSTR szFileName, DWORD dwCreationDisposition, USBVIEW_SAVE_FILE_TYPE fileType)
{
    HRESULT hr = S_OK;
    LPTSTR szErrorBuffer = NULL;

    if (UsbViewNone == fileType || NULL == szFileName)
    {
        hr = E_INVALIDARG;
        // Invalid arguments, return
        return (hr);
    }

    // The UI is not created yet, open the UI, but HIDE it
    CreateMainWindow(SW_HIDE);

    if (UsbViewXmlFile == fileType)
    {
        hr = SaveAllInformationAsXml(szFileName, dwCreationDisposition);
    }

    if (UsbViewTxtFile == fileType)
    {
        hr = SaveAllInformationAsText(szFileName, dwCreationDisposition);
    }

    if (FAILED(hr))
    {
        if (GetLastError() == ERROR_FILE_EXISTS || hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
        {
            // The operation failed because the file we tried to write to already existed and '/f' option
            // was not present. Display error message to user describing '/f' option
            switch(fileType)
            {
                case UsbViewXmlFile:
                    DisplayMessage(IDS_USBVIEW_FILE_EXISTS_XML, szFileName);
                    break;
                case UsbViewTxtFile:
                    DisplayMessage(IDS_USBVIEW_FILE_EXISTS_TXT, szFileName);
                    break;
                default:
                    DisplayMessage(IDS_USBVIEW_INTERNAL_ERROR);
                    break;
            }

        }
        else
        {
            // Try to obtain system error message
            FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &szErrorBuffer,        // FormatMessage expects this buffer to be cast as LPTSTR
                    0,
                    NULL);
            PrintString("Unable to save file.\n");
            PrintString(szErrorBuffer);
            LocalFree(szErrorBuffer);
        }
    }
    else
    {
        // Display file saved to message in console
        DisplayMessage(IDS_USBVIEW_SAVED_TO, szFileName);
    }

    return (hr);
}

/*****************************************************************************

InitializeConsole()

Initializes the std output in console

*****************************************************************************/
HRESULT InitializeConsole()
{
    HRESULT hr = S_OK;

    SetLastError(0);

    // Find if STD_OUTPUT is a console or has been redirected to a File
    gbConsoleFile = IsStdOutFile();

    if (!gbConsoleFile)
    {
        // Output is not redirected and GUI application do not have console by default, create a console
        if(AllocConsole())
        {
#pragma warning(disable:4996) // We don' need the FILE * returned by freopen
            // Reopen STDOUT , STDIN and STDERR
            if((freopen("conout$", "w", stdout) != NULL) &&
                    (freopen("conin$", "r", stdin)  != NULL) &&
                    (freopen("conout$","w", stderr) != NULL))
            {
                gbConsoleInitialized = TRUE;
                ghStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            }
#pragma warning(default:4996)
        }
    }

    if (INVALID_HANDLE_VALUE == ghStdOut || FALSE == gbConsoleInitialized)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        OOPS();
    }
    return hr;
}

/*****************************************************************************

UnInitializeConsole()

UnInitializes the console

*****************************************************************************/
VOID UnInitializeConsole()
{
    gbConsoleInitialized = FALSE;
    FreeConsole();
}

/*****************************************************************************

IsStdOutFile()

Finds if the STD_OUTPUT has been redirected to a file
*****************************************************************************/
BOOL IsStdOutFile()
{
    unsigned htype;
    HANDLE hFile;

    // 1 = STDOUT
    hFile = (HANDLE) _get_osfhandle(1);
    htype = GetFileType(hFile);
    htype &= ~FILE_TYPE_REMOTE;


    // Check if file type is character file
    if (FILE_TYPE_DISK == htype)
    {
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************

DisplayMessage()

Displays a message to standard output
*****************************************************************************/
VOID DisplayMessage(DWORD dwResId, ...)
{
    CHAR szFormat[4096];
    HRESULT hr = S_OK;
    LPTSTR lpszMessage = NULL;
    DWORD dwLen = 0;
    va_list ap;

    va_start(ap, dwResId);

    // Initialize console if needed
    if (!gbConsoleInitialized)
    {
        hr = InitializeConsole();
        if (FAILED(hr))
        {
            OOPS();
            return;
        }
    }

    // Load the string resource
    dwLen = LoadString(GetModuleHandle(NULL),
            dwResId,
            szFormat,
            ARRAYSIZE(szFormat)
            );

    if(0 == dwLen)
    {
        PrintString("Unable to find message for given resource ID");

        // Return if resource ID could not be found
        return;
    }

    dwLen = FormatMessage(
                FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                szFormat,
                dwResId,
                0,
                (LPTSTR) &lpszMessage,
                ARRAYSIZE(szFormat),
                &ap);

    if (dwLen > 0)
    {
        PrintString(lpszMessage);
        LocalFree(lpszMessage);
    }
    else
    {
        PrintString("Unable to find message for given ID");
    }

    va_end(ap);
    return;
}

/*****************************************************************************

WStringToAnsiString()

Converts the Wide char string to ANSI string and returns the allocated ANSI string.
*****************************************************************************/
LPTSTR WStringToAnsiString(LPWSTR lpwszString)
{
    int strLen = 0;
    LPTSTR szAnsiBuffer = NULL;

    szAnsiBuffer = LocalAlloc(LPTR, (MAX_PATH + 1) * sizeof(CHAR));

    // Convert string from from WCHAR to ANSI
    if (NULL != szAnsiBuffer)
    {
        strLen = WideCharToMultiByte(
                CP_ACP,
                0,
                lpwszString,
                -1,
                szAnsiBuffer,
                MAX_PATH + 1,
                NULL,
                NULL);

        if (strLen > 0)
        {
            return szAnsiBuffer;
        }
    }
    return NULL;
}

/*****************************************************************************

PrintString()

Displays a string to standard output
*****************************************************************************/
VOID PrintString(LPTSTR lpszString)
{
    DWORD dwBytesWritten = 0;
    size_t Len = 0;
    LPSTR lpOemString = NULL;

    if (INVALID_HANDLE_VALUE == ghStdOut || NULL == lpszString)
    {
        OOPS();
        // Return if invalid inputs
        return;
    }

    if (FAILED(StringCchLength(lpszString, OUTPUT_MESSAGE_MAX_LENGTH, &Len)))
    {
        OOPS();
        // Return if string is too long
        return;
    }

    if (gbConsoleFile)
    {
        // Console has been redirected to a file, ex: `usbview /savexml:xx > test.txt`. We need to use WriteFile instead of
        // WriteConsole for text output.
        lpOemString = (LPSTR) LocalAlloc(LPTR, (Len + 1) * sizeof(CHAR));
        if (lpOemString != NULL)
        {
            if (CharToOemBuff(lpszString, lpOemString, (DWORD) Len))
            {
                WriteFile(ghStdOut, (LPVOID) lpOemString, (DWORD) Len, &dwBytesWritten, NULL);
            }
            else
            {
                OOPS();
            }
        }
    }
    else
    {
        // Write to std out in console
        WriteConsole(ghStdOut, (LPVOID) lpszString, (DWORD) Len, &dwBytesWritten, NULL);
    }

    return;
}

/*****************************************************************************

WaitForKeyPress()

Waits for key press in case of console
*****************************************************************************/
VOID WaitForKeyPress()
{
    // Wait for key press if console
    if (!gbConsoleFile && gbConsoleInitialized)
    {
        DisplayMessage(IDS_USBVIEW_PRESSKEY);
        (VOID) _getch();
    }
    return;
}

/*****************************************************************************

CreateMainWindow()

*****************************************************************************/

BOOL
CreateMainWindow (
                  int nCmdShow
                  )
{
    RECT rc;
	INITCOMMONCONTROLSEX icex;

	// Ensure that the common control DLL is loaded. 
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_HOTKEY_CLASS | ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx( &icex );
	InitCommonControls();

    ghMainWnd = CreateDialog(ghInstance,
        MAKEINTRESOURCE(IDD_MAINDIALOG),
        NULL,
        (DLGPROC) MainDlgProc);

    if (ghMainWnd == NULL)
    {
        OOPS();
        return FALSE;
    }

    GetWindowRect(ghMainWnd, &rc);

    gBarLocation = (rc.right - rc.left) / 3;

    ResizeWindows(FALSE, 0);

    ShowWindow(ghMainWnd, nCmdShow);

    UpdateWindow(ghMainWnd);

    return TRUE;
}


/*****************************************************************************

ResizeWindows()

*****************************************************************************/

//! Resizes the child windows
/*!
Handles resizing the child windows of the main window.  If
bSizeBar is true, then the sizing is happening because the user is
moving the bar.  If bSizeBar is false, the sizing is happening
because of the WM_SIZE or something like that.
*/

VOID
ResizeWindows (
               BOOL    bSizeBar,
               int     BarLocation
               )
{
    RECT    MainClientRect;
    RECT    MainWindowRect;
    //RECT    TreeWindowRect;
	RECT    ListWindowRect;
    RECT    StatusWindowRect;
	RECT    SaveWindowRect;
    int     right;

    // Is the user moving the bar?
    //
    if (!bSizeBar)
    {
        BarLocation = gBarLocation;
		
    }
	printf("%i\n", gBarLocation);
    GetClientRect(ghMainWnd, &MainClientRect);

    GetWindowRect(ghStatusWnd, &StatusWindowRect);

    // Make sure the bar is in a OK location
    //
    if (bSizeBar)
    {
        if (BarLocation <
            GetSystemMetrics(SM_CXSCREEN)/WINDOWSCALEFACTOR)
        {
            return;
        }

        if ((MainClientRect.right - BarLocation) <
            GetSystemMetrics(SM_CXSCREEN)/WINDOWSCALEFACTOR)
        {
            return;
        }
    }

    // Save the bar location
    //
    gBarLocation = BarLocation;

    
	//move the list window
	MoveWindow(ghListWnd,
		0,
		0,
		BarLocation,
		(StatusWindowRect.top - StatusWindowRect.bottom + MainClientRect.bottom)/2,
		TRUE);

	
	
    // Get the size of the window (in case move window failed
    //
    //GetWindowRect(ghTreeWnd, &TreeWindowRect);
	GetWindowRect(ghListWnd, &ListWindowRect);
    GetWindowRect(ghMainWnd, &MainWindowRect);

	//move the save window
	MoveWindow(ghSaveWnd,
		0,
		(StatusWindowRect.top - StatusWindowRect.bottom + MainClientRect.bottom) / 2,
		BarLocation,
		(StatusWindowRect.top - StatusWindowRect.bottom + MainClientRect.bottom) / 2 - 30,
		TRUE);
	
	GetWindowRect(ghSaveWnd, &SaveWindowRect);

	
	//move the buttons
	MoveWindow(ghButton,
		0,
		(StatusWindowRect.top - StatusWindowRect.bottom + MainClientRect.bottom) - 30,
		100,
		30,
		TRUE);

	MoveWindow(ghButton2,
		100,
		(StatusWindowRect.top - StatusWindowRect.bottom + MainClientRect.bottom) - 30,
		100,
		30,
		TRUE);

	right = ListWindowRect.right - MainWindowRect.left;

    // Move the edit window with respect to the tree window
    //
    MoveWindow(ghEditWnd,
        right+SIZEBAR,
        0,
        MainClientRect.right-(right+SIZEBAR),
        MainClientRect.bottom - StatusWindowRect.bottom + StatusWindowRect.top,
        TRUE);

	
    // Move the Status window with respect to the tree window
	//
	MoveWindow(ghStatusWnd,
		0,
		MainClientRect.bottom - StatusWindowRect.bottom + StatusWindowRect.top,
		MainClientRect.right,
		StatusWindowRect.bottom - StatusWindowRect.top,
		TRUE);
}


/*****************************************************************************

MainDlgProc()

*****************************************************************************/

//! Is the main message function 
/*! Is a Custom message handling routine of this program.
Note that the inclusion of DefWindowProc caused adverse side effects and is intentionally not used
*/

LRESULT CALLBACK
MainDlgProc (
             HWND   hWnd,
             UINT   uMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{

    switch (uMsg)
    {
		/*
		case WM_SETCURSOR:
		printf("set cursor is here\n");
		if (1)
		{
			SetClassLong(hWnd,    // window handle 
				GCL_HCURSOR,      // change cursor 
				ghSplitCursor);   // new cursor 
			break;
		}
		break;
		*/

        HANDLE_MSG(hWnd, WM_INITDIALOG,     USBView_OnInitDialog);
        HANDLE_MSG(hWnd, WM_CLOSE,          USBView_OnClose);
        HANDLE_MSG(hWnd, WM_COMMAND,        USBView_OnCommand);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN,    USBView_OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONUP,      USBView_OnLButtonUp);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE,      USBView_OnMouseMove);
        HANDLE_MSG(hWnd, WM_SIZE,           USBView_OnSize);
        HANDLE_MSG(hWnd, WM_NOTIFY,         USBView_OnNotify);
        HANDLE_MSG(hWnd, WM_DEVICECHANGE,   USBView_OnDeviceChange);
		//HANDLE_MSG(hWnd, WM_SETCURSOR,      USBView_SetCursor);
		
    }

    return 0;
}

/*****************************************************************************

USBView_OnInitDialog()

*****************************************************************************/
//! Initilizes elements of the main window and registers for usb notifications
/*! This is called when an WM_INITDIALOG messager is sent.
the main portions of this program
1) Register to receive notification when a USB device is plugged in.
2) Create tree, list and both button windows
3) Sets properties for the edit window
4) Makes a call to RefreshTree()

Note: legacy code from OEM usbview is the creation of an immage list.
The code is kept incase an images ever want to be implimented
*/
BOOL
USBView_OnInitDialog (
                      HWND    hWnd,
                      HWND    hWndFocus,
                      LPARAM  lParam
                      )
{
    HFONT                           hFont;
    HIMAGELIST                      himl;
    HICON                           hicon;
    DEV_BROADCAST_DEVICEINTERFACE   broadcastInterface;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(hWndFocus);

	//register the standard cursor
	standard_cursor = LoadCursorA(NULL, IDC_ARROW);

    // Register to receive notification when a USB device is plugged in.
    broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    memcpy( &(broadcastInterface.dbcc_classguid),
        &(GUID_DEVINTERFACE_USB_DEVICE),
        sizeof(struct _GUID));

    gNotifyDevHandle = RegisterDeviceNotification(hWnd,
        &broadcastInterface,
        DEVICE_NOTIFY_WINDOW_HANDLE);

   // // Now register for Hub notifications.
   // memcpy( &(broadcastInterface.dbcc_classguid),
   //     &(GUID_CLASS_USBHUB),
   //     sizeof(struct _GUID));

   // gNotifyHubHandle = RegisterDeviceNotification(hWnd,
    //    &broadcastInterface,
   //     DEVICE_NOTIFY_WINDOW_HANDLE);

    gHubList.DeviceInfo = INVALID_HANDLE_VALUE;
    InitializeListHead(&gHubList.ListHead);
    gDeviceList.DeviceInfo = INVALID_HANDLE_VALUE;
    InitializeListHead(&gDeviceList.ListHead);

    //end add
	//create tree window handle
	ghTreeWnd = CreateWindowEx(0,
		WC_TREEVIEW,
		TEXT("Tree View"),
		 WS_CHILD | WS_BORDER | WS_TABSTOP | TVS_HASLINES | TVS_HASBUTTONS ,
		0,
		0,
		0,	//120,
		0,	//234,
		hWnd,
		(HMENU)IDC_TREE,
		ghInstance,
		NULL);


	ghListWnd = CreateWindowEx(0,
		WC_LISTVIEW,
		TEXT("List View"),
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT ,
		0,
		0,
		120,
		260,
		hWnd,
		(HMENU)IDC_LIST,
		ghInstance,
		NULL);

	ghSaveWnd = GetDlgItem(hWnd, IDC_SAVEWINDOW);


	 ghButton = CreateWindow(
		"BUTTON",  // Predefined class; Unicode assumed 
		"Save Selected",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_CLIPSIBLINGS,  // Styles 
		0,         // x position 
		300,         // y position 
		80,        // Button width
		30,        // Button height
		hWnd,     // Parent window
		(HMENU) IDC_BUTTON,       
		ghInstance,
		NULL);      // Pointer not needed.

	
	
	 ghButton2 = CreateWindow(
		 "BUTTON",  // Predefined class; Unicode assumed 
		 "Save All USB",      // Button text 
		 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_CLIPSIBLINGS,  // Styles 
		 0,         // x position 
		 300,         // y position 
		 80,        // Button width
		 30,        // Button height
		 hWnd,     // Parent window
		 (HMENU)IDC_BUTTON2,       // No menu.
		 ghInstance,
		 NULL);      // Pointer not needed.
	
	

   

    //added
    if ((himl = ImageList_Create(15, 15,
        FALSE, 2, 0)) == NULL)
    {
        OOPS();
    }

    if(himl != NULL)
    {
        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_ICON));
        giGoodDevice = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_BADICON));
        giBadDevice = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_COMPUTER));
        giComputer = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_HUB));
        giHub = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_NODEVICE));
        giNoDevice = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_SSICON));
        giGoodSsDevice = ImageList_AddIcon(himl, hicon);

        hicon = LoadIcon(ghInstanceL, MAKEINTRESOURCE(IDI_NOSSDEVICE));
        giNoSsDevice = ImageList_AddIcon(himl, hicon);

        TreeView_SetImageList(ghTreeWnd, himl, TVSIL_NORMAL);
		ListView_SetImageList(ghListWnd, himl, LVSIL_NORMAL);
		// end add
    }

    ghEditWnd = GetDlgItem(hWnd, IDC_EDIT);
	//ShowWindow(ghEditWnd, SW_HIDE);


#ifdef H264_SUPPORT
    // set the edit control to have a max text limit size
    SendMessage(ghEditWnd, EM_LIMITTEXT, 0 /* USE DEFAULT MAX*/, 0);
#endif

    ghStatusWnd = GetDlgItem(hWnd, IDC_STATUS);
	//ShowWindow(ghStatusWnd, SW_HIDE);

    ghMainMenu = GetMenu(hWnd);
    if (ghMainMenu == NULL)
    {
        OOPS();
    }
	
    {
        CHAR pszFont[256];
        CHAR pszHeight[8];

        memset(pszFont, 0, sizeof(pszFont));
        LoadString(ghInstance, IDS_STANDARD_FONT, pszFont, sizeof(pszFont) - 1);
        memset(pszHeight, 0, sizeof(pszHeight));
        LoadString(ghInstance, IDS_STANDARD_FONT_HEIGHT, pszHeight, sizeof(pszHeight) - 1);

        hFont  = CreateFont((int) pszHeight[0],  0, 0, 0,
            400, 0, 0, 0,
            0,   1, 2, 1,
            49, pszFont);
    }
    SendMessage(ghEditWnd,
        WM_SETFONT,
        (WPARAM) hFont,
        0);

    RefreshTree();

    return FALSE;
}

/*****************************************************************************

USBView_OnClose()

*****************************************************************************/
//! Cleanup function
/*! Is called when the user closes the application. It frees all dynamically allocated memory
in both the tree and list. Calls DestroyTree and DestroyList.
*/
VOID
USBView_OnClose (
                 HWND hWnd
                 )
{

    UNREFERENCED_PARAMETER(hWnd);

    DestroyTree();
	DestroyList();
	DestroyCursor(standard_cursor);
	DestroyWindow(hWnd);
    PostQuitMessage(0);
}

/*****************************************************************************

CreateAdditionalText()

*****************************************************************************/

//! Generates text to be appended to top of the edit window
/*! 
* Takes a pointer to the "list name" of the usb and returns the pointer to a string with the generated text.
* This funciton allocates memory for additional_text string. Then 1) checks if the usb is writable, 2) checks if the
* usb has a valid serial number and 3) generates a hash of the list name and serial number.
* 
*/
char * CreateAdditionalText(char* listname)
{
	char* additional_text = (char*)malloc(300 * sizeof(char));
	additional_text[0] = '\0';
	int is_writeable = 0, has_serial = 0; //flags

	//checks to see if a drive volume could be established
	if (listname[0] != ':')
	{
		
		//get a hold of this devices serial number and check if it is writeable. Does this by 
		//making an inner join on the drive letter from the list name and the letter variable of myList global 
		char* serial_str = NULL;
		for (int i = 0; i < DeviceCount; i++)
		{
			if (listname[0] == myList[i]->letter[0])
			{
				serial_str = myList[i]->serialnum;
				is_writeable = myList[i]->is_writeable;
				break;
			}
		}

		if (is_writeable)
		{
			strcat(additional_text, "\r\n\r\nThis stick is WRITABLE");
		}
		else
		{
			strcat(additional_text, "\r\n\r\nThis stick is NOT WRITABLE");
		}

		//checks to make sure that the serial number is valid and is writable
		if (is_writeable && serial_str != NULL && serial_str[0] != '\0' && serial_str[0] != ' ')
		{

			has_serial = 1;
			//now concatenate listname and serial number and pass to hash function
			char passtohash[300];
			passtohash[0] = '\0';
			strcat(passtohash, listname);
			strcat(passtohash, serial_str);
			char finalstr[11];
			
			//begin hash function

			char letters_numbers[] = { 'A','B', 'C', 'D', 'E', 'F', 'G', 'H','I','J',
				'K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a',
				'b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r',
				's','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0' };

			int hasharr[10];
			for (int i = 0; i < 10; i++)
			{
				hasharr[i] = 0;
			}

			for (int i = 0; passtohash[i] != '\0'; i++)
			{
				int j = i % 10;
				hasharr[j] += (int)passtohash[i];
				hasharr[j] *= hasharr[j];
				hasharr[j] %= 62;
			}

			
			for (int i = 0; i < 10; i++)
			{
				finalstr[i] = letters_numbers[hasharr[i]];
			}
			finalstr[10] = '\0';

			//end hash function


			strcat(additional_text, "\r\n\r\nUnique HiperID is: ");
			strcat(additional_text, finalstr);

		}
		else { //if the serial number was not valid
			strcat(additional_text, "\r\n\r\nHiperID could not be generated");
		}

	}
	else { //if the drive letter was not included in the list name and therfore not axcessable
		strcat(additional_text, "\r\n\r\nThis stick is NOT WRITEABLE");
	}

	char tmp[300];
	if (is_writeable && has_serial) { 
		strcpy(tmp, "\r\n\r\n------>>This usb is Hiperwall Compliant<<------");
		strcat(tmp ,additional_text);
		strcpy(additional_text, tmp);
		strcat(additional_text, "\r\n\r\n");
	}
	else {
		//notice the period..I am currently just using it as a flag in display.c
		strcpy(tmp, ".\r\n\r\n------>>This usb is NOT Hiperwall Compliant<<------");
		strcat(tmp, additional_text);
		strcpy(additional_text, tmp);
		strcat(additional_text, "\r\n\r\n");
	}
	return(additional_text);
}


/*****************************************************************************

AddItemInformationToFile()

*****************************************************************************/

//! Save info about hTreeItem to file pContext 
/*!
Saves the information of the treeitem hTreeItem to a file handle passed to function by pContext. Note that
the additional_text parameter contains all "custom" text generated in uvcview to be passed to saverile
File handle must be cast to a PVOID type before it is passed to this function. Unsure why but this function was OEM
*/

VOID
AddItemInformationToFile(
            HWND hTreeWnd,
            HTREEITEM hTreeItem,
            PVOID pContext,
			char* additional_text
        )
{
    HRESULT hr = S_OK;
    HANDLE hf = NULL;
    DWORD dwBytesWritten = 0;

    hf = *((PHANDLE) pContext);

    ResetTextBuffer();

    hr = UpdateTreeItemDeviceInfo(hTreeWnd, hTreeItem, additional_text);

    if (0) //FAILED(hr)
    {
        OOPS();
    }
    else
    {
        WriteFile(hf, GetTextBuffer(), GetTextBufferPos()*sizeof(CHAR), &dwBytesWritten, NULL);
    }

    ResetTextBuffer();
}



/*****************************************************************************

SaveAllInformationAsText()

*****************************************************************************/

//! Unused Legacy code. Saves the entire tree as a text file
/*!
This funciton saves the entire tree as a text file. That means all hubs, connectors, etc. Is unused in 
hiperwall usbview code but is kept incase future requirements demand this feature.
*/
HRESULT
SaveAllInformationAsText(
        LPTSTR lpstrTextFileName,
        DWORD dwCreationDisposition
        )
{
    HRESULT hr = S_OK;
    HANDLE hf = NULL;

    hf = CreateFile(lpstrTextFileName,
            GENERIC_WRITE,
            0,
            NULL,
            dwCreationDisposition,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hf == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        OOPS();
    }
    else
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            // CreateFile() sets this error if we are overwriting an existing file
            // Reset this error to avoid false alarms
            SetLastError(0);
        }

        if (ghTreeRoot == NULL)
        {
            // If tree has not been populated yet, try a refresh
            RefreshTree();
        }

        if (ghTreeRoot)
        {

            LockFile(hf, 0, 0, 0, 0);
            WalkTreeTopDown(ghTreeRoot, AddItemInformationToFile, &hf, NULL);
            UnlockFile(hf, 0, 0, 0, 0);
            CloseHandle(hf);

            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            OOPS();
        }
    }

    ResetTextBuffer();
    return hr;
}

/*****************************************************************************

GetUSBDrive_filepath()

*****************************************************************************/
//! Gets filepath to be used for writes to usb
/*! Takes the list name of an item and returns a string to a the usb file path 
First it checks to make sure that it has a valid drive letter.
Next it allocates a string to pass as the return value
Then it takes the drive letter, formats it to something like D:\\ aand then returns it to the user.
If the name passed is bad then NULL is returned
*/
char* GetUSBDrive_filepath(char* name) {
	if (  (name != NULL) && (name[0] != ':')) {
		char * file_destination = (char*) malloc((LISTNAMESIZE + 20) * sizeof(char) );
		file_destination[0] = name[0]; file_destination[1] = name[1];
		file_destination[2] = file_destination[3] = '\\'; file_destination[4] = '\0';
		return file_destination;
	}
	return NULL;
}




/*****************************************************************************

USBView_OnCommand()

*****************************************************************************/

//! Identifies and impliments any command messages passed
/*!This function at its core takes care of any actions to take when the user selects a button 
at the top window bar, or when the user presses on of the two save buttons.
Notable is the save and save all cases. This is where a file name is generated from usbname and by gettin the current date,
a file is opened and subsequently written to containing relavent data.
Also at the completion of save or save_all the save window text is updated to reflect the files paths written to on 
sucess and the files that could not be written to on failure.
*/

VOID
USBView_OnCommand (
                   HWND hWnd,
                   WPARAM  id,
                   HWND hwndCtl,
                   UINT codeNotify
                   )
{
    MENUITEMINFO menuInfo;
    char            szFile[MAX_PATH + 1];
    OPENFILENAME    ofn;
    HANDLE          hf = NULL;
    DWORD           dwBytesWritten = 0;
    int             nTextLength = 0;
    size_t          lengthToNull = 0;
    HRESULT         hr = S_OK;

    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);

    //initialize save dialog variables
    memset(szFile, 0, sizeof(szFile));
    memset(&ofn, 0, sizeof(OPENFILENAME));

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = hWnd;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = 0;
    ofn.lpstrTitle      = NULL;
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	//printf("the loword is %i\n", LOWORD(id));
	//printf("the HIword is %i\n", HIWORD(id));

    switch (id)
    {
	
	
		
	
    case ID_AUTO_REFRESH:
        gDoAutoRefresh = !gDoAutoRefresh;
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask  = MIIM_STATE;
        menuInfo.fState = gDoAutoRefresh ? MFS_CHECKED : MFS_UNCHECKED;
        SetMenuItemInfo(ghMainMenu,
            id,
            FALSE,
            &menuInfo);
        break;

	case IDC_BUTTON:
    case ID_SAVE:
        {
			//Basically saves the current edit window text to the local computer and to the usb if possible
			
			//1024 should be sufficent for two lines of text
			//256 should be good for even longest file path
			char savewindow_text[1024], buffer[256];
			savewindow_text[0] = buffer[0] = '\0';
			strcat(savewindow_text, "SAVE WINDOW\r\n\r\n");

			time_t rawtime;
			struct tm * timeinfo;
			char cur_date[15],filename[LISTNAMESIZE+15];
			

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(cur_date, 15, "%m-%d-%y",timeinfo);
			if (SelectedUsbName != NULL) {
				//get rid of any unacceptable characters
				//note there will always be a ": " in the name by my design
				//so simply pass the pointer of the char array right after the space
				for (int i = 0; (SelectedUsbName[i] == '\0') || (i < LISTNAMESIZE); i++) {
					if (SelectedUsbName[i] == ':') {
						if (i == 1) { //this should always get triggered
							i = i + 2;
							snprintf(filename, MAX_PATH, "USB_%s_%c_%s.txt", cur_date, SelectedUsbName[0] ,(SelectedUsbName + i));
							break;
						}
						else { //this should never get triggered
							i = i + 2;
							snprintf(filename, MAX_PATH, "USB_%s_%s.txt", cur_date,  (SelectedUsbName + i));
							break;
						}
						
					}
				}
				
			}
			else {
				snprintf(filename, MAX_PATH, "USB_%s.txt", cur_date);
			}
			
            // initialize the save file name
            StringCchCopy(szFile, MAX_PATH, filename);
            ofn.lpstrFilter     = "Text\0*.TXT\0\0";
            ofn.lpstrDefExt     = "txt";

            //call dialog box
            if (! GetSaveFileName(&ofn))
            {
				DWORD error = CommDlgExtendedError();
                OOPS();
				strcat(savewindow_text, "Failure in getting file name \r\n");
                break;
            }

            //create new file where they want
            hf = CreateFile((LPTSTR)ofn.lpstrFile,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (hf == INVALID_HANDLE_VALUE)
            {
				sprintf(buffer, "Could not create a file at %s \r\n", ofn.lpstrFile);
				strcat(savewindow_text, buffer);
                OOPS();
            }
            else
            {
                char *szText = NULL;

                //get data from display window to transfer to file
                nTextLength = GetWindowTextLength(ghEditWnd);
                nTextLength++;

                szText = ALLOC((DWORD)nTextLength);
                if (NULL != szText)
                {
                    GetWindowText(ghEditWnd, (LPSTR) szText, nTextLength);

                    //
                    // Constrain length to the first null, which should be at
                    // the end of the window text. This prevents writing extra
                    // null characters.
                    //
                    if (StringCchLength(szText, nTextLength, &lengthToNull) == S_OK)
                    {
                        nTextLength = (int) lengthToNull;

                        //lock the file, write to the file, unlock file
                        LockFile(hf, 0, 0, 0, 0);

                        WriteFile(hf, szText, nTextLength, &dwBytesWritten, NULL);

                        UnlockFile(hf, 0, 0, 0, 0);

						sprintf(buffer, "File created at %s \r\n", ofn.lpstrFile);
						strcat(savewindow_text, buffer);
                    }
                    else
                    {
                        OOPS();
						sprintf(buffer, "Could not create a file at %s \r\n", ofn.lpstrFile);
						strcat(savewindow_text,buffer);

                    }
                    CloseHandle(hf);
                    FREE(szText);
                }
                else
                {
                    OOPS();
                }
            }
			char* file_destination;
			if ((file_destination = GetUSBDrive_filepath(SelectedUsbName)) != NULL)
			{
				
				strcat(file_destination, filename);
				printf("%s\n", file_destination);
				hf = NULL;
				hf = CreateFile((LPTSTR) file_destination,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

				

				if (hf == INVALID_HANDLE_VALUE) {
					sprintf(buffer, "Could not create a file at %s \r\n", file_destination);
					strcat(savewindow_text, buffer);
					OOPS();
				}
				else
				{
					char *szText = NULL;

					//get data from display window to transfer to file
					nTextLength = GetWindowTextLength(ghEditWnd);
					nTextLength++;

					szText = ALLOC((DWORD)nTextLength);
					if (NULL != szText)
					{
						GetWindowText(ghEditWnd, (LPSTR)szText, nTextLength);

						//
						// Constrain length to the first null, which should be at
						// the end of the window text. This prevents writing extra
						// null characters.
						//
						if (StringCchLength(szText, nTextLength, &lengthToNull) == S_OK)
						{
							nTextLength = (int)lengthToNull;

							//lock the file, write to the file, unlock file
							LockFile(hf, 0, 0, 0, 0);

							WriteFile(hf, szText, nTextLength, &dwBytesWritten, NULL);

							UnlockFile(hf, 0, 0, 0, 0);
							sprintf(buffer, "File created at %s \r\n", file_destination);
							strcat(savewindow_text, buffer);
						}
						else
						{
							sprintf(buffer, "Could not create a file at %s \r\n", file_destination);
							strcat(savewindow_text, buffer);
							OOPS();
						}
						
					FREE(szText);
					}
					else
					{
						OOPS();
					}
				}
				free(file_destination);
				CloseHandle(hf);
			}
			strcat(savewindow_text, "UPDATE Save Current View has completed\r\n");

			SetWindowText(ghSaveWnd, savewindow_text);

           break;
        }
	case IDC_BUTTON2:
    case ID_SAVEALL:
	{

		//i am going to want to traverse the list and for every list item i am going to 
		//want to open a file for it on local computer and on the usb, and for each file handle
		//pass addItemInformationToFile for it.

		//4096 
			char savewindow_text[4096], buffer[256], NotWritableList[1024];
			savewindow_text[0] = buffer[0] = NotWritableList[0] = '\0';
			int popup = 0;

			strcat(savewindow_text,"SAVE WINDOW\r\n\r\n");
			strcat(NotWritableList, "Warning these items were not Hiperwall compliant and could not be saved\r\n\r\n");

			int num_of_list_items = ListView_GetItemCount(ghListWnd);
			printf("num of list items is %i\n", num_of_list_items);
			
			//for each list item
			for (int i = 0; i < num_of_list_items; i++)
			{
				//axcess the list item directly by itemnumber from a query of the listview controler
				LVITEMA item;
				char text[LISTNAMESIZE];
				item.iItem = i;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM | LVIF_TEXT;
				item.pszText = text;
				item.cchTextMax = LISTNAMESIZE;
				ListView_GetItem(ghListWnd, &item);


				//check to see if the item selected is hiperwall compatable step one
				if (item.pszText[0] != ':')
				{

					//get a hold of this devices serial number and chek if it is writeable. Does this by 
					//making an inner join on the drive letter from the list name and the letter variable of myList global 
					char* serial_str = NULL;
					int is_writeable = 0;
					for (int i = 0; i < DeviceCount; i++)
					{
						if (item.pszText[0] == myList[i]->letter[0])
						{
							serial_str = myList[i]->serialnum;
							is_writeable = myList[i]->is_writeable;
							break;
						}
					}
					if (is_writeable && serial_str != NULL && serial_str[0] != '\0' && serial_str[0] != ' ')
					{
						
						//get/make file name
						time_t rawtime;
						struct tm * timeinfo;
						char cur_date[15], filename[LISTNAMESIZE + 15];


						time(&rawtime);
						timeinfo = localtime(&rawtime);
						strftime(cur_date, 15, "%m-%d-%y", timeinfo);
						if (item.pszText != NULL) {
							//get rid of any unacceptable characters
							//note there will always be a ": " in the name by my design
							//so simply pass the pointer of the char array right after the space
							for (int i = 0; (item.pszText[i] == '\0') || (i < LISTNAMESIZE); i++)
							{
								if (item.pszText[i] == ':')
								{
									if (i == 1) { //this should always get triggered
										i = i + 2;
										snprintf(filename, MAX_PATH, "USB_%s_%c_%s.txt", cur_date, item.pszText[0], (item.pszText + i));
										break;
									}
									else { //this should never get triggered
										i = i + 2;
										snprintf(filename, MAX_PATH, "USB_%s_%s.txt", cur_date, (item.pszText + i));
										break;
									}
								}
							}

						}
						else {
							snprintf(filename, MAX_PATH, "USB_%s.txt", cur_date);
						}


						//get the handle to a file with the filename for saving on local drive
						hf = NULL;
						hf = CreateFile((LPTSTR)filename,
							GENERIC_WRITE,
							0,
							NULL,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL);

						if (hf == INVALID_HANDLE_VALUE) {
							sprintf(buffer, "Could not create a file at %s \r\n", filename);
							strcat(savewindow_text, buffer);
							OOPS();
						}
						else
						{
							//get the respective tree item from the current list item
							HTREEITEM hTreeItem;
							hTreeItem = (HTREEITEM)item.lParam;

							//get the additional text to add
							char* add_text = CreateAdditionalText(item.pszText);

							//write to the computer drive
							AddItemInformationToFile(ghTreeWnd, hTreeItem, &hf, add_text);
							CloseHandle(hf);
							sprintf(buffer, "File created in app directory. File name: %s \r\n", filename);
							strcat(savewindow_text, buffer);

							//write file to usb drive
							char* file_destination;
							if ((file_destination = GetUSBDrive_filepath(item.pszText)) != NULL) {

								strcat(file_destination, filename);
								hf = NULL;
								hf = CreateFile((LPTSTR)file_destination,
									GENERIC_WRITE,
									0,
									NULL,
									CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL,
									NULL);

								if (hf == INVALID_HANDLE_VALUE) {
									sprintf(buffer, "Could not create a file at %s \r\n", file_destination);
									strcat(savewindow_text, buffer);
									OOPS();
								}
								else
								{
									AddItemInformationToFile(ghTreeWnd, hTreeItem, &hf, add_text);
									sprintf(buffer, "File created at %s \r\n", file_destination);
									strcat(savewindow_text, buffer);

								}
								CloseHandle(hf);

							}
							//free memory
							free(add_text);
							free(file_destination);
						}

					}//end of if writeable && ...
					else {
						//add string to popup
						sprintf(buffer, "%s\r\n", item.pszText);
						strcat(NotWritableList, buffer);
						popup = 1;
						
					}
				}//end of if :
				else {
					//add string to popup
					sprintf(buffer, "%s\r\n", item.pszText);
					strcat(NotWritableList, buffer);
					popup = 1;
				}

				
				
			}//end of for loop
			

			strcat(savewindow_text, "UPDATE Save All has completed\r\n");
			SetWindowText(ghSaveWnd, savewindow_text);
            
			if (popup) {
				MessageBox(ghMainWnd, NotWritableList, "Hiperdrive", MB_OK);
			}
			break;
        }

    case ID_SAVEXML:
        {
            // initialize the save file name
            StringCchCopy(szFile, MAX_PATH, "USBViewAll.xml");
            ofn.lpstrFilter     = "Xml\0*.xml\0\0";
            ofn.lpstrDefExt     = "xml";

            //call dialog box
            if (! GetSaveFileName(&ofn))
            {
                OOPS();
                break;
            }

            // Save the file, overwrite in case of UI since UI gives popup for confirmation
            hr = SaveAllInformationAsXml(ofn.lpstrFile, CREATE_ALWAYS);
            if (FAILED(hr))
            {
                OOPS();
            }

            break;
        }

    case ID_CONFIG_DESCRIPTORS:
        gDoConfigDesc = !gDoConfigDesc;
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask  = MIIM_STATE;
        menuInfo.fState = gDoConfigDesc ? MFS_CHECKED : MFS_UNCHECKED;
        SetMenuItemInfo(ghMainMenu,
            id,
            FALSE,
            &menuInfo);
        break;

    case ID_ANNOTATION:
        gDoAnnotation = !gDoAnnotation;
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask  = MIIM_STATE;
        menuInfo.fState = gDoAnnotation ? MFS_CHECKED : MFS_UNCHECKED;
        SetMenuItemInfo(ghMainMenu,
            id,
            FALSE,
            &menuInfo);
        break;

    case ID_LOG_DEBUG:
        gLogDebug       = !gLogDebug;
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask  = MIIM_STATE;
        menuInfo.fState = gLogDebug ? MFS_CHECKED : MFS_UNCHECKED;
        SetMenuItemInfo(ghMainMenu,
            id,
            FALSE,
            &menuInfo);
        break;

    case ID_ABOUT:
        DialogBox(ghInstance,
            MAKEINTRESOURCE(IDD_ABOUT),
            ghMainWnd,
            (DLGPROC) AboutDlgProc);
        break;

    case ID_EXIT:
        UnregisterDeviceNotification(gNotifyDevHandle);
        UnregisterDeviceNotification(gNotifyHubHandle);
        DestroyTree();
		DestroyList();
        PostQuitMessage(0);
        break;

    case ID_REFRESH:
        RefreshTree();
        break;
    }
}

/*****************************************************************************

USBView_OnLButtonDown()

*****************************************************************************/

VOID
USBView_OnLButtonDown (
                       HWND hWnd,
                       BOOL fDoubleClick,
                       int  x,
                       int  y,
                       UINT keyFlags
                       )
{

    UNREFERENCED_PARAMETER(fDoubleClick);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    gbButtonDown = TRUE;
	move_area = 100;
    SetCapture(hWnd);
}

/*****************************************************************************

USBView_OnLButtonUp()

*****************************************************************************/

VOID
USBView_OnLButtonUp (
                     HWND hWnd,
                     int  x,
                     int  y,
                     UINT keyFlags
                     )
{

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

    gbButtonDown = FALSE;
	move_area = 10;
    ReleaseCapture();
}

/*****************************************************************************

USBView_OnMouseMove()

*****************************************************************************/

VOID
USBView_OnMouseMove (
                     HWND hWnd,
                     int  x,
                     int  y,
                     UINT keyFlags
                     )
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(keyFlags);

	 


	if ( (x - gBarLocation) < move_area && (x - gBarLocation) > -move_area) {
		//SetCursor(IDC_HAND);
		SetClassLong(hWnd, GCL_HCURSOR, ghSplitCursor);
		if (gbButtonDown)
		{
			//printf("mousemove= %i\n", x);
			ResizeWindows(TRUE, x);
		}
	}
	else {
		//printf("mousemove yes yes= %i\n", x);
		SetClassLong(hWnd, GCL_HCURSOR, standard_cursor);
	}
}

/*****************************************************************************

USBView_OnSize();

*****************************************************************************/

VOID
USBView_OnSize (
                HWND hWnd,
                UINT state,
                int  cx,
                int  cy
                )
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);

    ResizeWindows(FALSE, 0);
}

/*****************************************************************************

USBView_OnNotify()

*****************************************************************************/

//! Responds to a change in the list window
/*! If the user clicks or changes selected item in the list window this function respons to the notify message. 
It identifys what is now the selected listview item and then subsequently updates the edit view to reflect this change.
*/
LRESULT
USBView_OnNotify (
                  HWND    hWnd,
                  int     DlgItem,
                  LPNMHDR lpNMHdr
                  )
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(DlgItem);

	//printf("Item changed notify received!!\n");

	if (lpNMHdr->code == LVN_ITEMCHANGED)
	{
		
		
		LPNMLISTVIEW lvmessage = (LPNMLISTVIEW)lpNMHdr;

		

		if (lvmessage->uNewState & LVIS_SELECTED)
		{
			if (lvmessage->iItem < 0)
				return 0;


			if (SelectedUsbName == NULL) {
				SelectedUsbName = (char*)calloc(LISTNAMESIZE, sizeof(char));
			}

			//query the ghListWnd for the selected item
			LVITEMA item;
			char text[LISTNAMESIZE];
			item.iItem = lvmessage->iItem;
			item.iSubItem = lvmessage->iSubItem;
			item.mask = LVIF_PARAM | LVIF_TEXT;
			item.pszText = text;
			item.cchTextMax = LISTNAMESIZE;
			ListView_GetItem(ghListWnd, &item);

			printf("selected list item text is %s\n", item.pszText);
			//selectedusbname global is updated
			strcpy(SelectedUsbName, item.pszText);

			//check to see if the item selected is hiperwall compatable
			if (SelectedUsbName[0] != ':')
			{
				
				//get a hold of this devices serial number and chek if it is writeable. Does this by 
				//making an inner join on the drive letter from the list name and the letter variable of myList global 
				char* serial_str = NULL;
				int is_writeable = 0;
				for (int i = 0; i < DeviceCount; i++)
				{
					if (SelectedUsbName[0] == myList[i]->letter[0])
					{
						serial_str = myList[i]->serialnum;
						is_writeable = myList[i]->is_writeable;
						break;
					}
				}
				if (is_writeable && serial_str != NULL && serial_str[0] != '\0' && serial_str[0] != ' ' )
				{
					Button_Enable(ghButton, TRUE);
					Button_Enable(ghButton2, TRUE);
					EnableMenuItem(ghMainMenu, ID_SAVE, MF_ENABLED);
					EnableMenuItem(ghMainMenu, ID_SAVEALL, MF_ENABLED);
				}
				else
				{
					Button_Enable(ghButton, FALSE);
					Button_Enable(ghButton2, TRUE);
					EnableMenuItem(ghMainMenu, ID_SAVE, MF_GRAYED);
					EnableMenuItem(ghMainMenu, ID_SAVEALL, MF_ENABLED);
				}

			}

			//get the selected treeitem and update the edit window
			
			HTREEITEM hTreeItem;

			hTreeItem = (HTREEITEM)item.lParam;

			if (hTreeItem)
			{


				char* add_text = CreateAdditionalText(SelectedUsbName);
				UpdateEditControl( ghEditWnd, ghTreeWnd, hTreeItem, add_text);
				free(add_text);

			}
		}


	}
	
    return 0;
}


/*****************************************************************************

USBView_OnDeviceChange()

*****************************************************************************/

BOOL
USBView_OnDeviceChange (
                        HWND  hwnd,
                        UINT  uEvent,
                        DWORD dwEventData
                        )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(dwEventData);

    if (gDoAutoRefresh)
    {
        switch (uEvent)
        {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
			printf("on device change refresh is called\n");
            RefreshTree();
            break;
        }
    }

	
    return TRUE;
}



/*****************************************************************************

DestroyTree()

*****************************************************************************/

//! clears all tree items
/*!
Walks down the tree and cleans all of the dynamicly allocated memory using WalkTree function. 
Then deletes all of the treeview items and nullifys the root.
*/

VOID DestroyTree (VOID)
{
    // Clear the selection of the TreeView, so that when the tree is
    // destroyed, the control won't try to constantly "shift" the
    // selection to another item.
    //
    TreeView_SelectItem(ghTreeWnd, NULL);

    // Destroy the current contents of the TreeView
    //
    if (ghTreeRoot)
    {
        WalkTree(ghTreeRoot, CleanupItem, NULL);

        TreeView_DeleteAllItems(ghTreeWnd);

        ghTreeRoot = NULL;
    }

    ClearDeviceList(&gDeviceList);
    ClearDeviceList(&gHubList);
}

/*****************************************************************************

DestroyList()

*****************************************************************************/
//! Deletes all list items
/*! Deletes all of the list itmes and delets the list column structre. 
Nullifies the selectedusbname global parameter and zeros out the numUsbConnected counter.
*/
VOID DestroyList(VOID)
{
	
	ListView_DeleteAllItems(ghListWnd);
	ListView_DeleteColumn(ghListWnd, 0);
	if (SelectedUsbName != NULL) {
		free(SelectedUsbName);
		SelectedUsbName = NULL;
	}
	//zero out usb counter
	numUsbConnected = 0;

}


/*****************************************************************************

GetUSBDevices()

*****************************************************************************/
//! Fills myList structure and associates a drive letter with a usb
/*! Takes TUSB_Device* array and size of array as parameters. Returns the number of devices found and stored
in the array. This function is particularly confusing and caution is advised if modified.
1)Creates an array of Letter_num structures and checks every path A: - Z: to see if there is a device connected
to said path. If sucessful associates drive number of that device with path letter.
2) Assuming sucess of 1), we now ensure that the drive is infact writable by creating a file test.txt on the 
drive, writing to it, and subsequently deleting said file.  
3) Now we attempt to fill in the autual devList array with TUSB_Device structures buy calling SetupDiGetDeviceInterfaceDetail
among other functions
4) Get the drive number again for the devList array, then make an inner join on the drive number between the 
devList array and the letter_number array to pass the drive letter and the is_readable flag to the devList array.
Also extract the device serial number from the devlist device path variable. Note that if device does not have serial number the 
serialnumber variable will either be set to an empty string '\0' or a string with a single space " ".
*/

int
GetUSBDevices(TUSB_Device *devList[], int size)
{
	HANDLE      hHCDev;
	HANDLE		hLetterDrive;
	HANDLE		hWriteable;

	HDEVINFO                         deviceInfo;
	SP_DEVICE_INTERFACE_DATA         deviceInfoData;
	ULONG                            index;
	ULONG                            requiredLength;
	int                              devCount = 0;
	//SP_DEVINFO_DATA                DevInfoData;

	// (1)

	//create an array of letter num structures with every capital number
	Letter_num  my_letter_num_array[26];
	char drive[16];

	//for every letter check to see if there is a drive connected to it, then get the number of the drive
	for (char i = 0; i <= 25; i++) {
		my_letter_num_array[i].letter[0] = i + 65;
		my_letter_num_array[i].letter[1] = '\0';
		sprintf(drive, "\\\\.\\%s:", my_letter_num_array[i].letter); 
		my_letter_num_array[i].is_writeable = 0;

		//attempt to get a handle to the drive path
		hLetterDrive = CreateFile(drive, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (hLetterDrive != INVALID_HANDLE_VALUE) {
			STORAGE_DEVICE_NUMBER sdn;
			DWORD BytesReturned;

			//this gets the devie
			DeviceIoControl(
				hLetterDrive,                // handle to device
				IOCTL_STORAGE_GET_DEVICE_NUMBER, // dwIoControlCode
				NULL,                            // lpInBuffer
				0,                               // nInBufferSize(LPVOID), 
				&sdn,           // output buffer
				sizeof(sdn),         // size of output buffer
				&BytesReturned,       // number of bytes returned
				NULL      // OVERLAPPED structure
			);

			//load the num into the letter drive array
			my_letter_num_array[i].num = sdn.DeviceNumber;
			

			// (2)

			//make a test file and write to this drive
			//create a test file
			char testfile[128];
			sprintf(testfile, "%s:\\\\test.txt", my_letter_num_array[i].letter);
			printf("%s  ", testfile);
			
			hWriteable = NULL;
			hWriteable = CreateFile((LPTSTR)testfile,
				GENERIC_WRITE,
				0,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			DWORD error = GetLastError();

			if (hWriteable != INVALID_HANDLE_VALUE)
			{
				//printf("file handle received \n");

				int ret = 0, biteswritten = 0;

				//lock the file, write to the file, unlock file
				LockFile(hWriteable, 0, 0, 0, 0);

				ret = WriteFile(hWriteable, testfile, 12, &biteswritten, NULL);

				UnlockFile(hWriteable, 0, 0, 0, 0);

				if (ret)
				{
					my_letter_num_array[i].is_writeable = 1;
				}

				
			}

			CloseHandle(hWriteable);

			//delete the file
			DeleteFileA((LPTSTR)testfile);
			
		}
		CloseHandle(hLetterDrive);

	}

	//  (3)

	// Now iterate over host controllers using the new GUID based interface
	//GUID_DEVINTERFACE_USB_DEVICE
	deviceInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVINTERFACE_DISK,
		NULL,
		NULL,
		(DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

	if (deviceInfo != INVALID_HANDLE_VALUE)
	{
		deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		for (index = 0;
			SetupDiEnumDeviceInterfaces(deviceInfo,
				0,
				(LPGUID)&GUID_DEVINTERFACE_DISK,
				index,
				&deviceInfoData);
			index++)
		{
			SetupDiGetDeviceInterfaceDetail(deviceInfo,
				&deviceInfoData,
				NULL,
				0,
				&requiredLength,
				NULL);

			//allocate memory for pointer to TUSB_Device structure
			devList[devCount] = (TUSB_Device*)malloc(sizeof(TUSB_Device));

			devList[devCount]->deviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, requiredLength);

			devList[devCount]->deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			SetupDiGetDeviceInterfaceDetail(deviceInfo,
				&deviceInfoData,
				devList[devCount]->deviceDetailData,
				requiredLength,
				&requiredLength,
				NULL);

			//open the usb device GENERIC_WRITE
			hHCDev = CreateFile(devList[devCount]->deviceDetailData->DevicePath,
				0,
				FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);


			// If the handle is valid, then we've successfully found a usb device
			//
			if (hHCDev != INVALID_HANDLE_VALUE)
			{
				
			// (4)

				strncpy(devList[devCount]->devicePath, devList[devCount]->deviceDetailData->DevicePath, sizeof(devList[devCount]->devicePath));

				//get the device number
				STORAGE_DEVICE_NUMBER sdn;
				DWORD BytesReturned;
				DeviceIoControl(
					hHCDev,                // handle to device
					IOCTL_STORAGE_GET_DEVICE_NUMBER, // dwIoControlCode
					NULL,                            // lpInBuffer
					0,                               // nInBufferSize(LPVOID), 
					&sdn,           // output buffer
					sizeof(sdn),         // size of output buffer
					&BytesReturned,       // number of bytes returned
					NULL      // OVERLAPPED structure
				);


				//run through the letter num array to find the letter associated with this drive number
				for (int j = 0; j < 26; j++) {
					if (my_letter_num_array[j].num == sdn.DeviceNumber) { //storage device number match
						strncpy(devList[devCount]->letter, my_letter_num_array[j].letter, 2);
						devList[devCount]->is_writeable = my_letter_num_array[j].is_writeable;
						break;
					}
					if (j == 25) { //no match has been made, set string to "!", and say not writeable
						devList[devCount]->letter[0] = '!';
						devList[devCount]->letter[1] = '\0';
						devList[devCount]->is_writeable = 0;
					}
				}



				/* we have a usb that im assuming has a serial number (or space if usb has no serial number) inbetween the second # and an &
				 After doing some research, emmet gray of http://www.emmet-gray.com/Articles/USB_SerialNumbers.html summarizes it best as 
				 "Although it is generally accepted as true, there is no documentation to guarantee that the PnPDeviceID will continue to contain the Serial Number."
				 With that being said there is also no guarentee that a usb will conatin a serial number in general.
				 For the time being all of my tests point to PnPDeviceID containing a serial number as true throughout the usb market.
				 Holding this property as true led to by far the simplest solution that hinged on two seprate joins to create the mass
				 of usb data; a join of drive numbers (the for loop directly above) and a join of serial numbers in the createlistitems() function.
				 Note that if this property for some drive should infact not
				 hold true then a program bug would occur in that this application would not be able to make a join and would see this drive as 
				 not having an assigned drive letter (therefore assuming then that this drive is not readable) and seprately would assume that this drive does not serial number.
				 Should a future developer find this corner case to be a critical error I lay out the plans of a solution for a reworked application here.
				 First this function should then be dissreguarded and replaced, it hinges on the above fact and would prove tough to rework. It is only called
				 in the refresh list function.
				 In its place the developer should fill the device list array myList by their own design. Critically the developer must find a way to map the 
				 drive letter with a usb device from the list of items, a task I could not find another way to do. If this is accheived it then becomes 
				 trival to create a file on the drive to find out if it is writable and then make a call to collectUsbName() to retreive the usb's true serial 
				 number.
				*/
				if (devList[devCount]->letter[0] != '!') { 
					int hashcount = 0, copybool = 0, serialnumcount = 0;

					for (int j = 0; j < 300; j++) {
						if (devList[devCount]->devicePath[j] == '#') {
							hashcount++;
						}
						if (devList[devCount]->devicePath[j] == '\0') {
							break;
						}
						if (copybool && devList[devCount]->devicePath[j] == '&') {
							devList[devCount]->serialnum[serialnumcount] = '\0';
							break;
						}
						if (copybool) {
							//in copy section copy untill &
							devList[devCount]->serialnum[serialnumcount] = devList[devCount]->devicePath[j];
							serialnumcount++;
						}

						if (hashcount == 2) {
							copybool = 1;
							hashcount++;
						}

					}

				}
				else {
					strcpy(devList[devCount]->serialnum, " ");
				}



				CloseHandle(hHCDev); //--------------------------------------------------------------------------------------------------------------------------

				devCount++;
			}

		}

		SetupDiDestroyDeviceInfoList(deviceInfo);
	}

	return devCount;
}


/*****************************************************************************

CollectUsbName()

*****************************************************************************/

//! Use a tv_items lparam to collect name and serial number
/*!
basically goes inside a tv_item's lparam and collects the specific name of usb and the serial number and 
places it in the usb_name and serial_num parameters.
Note that to get an auctual name a string descriptor array must be traversed untill the matching index is found.
*/


VOID CollectUsbName(PVOID info, char* usb_name, char* serial_num, int max_str_len) {
	//collect naming info

	ULONG nBytes = 0;
	
	if (NULL != info)
	{
		PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo = NULL;
		PSTRING_DESCRIPTOR_NODE                StringDescs = NULL;

		if ((*(PUSBDEVICEINFOTYPE)info) == DeviceInfo)
		{
			ConnectionInfo = ((PUSBDEVICEINFO)info)->ConnectionInfo;
			StringDescs = ((PUSBDEVICEINFO)info)->StringDescs;
			if (ConnectionInfo->DeviceDescriptor.iProduct)
			{
				//DisplayUSEnglishStringDescriptor(ConnectInfo->DeviceDescriptor.iProduct,
				//	StringDescs,
				//	info->DeviceInfoNode != NULL ? info->DeviceInfoNode->LatestDevicePowerState : PowerDeviceUnspecified);
				
				
				for (; StringDescs; StringDescs = StringDescs->Next)
				{
					if (StringDescs->DescriptorIndex == ConnectionInfo->DeviceDescriptor.iProduct )//&& StringDescs->LanguageID == 0x0409
					{

						memset(usb_name, 0, max_str_len);
						nBytes = WideCharToMultiByte(
							CP_ACP,     // CodePage
							WC_NO_BEST_FIT_CHARS,
							StringDescs->StringDescriptor->bString,
							(StringDescs->StringDescriptor->bLength - 2) / 2,
							usb_name,
							max_str_len,
							NULL,       // lpDefaultChar
							NULL);      // pUsedDefaultChar

					}

				}

			}
			ConnectionInfo = ((PUSBDEVICEINFO)info)->ConnectionInfo;
			StringDescs = ((PUSBDEVICEINFO)info)->StringDescs;
			if (ConnectionInfo->DeviceDescriptor.iSerialNumber) {

				//DisplayUSEnglishStringDescriptor(ConnectInfo->DeviceDescriptor.iProduct,
				//	StringDescs,
				//	info->DeviceInfoNode != NULL ? info->DeviceInfoNode->LatestDevicePowerState : PowerDeviceUnspecified);

				for (; StringDescs; StringDescs = StringDescs->Next)
				{
					if (StringDescs->DescriptorIndex == ConnectionInfo->DeviceDescriptor.iSerialNumber)
					{

						memset(serial_num, 0, max_str_len);
						nBytes = WideCharToMultiByte(
							CP_ACP,     // CodePage
							WC_NO_BEST_FIT_CHARS,
							StringDescs->StringDescriptor->bString,
							(StringDescs->StringDescriptor->bLength - 2) / 2,
							serial_num,
							max_str_len,
							NULL,       // lpDefaultChar
							NULL);      // pUsedDefaultChar

					}

				}

				
			}

		}

	}

}



/*****************************************************************************

CreateListItems()

*****************************************************************************/
//! takes a tree item and creates a list item
/*!
Takes a tree item, decides if it is a usb mass storage device, and if it is then proceeds to create
a listview item from the treeview item data and inserts the lvi into the lv control.
*/

VOID
CreateListItems(
	HWND      hTreeWnd,
	HTREEITEM hTreeItem,
	PVOID     pContext
)
{
	UNREFERENCED_PARAMETER(pContext);

	//query the treeview controller to get the item
	char* mystr = (char*)malloc(LISTNAMESIZE);
	TVITEM item;
	memset(&item, 0, sizeof(item));
	item.mask = TVIF_TEXT | TVIF_PARAM;
	item.hItem = hTreeItem;
	item.pszText = mystr;
	item.cchTextMax = LISTNAMESIZE;
	TreeView_GetItem( hTreeWnd, &item);
	
	if (strstr(item.pszText, "USB Mass Storage") != NULL) {
		printf("test: %s\n", item.pszText);
		//something says usb
		numUsbConnected++; //increment how many usb are connected




		char usb_name[LISTNAMESIZE], serial[256], final_name[LISTNAMESIZE];
		strcpy(usb_name, "");  
		strcpy(final_name, "");
		//collect the usb name and the usb serial number from the tree item
		CollectUsbName((PVOID)item.lParam, usb_name,serial, LISTNAMESIZE);

		if ((usb_name[0] == '\0') || usb_name[0] == ' ')
		{
			strcpy(usb_name, "NoNameUSB");  //set a default name
		}

		//now check our device list to find the matching serial number to get 
		//the appropraite drive

		for (int i = 0; i < DeviceCount; i++) {
			if (strcmpi(serial, myList[i]->serialnum) == 0)
				strcpy(final_name, myList[i]->letter);
		}
		strcat(final_name, ": ");
		strcat(final_name, usb_name);
		

		LVITEM lvI;
		memset(&lvI, 0, sizeof(lvI));
		lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;  //| LVIF_PARAM
		lvI.pszText = final_name;
		lvI.iImage = 1;
		lvI.lParam = (LPARAM) hTreeItem;
		ListView_InsertItem(ghListWnd, &lvI);
	}
	free(mystr);
	
}


/*****************************************************************************

RefreshTree()

*****************************************************************************/
//! Totally refreshes the tree and list items
/*!
Clears the edit window, makes a call to destroy the tree. Then it repopulates the tree in its entirety,
proceeds to generate the list by a call to RefreshList() and then finally update the status window.
Notably this function also makes a call to enumerate host controlers, which locks the prgram while making
system calls to collect the information of the usb's. In the future perhaps make this function call in a 
seprate thread
*/
VOID RefreshTree (VOID)
{
    CHAR  statusText[128];
    ULONG devicesConnected;

    // Clear the edit control
    //
    SetWindowText(ghEditWnd, "Please select item on the left");
	SetWindowText(ghSaveWnd, "This is the save window.\r\nSelect an item from the list above\r\nand choose save selected\r\nto save that usb's information.\r\nAlternatively choose save all to\r\nsave all usbs' informaiton");
    
	//disable the buttons
	Button_Enable(ghButton, FALSE);
	Button_Enable(ghButton2, FALSE);
	EnableMenuItem(ghMainMenu, ID_SAVE, MF_GRAYED);
	EnableMenuItem(ghMainMenu, ID_SAVEALL, MF_GRAYED);

	// Destroy the current contents of the TreeView
    //
    DestroyTree();

    // Create the root tree node
    //
    ghTreeRoot = AddLeaf(TVI_ROOT, 0, "My Computer", ComputerIcon);

	
    if (ghTreeRoot != NULL)
    {
        // Enumerate all USB buses and populate the tree
        //


		//I beleive that when a stall happens from a usb being difficult to read
		//it is infact this line that does it. I am not 100% sure, testing has not been done
		//but this is the function that is the first to make system calls to collect the usb data from the os.
		//If one wants to make sure that the controller never stalls then perhaps making this function excecute 
		//in another thread would be the best option. Perhaps puting some sort of loading circle?

        EnumerateHostControllers(ghTreeRoot, &devicesConnected); //----------------------------------------------------------------------------

        //
        // Expand all tree nodes
        //
        //WalkTree(ghTreeRoot, ExpandItem, NULL);

		//Generate the list.
		RefreshList();


        // Update Status Line with number of devices connected
        //
        memset(statusText, 0, sizeof(statusText));
        StringCchPrintf(statusText, sizeof(statusText),
#ifdef H264_SUPPORT
        "UVC Spec Version: %d.%d Version: %d.%d Devices Connected: %d ",
        UVC_SPEC_MAJOR_VERSION, UVC_SPEC_MINOR_VERSION, USBVIEW_MAJOR_VERSION, USBVIEW_MINOR_VERSION,
        numUsbConnected );
#else
        "Devices Connected: %d ",
        numUsbConnected);
#endif

        SetWindowText(ghStatusWnd, statusText);
		
    }
    else
    {
        OOPS();
    }
	
	
}

/*****************************************************************************

RefreshList()

*****************************************************************************/
//! initilizes list and calls CreateLIstItems()
/*! First this function creatates a single column with which to use for the list.
Then it calls WalkTree() with CreateLIstItems() as the recurive function to make the list
*/
void RefreshList(void) {
	

	LV_COLUMN   lvColumn;
	TCHAR       c_name[20] = "ACTIVE USB";

	//empty the list
	DestroyList();
	

	//get devices and stuff
	DeviceCount = GetUSBDevices(myList, 26);
	

	//initialize the column
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.cx = 260;
	lvColumn.pszText = c_name;
	ListView_InsertColumn(ghListWnd, 0, &lvColumn);

	ListView_SetItemCount(ghListWnd, 23); //--------------------------Max size of list is 23

	WalkTree(ghTreeRoot, CreateListItems, NULL);
}


/*****************************************************************************

AboutDlgProc()

*****************************************************************************/

//! handles messages for the about window
/*! this function is the message handler for the about popup window
*/

LRESULT CALLBACK
AboutDlgProc (
              HWND   hwnd,
              UINT   uMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HRESULT hr;
            char TextBuffer[TEXT_ITEM_LENGTH];
            HWND hItem;

            hItem = GetDlgItem(hwnd, IDC_VERSION);

            if (hItem != NULL)
            {
                hr = StringCbPrintfA(TextBuffer,
                                     sizeof(TextBuffer),
                                     "USBView version: %d.%d",
                                     USBVIEW_MAJOR_VERSION,
                                     USBVIEW_MINOR_VERSION);
                if (SUCCEEDED(hr))
                {
                    SetWindowText(hItem,TextBuffer);
                }
            }

            hItem = GetDlgItem(hwnd, IDC_UVCVERSION);

            if (hItem != NULL)
            {
                hr = StringCbPrintfA(TextBuffer,
                                     sizeof(TextBuffer),
                                     "USB Video Class Spec version: %d.%d",
                                     UVC_SPEC_MAJOR_VERSION,
                                     UVC_SPEC_MINOR_VERSION);
                if (SUCCEEDED(hr))
                {
                    SetWindowText(hItem,TextBuffer);
                }
            }
        }
        break;
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:

            EndDialog (hwnd, 0);
            break;
        }
        break;

    }

    return FALSE;
}


/*****************************************************************************

AddLeaf()

*****************************************************************************/
//! adds a leaf to the treeview structure
/*! basically creates a treeview item and inserts it into the treeview control
*/
HTREEITEM
AddLeaf (
         HTREEITEM hTreeParent,
         LPARAM    lParam,
         _In_ LPTSTR    lpszText,
         TREEICON  TreeIcon
         )
{
    TV_INSERTSTRUCT tvins;
    HTREEITEM       hti;

    memset(&tvins, 0, sizeof(tvins));

    // Set the parent item
    //
    tvins.hParent = hTreeParent;

    tvins.hInsertAfter = TVI_LAST;

    // pszText and lParam members are valid
    //
    tvins.item.mask = TVIF_TEXT | TVIF_PARAM;

    // Set the text of the item.
    //
	tvins.item.pszText = lpszText;

    // Set the user context item
    //
    tvins.item.lParam = lParam;

    // Add the item to the tree-view control.
    //
	
    hti = TreeView_InsertItem(ghTreeWnd, &tvins);

    // added
    tvins.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvins.item.hItem = hti;

    // Determine which icon to display for the device
    //
    switch (TreeIcon)
    {
        case ComputerIcon:
            tvins.item.iImage = giComputer;
            tvins.item.iSelectedImage = giComputer;
            break;

        case HubIcon:
            tvins.item.iImage = giHub;
            tvins.item.iSelectedImage = giHub;
            break;

        case NoDeviceIcon:
            tvins.item.iImage = giNoDevice;
            tvins.item.iSelectedImage = giNoDevice;
            break;

        case GoodDeviceIcon:
            tvins.item.iImage = giGoodDevice;
            tvins.item.iSelectedImage = giGoodDevice;
            break;

        case GoodSsDeviceIcon:
            tvins.item.iImage = giGoodSsDevice;
            tvins.item.iSelectedImage = giGoodSsDevice;
            break;

        case NoSsDeviceIcon:
            tvins.item.iImage = giNoSsDevice;
            tvins.item.iSelectedImage = giNoSsDevice;
            break;

        case BadDeviceIcon:
        default:
            tvins.item.iImage = giBadDevice;
            tvins.item.iSelectedImage = giBadDevice;
            break;
    }
    TreeView_SetItem(ghTreeWnd, &tvins.item);

	
	
    return hti;
}


/*****************************************************************************

WalkTreeTopDown()

*****************************************************************************/
//!traverses the tree top down recursively calling function
VOID
WalkTreeTopDown(
          _In_ HTREEITEM        hTreeItem,
          _In_ LPFNTREECALLBACK lpfnTreeCallback,
          _In_opt_ PVOID            pContext,
          _In_opt_ LPFNTREENOTIFYCALLBACK lpfnTreeNotifyCallback
          )
{
    if (hTreeItem)
    {
        HTREEITEM hTreeChild = TreeView_GetChild(ghTreeWnd, hTreeItem);
        HTREEITEM hTreeSibling = TreeView_GetNextSibling(ghTreeWnd, hTreeItem);

        //
        // Call the lpfnCallBack on the node itself.
        //
        (*lpfnTreeCallback)(ghTreeWnd, hTreeItem, pContext,NULL);

        //
        // Recursively call WalkTree on the node's first child.
        //

        if (hTreeChild)
        {
            WalkTreeTopDown(hTreeChild,
                    lpfnTreeCallback,
                    pContext,
                    lpfnTreeNotifyCallback);
        }

        //
        // Recursively call WalkTree on the node's first sibling.
        //
        if (hTreeSibling)
        {
            WalkTreeTopDown(hTreeSibling,
                    lpfnTreeCallback,
                    pContext,
                    lpfnTreeNotifyCallback);
        }
        else
        {
            // If there are no more siblings, we have reached the end of
            // list of child nodes. Call notify function
            if (lpfnTreeNotifyCallback != NULL)
            {
                (*lpfnTreeNotifyCallback)(pContext);
            }
        }
    }
}

/*****************************************************************************

WalkTree()

*****************************************************************************/

//!traverses the tree recursively calling function

VOID
WalkTree (
          _In_ HTREEITEM        hTreeItem,
          _In_ LPFNTREECALLBACK lpfnTreeCallback,
          _In_opt_ PVOID            pContext
          )
{
    if (hTreeItem)
    {
        // Recursively call WalkTree on the node's first child.
        //
        WalkTree(TreeView_GetChild(ghTreeWnd, hTreeItem),
            lpfnTreeCallback,
            pContext);

        //
        // Call the lpfnCallBack on the node itself.
        //
        (*lpfnTreeCallback)(ghTreeWnd, hTreeItem, pContext);

        //
        //
        // Recursively call WalkTree on the node's first sibling.
        //
        WalkTree(TreeView_GetNextSibling(ghTreeWnd, hTreeItem),
            lpfnTreeCallback,
            pContext);
    }
}

/*****************************************************************************

ExpandItem()

*****************************************************************************/

VOID
ExpandItem (
            HWND      hTreeWnd,
            HTREEITEM hTreeItem,
            PVOID     pContext
            )
{
    //
    // Make this node visible.
    //
    UNREFERENCED_PARAMETER(pContext);

    TreeView_Expand(hTreeWnd, hTreeItem, TVE_EXPAND);
}

/*****************************************************************************

SaveAllInformationAsXML()

Saves the entire USB tree as an XML file
*****************************************************************************/
//! not used. Kept for legacy
HRESULT
SaveAllInformationAsXml(
        LPTSTR lpstrTextFileName,
        DWORD dwCreationDisposition
        )
{
    HRESULT hr = S_OK;

    if (ghTreeRoot == NULL)
    {
        // If tree has not been populated yet, try a refresh
        RefreshTree();
    }
    if (ghTreeRoot)
    {
        WalkTreeTopDown(ghTreeRoot, AddItemInformationToXmlView, NULL, XmlNotifyEndOfNodeList);

        hr = SaveXml(lpstrTextFileName, dwCreationDisposition);
    }
    else
    {
        hr = E_FAIL;
        OOPS();
    }
    ResetTextBuffer();
    return hr;
}

//*****************************************************************************
//
//  AddItemInformationToXmlView
//
//  hTreeItem - Handle of selected TreeView item for which information should
//  be added to the XML View
//
//*****************************************************************************

//! not used kept for legacy
VOID
AddItemInformationToXmlView(
        HWND hTreeWnd,
        HTREEITEM hTreeItem,
        PVOID pContext
        )
{
    TV_ITEM tvi;
    PVOID   info;
    PCHAR tviName = NULL;

    UNREFERENCED_PARAMETER(pContext);

#ifdef H264_SUPPORT
    ResetErrorCounts();
#endif

    tviName = (PCHAR) ALLOC(256);

    if (NULL == tviName)
    {
        return;
    }

    //
    // Get the name of the TreeView item, along with the a pointer to the
    // info we stored about the item in the item's lParam.
    //

    tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
    tvi.hItem = hTreeItem;
    tvi.pszText = (LPSTR) tviName;
    tvi.cchTextMax = 256;

    TreeView_GetItem(hTreeWnd,
            &tvi);

    info = (PVOID)tvi.lParam;

    if (NULL != info)
    {
        //
        // Add Item to XML object
        //
        switch (*(PUSBDEVICEINFOTYPE)info)
        {
            case HostControllerInfo:
                XmlAddHostController(tviName, (PUSBHOSTCONTROLLERINFO) info);
                break;

            case RootHubInfo:
                XmlAddRootHub(tviName, (PUSBROOTHUBINFO) info);
                break;

            case ExternalHubInfo:
                XmlAddExternalHub(tviName, (PUSBEXTERNALHUBINFO) info);
                break;

            case DeviceInfo:
                XmlAddUsbDevice(tviName, (PUSBDEVICEINFO) info);
                break;
        }

    }
    return;
}

/*****************************************************************************

DisplayLastError()

*****************************************************************************/

DWORD
DisplayLastError(
          _Inout_updates_bytes_(count) char *szString,
          int count)
{
    LPVOID lpMsgBuf;

    // get the last error code
    DWORD dwError = GetLastError();

    // get the system message for this error code
    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL ))
    {
        StringCchPrintf(szString, count, "Error: %s", (LPTSTR)lpMsgBuf );
    }

    // Free the local buffer
    LocalFree( lpMsgBuf );

    // return the error
    return dwError;
}

#if DBG

/*****************************************************************************

Oops()

*****************************************************************************/

VOID
Oops
(
    _In_ PCHAR  File,
    ULONG       Line
 )
{
    char szBuf[1024];
    LPTSTR lpMsgBuf;
    DWORD dwGLE = GetLastError();

    memset(szBuf, 0, sizeof(szBuf));

    // get the system message for this error code
    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwGLE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL))
    {
        StringCchPrintf(szBuf, sizeof(szBuf),
            "File: %s, Line %d\r\nGetLastError 0x%x %u %s\n",
            File, Line, dwGLE, dwGLE, lpMsgBuf);
    }
    else
    {
        StringCchPrintf(szBuf, sizeof(szBuf),
            "File: %s, Line %d\r\nGetLastError 0x%x %u\r\n",
            File, Line, dwGLE, dwGLE);
    }
    OutputDebugString(szBuf);

    // Free the system allocated local buffer
    LocalFree(lpMsgBuf);

    return;
}

#endif
