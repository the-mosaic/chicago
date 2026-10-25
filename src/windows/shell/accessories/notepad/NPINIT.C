//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//	PROGRAM:	npinit.c
//
//	PURPOSE:	Demonstrates Windows initialization routines
//
//	PLATFORMS:	Chicago
//
//	FUNCTIONS:	
//
//	Private Functions
//		PoundToNull 		- Replaces # to NULL
//		InitStrings 		- Get all resource strings
//		ProcessSetupOption	- Handles special command line option /.SETUP
//		ProcessShellOptions - Command line options support
//		NPInit				- Standard application initialization process
//		NPRegister			- Registers Window class
//		InitCountry 		- Gets Country information from win.ini and sets state
//
//	SPECIAL INSTRUCTIONS:
//		Routines are in a separate segment.
//

#include "notepad.h"
#include <shellapi.h>
#include <string.h>

CHAR chPageText[6][PT_LEN];    /* Strings to hold PageSetup items.        */
CHAR szDec[5];                 /* Placeholder for intl decimal character. */
CHAR chIntl[] = "intl";
HMENU hDragMenu;               /* New file drag/drop menu support.      */
extern HMENU hSysMenuSetup;             /* Save Away for disabled Minimize */


static int NPRegister(
    HANDLE hInstance);

void FAR PASCAL PoundToNull(LPSTR str)
{

   while(*str){
      if(*str == '#')
        *str = 0x00;
      str++;
   }
}

/* InitStrings - Get all text strings from resource file */
BOOL InitStrings(HANDLE hInstance)
    {
    HANDLE  hStrings;
    CHAR    *pch;
    INT     cchRemaining=CCHSTRINGSMAX,
            ids, cch;

    hStrings = LocalAlloc(LPTR, cchRemaining);
    pch = (CHAR *)hStrings;

    if (!pch)
        return FALSE;

    for (ids = 0; ids < CSTRINGS-6; ids++)
        {
        cch = 1+LoadString(hInstance, (WORD)(*rgsz[ids]), (LPSTR)pch, cchRemaining);
        *rgsz[ids] = pch;
        pch += cch;
        cchRemaining -= cch;
        }

    /* Get decimal character. */

    for (ids=0; ids <6; ids++)
        LoadString(hInstance, (WORD)(ids+IDS_HEADER), chPageText[ids], PT_LEN);


#if !defined(NTBUG)
    //
    // this code assumes a shrinking object stays in place! currently nt
    // *always* moves objects when reallocing. 
    //
    LocalReAlloc(hStrings, CCHSTRINGSMAX - cchRemaining, LPTR);
#endif

    wMerge = (WORD)*szMerge;
    return TRUE ;
    }


// if /.SETUP option exists in the command line process it.
BOOL ProcessSetupOption(
        LPSTR lpszCmdLine)
{
        /* Search for /.SETUP in the command line */
        if (*lpszCmdLine == '/' && *(lpszCmdLine+1) == '.' &&
        *(lpszCmdLine+2) == 'S' && *(lpszCmdLine+3) == 'E' &&
        *(lpszCmdLine+4) == 'T' && *(lpszCmdLine+5) == 'U' &&
        *(lpszCmdLine+6) == 'P')
    {
        fRunBySetup = TRUE;
        /* Save system menu handle for INITMENUPOPUP message */
        hSysMenuSetup =GetSystemMenu(hwndNP, FALSE);
        /* Allow exit on ^C, ^D and ^Z                      */
        /* Note that LoadAccelerators must be called before */
        /* TranslateAccelerator is called, true here        */
        hAccel = LoadAccelerators(hInstanceNP, "SlipUpAcc");
        lpszCmdLine += 7;
    }
    else
        return FALSE;

    /* skip blanks again to get to filename */
    while (*lpszCmdLine == ' ' || *lpszCmdLine == '\t')
       lpszCmdLine++;

    if (*lpszCmdLine)
    {
        /* Get the filename. */
        lstrcpy((LPSTR)szFileName, lpszCmdLine);
        AddExt(szFileName);
        fp=OpenFile((LPSTR)szFileName, &rgbOpen, OF_READ);

        if (fp == -1)
        {
            if (AlertBox(hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                fp=OpenFile((LPSTR)szFileName, &rgbOpen, OF_CREATE);
                }

        if (fp == -1)
            return FALSE;
        LoadFile(szFileName);
    }

    return TRUE;
}

/*
 * ProcessShellOptions(lpszCmdLine)
 *
 * If the command line has any options specified by the shell
 * process them.
 * Currently /P <filename> - prints the given file
 */
BOOL ProcessShellOptions(
    LPSTR lpszCmdLine,
    int cmdShow)
{
    INT iError;
    CHAR *szMsg;

        /* look for /P */
        if (*lpszCmdLine != '/' || *(lpszCmdLine+1) != 'P')
                return FALSE;

        lpszCmdLine += 2;

        /* skip blanks */
    while (*lpszCmdLine == ' ' || *lpszCmdLine == '\t')
        lpszCmdLine++;

        if (!*lpszCmdLine)
                return FALSE;

    ShowWindow(hwndNP, cmdShow);

    /* Get the filename. */
    lstrcpy((LPSTR)szFileName, lpszCmdLine);
    AddExt(szFileName);

    fp=OpenFile((LPSTR)szFileName, (LPOFSTRUCT)&rgbOpen, OF_READ);

    if (fp == -1)
    {
        AlertBox(hwndNP, szNN, szFileOpenFail, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }

    /* load the file into the edit control */
    LoadFile(szFileName);

    /* print the file */
    if (((iError = NpPrint()) < 0) && (iError != SP_USERABORT))
    {
        if (iError == SP_OUTOFDISK)
            szMsg = szNEDSTP;
        else if (iError == SP_OUTOFMEMORY)
            szMsg = szNEMTP;
        else
            szMsg = szCP;
        AlertBox(hwndNP, szNN, szMsg, fUntitled ? szUntitled : szFileName,  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }

    return TRUE;
}

/* One time initialization */
INT FAR NPInit(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPSTR lpszCmdLine,
    INT cmdShow)
{
    CHAR   szWindowText[256];
    CHAR * pszFilterSpec = szFilterSpec;  /* temp. var. for creating filter text */

#if !defined(WIN32)
  /* The xchg instruction puts major version in hibyte, minor in lobyte  */
  /* Used to check if IsDBCSLeadByte is supported.                       */
    GetVersion();
    _asm {
      xchg  ah, al
      mov wWinVer, ax
      }
#endif

    /* Go load strings */
    if (!InitStrings(hInstance))
        return FALSE;

    /* Turn on the hourglass. */
    hWaitCursor = LoadCursor(NULL, IDC_WAIT);

    /* Load accelerators. */
    hAccel = LoadAccelerators(hInstance, (LPSTR)"MainAcc");
    if (!hWaitCursor || !hAccel)
        return FALSE;

    if (!hPrevInstance)
    {
        if (!NPRegister(hInstance))
            return (FALSE);
    }

    hInstanceNP = hInstance;
    szDec[0] = '.';    /* The initial decimal separator in the RC strings */
    InitCountry();

    hwndNP = CreateWindow(szNotepad, szWindowText,
                          WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                          WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                          WS_VSCROLL    | WS_HSCROLL,
                          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                          (HWND)NULL, (HMENU)NULL,
                          hInstance, NULL);
    if (!hwndNP)
        return FALSE;

        /* File Drag Drop support added 03/26/91 - prototype only. w-dougw */
        /* All processing of drag/drop files is done the under WM_DROPFILES*/
        /* message.                                                        */
        DragAcceptFiles(hwndNP,TRUE); /* Proccess dragged and dropped files. */

    /* init. fields of PRINTDLG struct.. */
    /* Inserted here since command line print statements are valid. */
    PD.lStructSize   = sizeof(PRINTDLG);          /* Don't hard code it */
    PD.hwndOwner     = hwndNP;
    PD.hDevMode      = NULL;
    PD.hDevNames     = NULL;
    PD.hDC           = NULL;


    /* ES_NOHIDESEL added so selection can be seen when bringing up
     * the Find and Find/Replace dialogs.
     */
#ifndef TEST
    if (!(hwndEdit = CreateWindow((LPSTR)"Edit", (LPSTR) "",
        WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE | ES_NOHIDESEL,
        0, 0, 600, 400,
        hwndNP, (HMENU)ID_EDIT, hInstance, (LPSTR)NULL)))
        return FALSE;
#else
    if (!(hwndEdit = CreateWindow((LPSTR)"Edit", (LPSTR) "",
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE | ES_NOHIDESEL,
        0, 0, 600, 400,
        hwndNP, (HMENU)ID_EDIT, hInstance, (LPSTR)NULL)))
        return FALSE;
#endif

    SendMessage(hwndEdit, WM_SETFONT,
      (WPARAM)(hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT)),
      (LONG) FALSE);

    NpWinIniChange();

    szSearch[0] = 0;
#ifdef WIN32S
    /*
     * Win32s does not allow local memory handles to be passed to Win3.1.
     * So, hEdit is used for transferring text to and from the edit control.
     * Before reading text into it, it must be reallocated to a proper size.
     */
    hEdit = LocalAlloc(LHND, 1);
#else
    hEdit = (HANDLE)SendMessage(hwndEdit, EM_GETHANDLE, (WPARAM)0, 0L);
#endif

    /* limit text to 64k for safety's sake. */
#ifndef WIN32
    PostMessage(hwndEdit, EM_LIMITTEXT, (WORD)CCHNPMAX, 0L);
#endif

    SetTitle(szUntitled);

    /* Convert input filenames to upper case. */
    AnsiUpper(lpszCmdLine);

    /* check for /.SETUP option first.
       if /.SETUP absent, check for SHELL options /P
       Whenever a SHELL option is processed, post a WM_CLOSE msg.
       */
    if (ProcessSetupOption(lpszCmdLine))
        ;
    else if (ProcessShellOptions(lpszCmdLine, cmdShow))
    {
        PostMessage(hwndNP, WM_CLOSE, 0, 0L);
        return TRUE;
    }
    else if (*lpszCmdLine)
    {
        /* Get the filename. */
        lstrcpy((LPSTR)szFileName, lpszCmdLine);
        AddExt(szFileName);
        fp=OpenFile((LPSTR)szFileName, &rgbOpen, OF_READ);

        if (fp == -1)
        {
          if (AlertBox(hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
              fp=OpenFile((LPSTR)szFileName, &rgbOpen, OF_CREATE);
        }

        if (fp != -1)
            LoadFile(szFileName);
}


    /* Don't offer a minimize button if we're being run by Slipup. */
    if (fRunBySetup)
        SetWindowLong(hwndNP, GWL_STYLE,
                      WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                      WS_THICKFRAME |                  WS_MAXIMIZEBOX |
                      WS_VSCROLL    | WS_HSCROLL);

    ShowWindow(hwndNP, cmdShow);

     /* construct default filter string in the required format for
      * the new FileOpen and FileSaveAs dialogs
      */
     lstrcpy((LPSTR)szFilterSpec, (LPSTR)szFilterText);
     pszFilterSpec += lstrlen ((LPSTR)szFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)szMyExt + 1);
     pszFilterSpec += lstrlen((LPSTR)pszFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)szAllFiles);
     pszFilterSpec += lstrlen ((LPSTR)pszFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)"*.*");
     pszFilterSpec += lstrlen((LPSTR)pszFilterSpec) + 1;
     *pszFilterSpec = '\0';
     *szCustFilterSpec = '\0';
     lstrcpy(szCustFilterSpec + 1, szMyExt + 1);

     /* init. some fields of the OPENFILENAME struct used by fileopen and
      * filesaveas
      */
     OFN.lStructSize       = sizeof(OPENFILENAME);
     OFN.hwndOwner         = hwndNP;
     OFN.lpstrFileTitle    = 0;
     OFN.nMaxCustFilter    = CCHFILTERMAX;
     OFN.nFilterIndex      = 1;
     OFN.nMaxFile          = CCHFILENAMEMAX;
     OFN.lpfnHook          = NULL;
     OFN.Flags             = 0L;/* for now, since there's no readonly support */

     /* init.fields of the FINDREPLACE struct used by FindText() */
     FR.lStructSize        = sizeof(FINDREPLACE);       /* Don't hard code it */
     FR.hwndOwner          = hwndNP;
     FR.Flags              = FR_DOWN | FR_HIDEWHOLEWORD;           /* default */
     FR.lpstrReplaceWith   = (LPSTR)NULL;  /* not used by FindText() */
     FR.wReplaceWithLen    = 0;     /* not used by FindText() */
     FR.lpfnHook           = NULL;

     /* determine the message number to be used for communication with
      * Find dialog
      */
     if (!(wFRMsg = RegisterWindowMessage ((LPSTR)FINDMSGSTRING)))
          return FALSE;
     if (!(wHlpMsg = RegisterWindowMessage ((LPSTR)HELPMSGSTRING)))
          return FALSE;

     /* Force a scroll to current selection (which could be at eof if
        we loaded a log file.) */
     SendMessage( hwndEdit, EM_SETSEL,
       GET_EM_SETSEL_MPS(0,SendMessage(hwndEdit, EM_GETSEL, 0, 0L)) ) ;
#ifdef JAPAN

    /* Edit Control Tune Up */
    {
        extern FARPROC lpEditSubClassProc;
        extern FARPROC lpEditClassProc;
        extern long far pascal EditSubClassProc();

        /* Sub Classing. See npmisc.c */
        lpEditSubClassProc = MakeProcInstance((FARPROC)EditSubClassProc,hInstance);
        lpEditClassProc = (FARPROC)GetWindowLong(hwndEdit,GWL_WNDPROC);
        SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG)lpEditSubClassProc);
    }

#endif
     return TRUE;

}

/* ** Notepad class registration proc */
BOOL NPRegister(
   HANDLE hInstance)
{
    PWNDCLASS pNPClass;

#if !defined (WIN32S) && !defined (WIN32)

    pNPClass=(PWNDCLASS)LocalAlloc(LPTR, sizeof(WNDCLASS));

/* If Pen Windows is running, make the background cursor an
 * arrow instead of the edit control ibeam.  This way the user will know
 * where they can use the pen for writing vs. what will be considered a
 * mouse action.
 */
    pNPClass->hCursor      = LoadCursor(NULL, GetSystemMetrics(SM_PENWINDOWS)
                                              ? IDC_ARROW : IDC_IBEAM);
    pNPClass->hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
    //pNPClass->lpszMenuName  = MAKEINTRESOURCE(ID_MENUBAR);
    pNPClass->lpszMenuName  = ID_MENUBAR;
    pNPClass->hInstance     = hInstance;
    pNPClass->lpszClassName = (LPSTR)szNotepad;
    pNPClass->lpfnWndProc   = (WNDPROC)NPWndProc;
    pNPClass->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    //pNPClass->style	      = CS_BYTEALIGNCLIENT;
    pNPClass->style	    = 0L;

#else

    WNDCLASS wc;

    _fmemset ( &wc, 0, sizeof (WNDCLASS ) );

    wc.hCursor	    = LoadCursor(NULL, GetSystemMetrics(SM_PENWINDOWS)
                                              ? IDC_ARROW : IDC_IBEAM);
    wc.hIcon	     = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
    wc.lpszMenuName  = MAKEINTRESOURCE(ID_MENUBAR);
    wc.hInstance     = hInstance;
    wc.lpszClassName = (LPSTR)szNotepad;
    wc.lpfnWndProc   = (WNDPROC)NPWndProc;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.style	     = CS_BYTEALIGNCLIENT;

    pNPClass = &wc;

#endif


    if (!RegisterClass((LPWNDCLASS)pNPClass))
        return(FALSE);

#if !defined(WIN32S) && !defined (WIN32)
	LocalFree((HANDLE)pNPClass);
#endif
    return(TRUE);
}

/* Get country info from win.ini file, and initialize global vars
      that determine format of date/time string. */
void FAR InitCountry(
    void)
{
#define MAX_DATE_FORMATS        8
    static DATEFORMAT DateFormats[MAX_DATE_FORMATS] =
    {{{"M/d/yy",    "d/M/yy",    "yy/M/d"    }, 0},
     {{"M/dd/yy",   "dd/M/yy",   "yy/M/dd"   }, LEADZERODAY},
     {{"MM/d/yy",   "d/MM/yy",   "yy/MM/d"   }, LEADZEROMONTH},
     {{"MM/dd/yy",  "dd/MM/yy",  "yy/MM/dd"  }, LEADZERODAY | LEADZEROMONTH},
     {{"M/d/yyyy",  "d/M/yyyy",  "yyyy/M/d"  }, CENTURY},
     {{"M/dd/yyyy", "dd/M/yyyy", "yyyy/M/dd" }, LEADZERODAY | CENTURY},
     {{"MM/d/yyyy", "d/MM/yyyy", "yyyy/MM/d" }, LEADZEROMONTH | CENTURY},
     {{"MM/dd/yyyy","dd/MM/yyyy","yyyy/MM/dd"}, LEADZERODAY | LEADZEROMONTH | CENTURY}
    };
    int  i, id;
    char szT[2];
    char szDateFormat[MAX_FORMAT];
    char *p;

    extern TIME Time;
    extern DATE Date;

    /* date format, 0 => MDY, 1=> DMY, 2=> YMD */
    Date.Order=GetProfileInt(chIntl, "iDate", 0);

    /* get char for date seperator */
    GetProfileString(chIntl, "sDate", "/", szT,2);
    Date.chSep = *szT;

    /* short date format, determines LeadZeroDay, LeadZeroMonth, Century */
    GetProfileString(chIntl, "sShortDate", "d/M/yy", Date.szShortFormat, 11);
    /* make private copy and replace current date seperator with our own('/')*/
    lstrcpy(szDateFormat, Date.szShortFormat);
    if (Date.chSep != '/')
    {
        p = szDateFormat;
        while (*p)
        {
            if (*p != 'M' && *p != 'd' && *p != 'y')
                *p = '/';
            p++;
        }
    }

    Date.Flags = 0;    /* assume "d/M/yy" */
    /* match szDateFormat and pick up the corresponding flags */
    for (i = 0; i < MAX_DATE_FORMATS; i++)
    {
        if (lstrcmpi(DateFormats[i].szFormats[Date.Order], szDateFormat) == 0)
        {
            Date.Flags = DateFormats[i].Flags;
            break;
        }
    }

    /* string for AM */
    GetProfileString(chIntl, "s1159", "AM", Time.sz1159, 6);
    /* string for PM */
    GetProfileString(chIntl, "s2359", "PM", Time.sz2359, 6);
    /* set flag for 24 hr time format */
    Time.f24Hr = GetProfileInt(chIntl, "iTime", 0);
    /* time seperator */
    GetProfileString(chIntl, "sTime", ":", szT,2);
    Time.chSep = *szT;
    /* leading zero for hour */
    Time.fLeadZeroHour = GetProfileInt(chIntl, "iTLzero", 0);

/* Replace the previously used decimal separator, not
 * always a period.
 */
    szT[0] = szDec[0];
    GetProfileString(chIntl, "sDecimal", ".", szDec, 2);
    /* Scan for . and replace with intl decimal */
    for (id=2; id<6; id++)
    {
        for (i=0; i < lstrlen(chPageText[id]); i++)
            if (chPageText[id][i] == szT[0])
                chPageText[id][i] = szDec[0];
    }
}
