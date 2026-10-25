//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
//PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//	MODULE:		Notepad.
//
//	PURPOSE:	Demonstates how to use MultiLine edit windows, common
//				dialogs.
//
//	PLATFORMS:	Chicago
//
//	FUNCTIONS:	
//
//		WinMain()			- calls initialization function, processes message loop
//		InitApplication()	- Initializes window data nd registers window
//		InitInstance()		- saves instance handle and creates main window
//		MainWindProc()		- Processes messages
//		About() 			- Process menssages for "About" dialog box
//		FreeGlobal ()		- Free Global Memory
//		NPSize ()			- Window Sizing
//		NPCommand			- Support for WM_COMMAND message
//		FileDragOpen		- Open file for Drag and Drop functionality
//		doDrop				- Proccess file drop/drag options
//		CheckSave			- Check for dirty notepad and prompt for save
//		NPWndProc			- Window Class Procedure
//		SetTitle			- Sets window caption text
//		PFileInPath 		- Fully qualify file path
//		NpResetMenu 		- Set menu item status ( active / not active )
//		NpWinIniChange		- Procedure for handle WM_WININICHANGE message
//		MergeStrings		- Merge two strings after spec string
//		AlertBox			- Common message box handler
//
//	SPECIAL INSTRUCTIONS: Support for EM_GETHANDLE and EM_SETHANDLE are
//						  not supported in Win32.	Make sure WIN32 is
//						  is defined.  We replacethe EM_GETHANDLE/EM_SETHANDLE
//						  functionality with WM_SETTEXT.
//

#include "notepad.h"
#include <shellapi.h>
#include <cderr.h>

#if defined(WIN32)
#define LockData(h) (h)      // LockData NOP on NT
#define UnlockData(h) (h)    // UnlockData NOP on NT
#endif

WORD     wMerge;
HWND     hwndNP = 0;                 /* handle to notepad parent window   */
HWND     hwndEdit = 0;               /* handle to main text control item  */
HANDLE   hEdit;                      /* Handle to storage for edit item   */
HWND     hDlgFind = NULL;           /* handle to modeless FindText window */
HANDLE   hWaitCursor;                /* handle to hour glass cursor       */
HANDLE   hInstanceNP;                /* Module instance handle            */
HANDLE   hFont;                      /* handle to Notepad font            */
CHAR     szFileName[CCHFILENAMEMAX]; /* Current notepad filename          */
CHAR     szSearch[CCHKEYMAX];           /* Search string                     */

BOOL     fUntitled = TRUE;           /* TRUE iff notepad has no title     */
INT      cchLineMac = CCHLINEMAX;    /* Max number of characters per line */

HMENU hSysMenuSetup;                 /* Save Away for disabled Minimize   */

WORD     wEmSetHandle = 0;           /* Is EM_SETHANDLE in process?       */
WORD     wWinVer;                    /* Return from GetVersion()          */
HANDLE   hAccel;                     /* Handle to accelerator table       */
BOOL     fRunBySetup = FALSE;        /* Did SlipUp WinExec us??           */
BOOL     fWrap = 0;                  /* Flag for word wrap                */
CHAR     szNotepad[] = "Notepad";    /* Name of notepad window class      */
CHAR     szExtTemp[11]="\\*.TXT";

BOOL fInSaveAsDlg=FALSE;

/* variables for the new File/Open, File/Saveas,Find Text and Print dialogs */
OPENFILENAME OFN;		                /* passed to the File Open/save APIs */
CHAR szFilterSpec[CCHFILTERMAX];     /* default filter spec. for above	  */
CHAR szCustFilterSpec[CCHFILTERMAX]; /* buffer for custom filters created */
CHAR szCurDir[CCHFILENAMEMAX];	    /* last. dir. for which listing made */
FINDREPLACE FR; 		     /* Passed to FindText()		  */
PRINTDLG    PD; 		     /* passed to PrintDlg()		  */
UINT wFRMsg;			     /* message used in communicating	  */
	         			     /* with Find/Replace dialog	  */
DWORD dwCurrentSelection = 0L;      /* WM_ACTIVATEAPP selection pos */
UINT wHlpMsg;			     /* message used in invoking help	  */

/* Strings loaded from resource file passed to LoadString at initialization time */
CHAR *szDiskError =(CHAR *)IDS_DISKERROR;  /* Can't open File, check disk  */
CHAR *szFNF       =(CHAR *)IDS_FNF;        /* File not found               */
CHAR *szFAE       =(CHAR *)IDS_FAE;        /* File already exists          */
CHAR *szSCBC      =(CHAR *)IDS_SCBC;       /* Save changes before closing? */
CHAR *szUntitled  =(CHAR *)IDS_UNTITLED;   /* untitled                     */
CHAR *szCFS       =(CHAR *)IDS_CFS;        /* Can't find string            */
CHAR *szErrSpace  =(CHAR *)IDS_ERRSPACE;   /* Memory space exhausted       */
CHAR *szNpTitle   =(CHAR *)IDS_NOTEPAD;    /* Notepad -                    */
CHAR *szFTL       =(CHAR *)IDS_FTL;        /* File too large for notepad   */
CHAR *szNN        =(CHAR *)IDS_NN;         /* Notepad Note!                */
CHAR *szPE        =(CHAR *)IDS_PASTEERR;   /* Paste Error                  */
CHAR *szWE        =(CHAR *)IDS_WRITEERR;   /* Write Error                  */
CHAR *szINF       =(CHAR *)IDS_INF;        /* Not a valid notepad file     */
CHAR *szEFD       =(CHAR *)IDS_EFD;        /* Empty file will be deleted:  */
CHAR *szCSEF      =(CHAR *)IDS_CSEF;       /* Can not save empty file      */
CHAR *szCP        =(CHAR *)IDS_CANTPRINT;  /* Can't print                  */
CHAR *szNVF       =(CHAR *)IDS_NVF;        /* Not a valid filename.        */
CHAR *szNVF2      =(CHAR *)IDS_NVF2;
CHAR *szNEDSTP    =(CHAR *)IDS_NEDSTP;     /* Not enough disk space/print. */
CHAR *szNEMTP     =(CHAR *)IDS_NEMTP;      /* Not enough memory to print.  */
CHAR *szCREATEERR =(CHAR *)IDS_CREATEERR;  /* cannot create file           */
CHAR *szNoWW      =(CHAR *)IDS_NOWW;       /* Too much text to word wrap   */
CHAR *szMerge     =(CHAR *)IDS_MERGE1;     /* search string for merge      */
CHAR *szMyExt     =(CHAR *)IDS_EXT;        /* Default extension for files  */
CHAR *szHelpFile  =(CHAR *)IDS_HELPFILE;   /* Name of helpfile.            */
CHAR *szBadMarg   =(CHAR *)IDS_BADMARG;    /* Bad margins.                 */
CHAR *szFileOpenFail =(CHAR *)IDS_FILEOPENFAIL;  /* Can't open File */
CHAR *szFilterText=(CHAR *)IDS_FILTERTEXT; /* File/Open Filter spec. string */
CHAR *szAllFiles  =(CHAR *)IDS_ALLFILES;   /* File/Open Filter spec. string */
CHAR *szOpenCaption = (CHAR *)IDS_OPENCAPTION; /* caption for File/Open dlg */
CHAR *szSaveCaption = (CHAR *)IDS_SAVECAPTION; /* caption for File/Save dlg */
CHAR *szCannotQuit = (char *)IDS_CANNOTQUIT;  /* cannot quit during a WM_QUERYENDSESSION */
CHAR *szLoadDrvFail = (CHAR *)IDS_LOADDRVFAIL;  /* LOADDRVFAIL from PrintDlg */

CHAR **rgsz[CSTRINGS-6] = {&szDiskError, &szFNF, &szFAE,
	&szSCBC, &szUntitled, &szErrSpace, &szCFS, &szNpTitle, &szFTL, &szNN,
	&szPE, &szWE, &szINF, &szEFD, &szCSEF, &szCP, &szNVF, &szNVF2,
        &szNEDSTP, &szNEMTP, &szCREATEERR, &szNoWW, &szMerge, &szMyExt,
	&szHelpFile, &szBadMarg, &szFileOpenFail, &szFilterText, &szAllFiles,
        &szOpenCaption, &szSaveCaption, &szCannotQuit, &szLoadDrvFail};


OFSTRUCT rgbOpen;	   /* buffer used by OpenFile() */

INT fp; 	    /* file pointer */

static CHAR  szPath[128];

void FileDragOpen(void);


VOID NpResetMenu(VOID);


/* FreeGlobal, frees  all global memory allocated. */
void NEAR PASCAL FreeGlobal()
{
	if(PD.hDevMode){
		GlobalFree(PD.hDevMode);
	}
	if(PD.hDevNames){
		GlobalFree(PD.hDevNames);
	}
}

/* Standard window size proc */
void NPSize(
    int cxNew,
    int cyNew)
{
    /* Invalidate the edit control window so that it is redrawn with the new
     * margins. Needed when comming up from iconic and when doing word wrap so
     * the new margins are accounted for.
     */
    InvalidateRect(hwndEdit, (LPRECT)NULL, TRUE);
    MoveWindow(hwndEdit, CXMARGIN, CYMARGIN, cxNew-CXMARGIN-CXMARGIN+1,
					     cyNew-CYMARGIN-CYMARGIN, TRUE);
    }

/* ** Notepad command proc - called whenever notepad gets WM_COMMAND
      message.	wParam passed as cmd */
INT NPCommand(
    HWND     hwnd,
    WPARAM   wParam,
    LONG     lParam )
  {
    HWND     hwndFocus;
    INT      iErrPrint;
    LONG     lSel;
    CHAR     *pchT;
    WORD     cch;
    CHAR     szNewName[CCHFILENAMEMAX] = "";	 /* New file name */
    FARPROC lpfn;

    cch;
    switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case M_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case M_NEW:
            New(TRUE);
            break;

        case M_OPEN:
            if (CheckSave(FALSE))
                {
		            /* set up the variable fields of the OPENFILENAME struct.
		            * (the constant fields have been set in NPInit()
		            */
	        OFN.lpstrFile	      = szNewName;
                lstrcpy (szNewName, szCustFilterSpec + 1); /* set default selection */
	        OFN.lpstrInitialDir   = szCurDir;
	        OFN.lpstrTitle	      = szOpenCaption;

	        /* ALL non-zero long pointers must be defined immediately
	         * before the call, as the DS might move otherwise.
	         */
                OFN.lpstrFilter	      = (LPSTR)szFilterSpec;
                OFN.lpstrCustomFilter = (LPSTR)szCustFilterSpec;
                OFN.lpstrDefExt       = (LPSTR)szMyExt + 3;  /* points to TXT */
                /* Added OFN_FILEMUSTEXIST to eliminate problems in LoadFile.
                 */
                OFN.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

                LockData(0);
		if (GetOpenFileName ((LPOPENFILENAME)&OFN))
                {
		    fp = OpenFile( (LPSTR)szNewName, (LPOFSTRUCT)&rgbOpen, 0 ) ;

                  /* Try to load the file and reset fp if failed */
                  if (!LoadFile(szNewName))
                        fp = OpenFile(szFileName, &rgbOpen, 0);
                }
		else if(CommDlgExtendedError())/* Lo-mem dialog box failed. */
			AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
                        UnlockData(0);
                }
            break;

        case M_SAVE:
	         /* set up the variable fields of the OPENFILENAME struct.
	         * (the constant fields have been sel in NPInit()
	         */

            if (!fUntitled && (IsNotepadEmpty(hwndNP, szFileName, FALSE) ||
                                   SaveFile(hwndNP, szFileName, FALSE)))
              {
                break;
              }

            /* Null out filename */
            rgbOpen.szPathName[0]=0;

	         /* fall through */

        case M_SAVEAS:
            if (IsNotepadEmpty(hwndNP, szFileName, TRUE))
                break;

            OFN.lpstrFile      = szNewName;
            OFN.lpstrInitialDir   = szCurDir;
            OFN.lpstrTitle      = szSaveCaption;
            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             */
            OFN.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             */
            OFN.lpstrFilter      = (LPSTR)szFilterSpec;
            OFN.lpstrCustomFilter = (LPSTR)szCustFilterSpec;
            OFN.lpstrDefExt       = (LPSTR)szMyExt + 3;  /* points to TXT */

            if (!fUntitled)
                lstrcpy (szNewName, szFileName); /* set default selection */
            else
                lstrcpy (szNewName, szCustFilterSpec + 1);

            fInSaveAsDlg = TRUE;
            LockData(0);
            if (GetSaveFileName(&OFN) && SaveFile(hwnd, szNewName, TRUE))
                lstrcpy (szFileName, szNewName);

            else if(CommDlgExtendedError())/*Lo-mem dialog box failed. */
	    	      AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
            UnlockData(0);

            fInSaveAsDlg = FALSE;
            break;

        case M_SELECTALL:
            lSel=(LONG)SendMessage(hwndEdit, WM_GETTEXTLENGTH,0,0L);
            SendMessage( hwndEdit, EM_SETSEL, GET_EM_SETSEL_MPS( 0, lSel ) ) ;
            break;

        case M_FINDNEXT:
            if (szSearch[0])
              {
                Search(szSearch);
                break;
              }
            /* else fall thro' a,d bring up "find" dialog */

        case M_FIND:
            if (hDlgFind)
                SetFocus(hDlgFind);
            else
              {
                FR.lpstrFindWhat      = (LPSTR)szSearch;
                FR.wFindWhatLen       = CCHKEYMAX;
                hDlgFind = FindText ((LPFINDREPLACE)&FR);
              }
            break;

        case M_ABOUT:
            ShellAbout(hwndNP, szNN, "",
                       LoadIcon(hInstanceNP, MAKEINTRESOURCE(ID_ICON)));
            break;

        case M_USEHELP:
            if(!WinHelp(hwndNP, (LPSTR)NULL, HELP_HELPONHELP, 0))
		    AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

            break;

        case M_HELP:
         if(!WinHelp(hwndNP, (LPSTR)szHelpFile, HELP_INDEX, 0))
           AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

            break;

        case M_SEARCHHELP:
/*            MessageBox(hwndNP, "Insert call to search engine here...",
                       "NOTEPAD.HELP.Search for Help on...", MB_OK); */
              if (!WinHelp(hwndNP, (LPSTR)szHelpFile, HELP_PARTIALKEY, (DWORD)(LPSTR)""))
                    AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
            break;

        case M_CUT:
        case M_COPY:
        case M_CLEAR:
              lSel = SendMessage( hwndEdit, EM_GETSEL, 0, 0L);
              if (LOWORD(lSel) == HIWORD(lSel))
                  break;

        case M_PASTE:
            /* If notepad parent or edit window has the focus,
               pass command to edit window.
               make sure line resulting from paste will not be too long. */
            hwndFocus = GetFocus();
            if (hwndFocus == hwndEdit || hwndFocus == hwndNP)
                PostMessage(hwndEdit, GET_WM_COMMAND_ID(wParam, lParam), 0, 0);
            break;

        case M_DATETIME:
            InsertDateTime(FALSE);
            break;

        case M_UNDO:
            SendMessage(hwndEdit, EM_UNDO, 0, 0L);
            break;

        case M_WW:
        /* ES_NOHIDESEL added so selection can be seen when bringing up
         * the Find and Find/Replace dialogs.
         */
            if (NpReCreate((!fWrap) ? WS_CHILD | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE | ES_NOHIDESEL
                                    : WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE))
                fWrap = !fWrap;
            else
                AlertBox(hwndNP, szNN, szNoWW, 0, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            break;

        case ID_EDIT:
            if (GET_WM_COMMAND_HWND(wParam, lParam) != hwndEdit)
                break;
            if (GET_WM_COMMAND_CMD(wParam, lParam)== EN_CHANGE)
            {
                if (!(SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, (LONG)0)))
                {
                    SetScrollPos(hwndNP, SB_VERT, 0, TRUE);
                    SetScrollPos(hwndNP, SB_HORZ, 0, TRUE);
                }
            }
            if (GET_WM_COMMAND_CMD(wParam, lParam)== EN_HSCROLL)
                {
                SetScrollPos(hwndNP, SB_HORZ,
                             (INT)SendMessage(hwndEdit, WM_HSCROLL, EM_GETTHUMB, 0L), TRUE);
                }
            else if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_VSCROLL)
                {
                SetScrollPos(hwndNP, SB_VERT,
                             (INT)SendMessage(hwndEdit, WM_VSCROLL, EM_GETTHUMB, 0L), TRUE);
                }
            break;

        case M_PRINT:
            if (((iErrPrint = NpPrint()) < 0) && (iErrPrint != SP_USERABORT))
            {
	        switch(iErrPrint){
                case SP_OUTOFDISK:
                    pchT = szNEDSTP;
		    break;
         	case SP_OUTOFMEMORY:
                    pchT = szNEMTP;
                    break;
                default:
                    pchT = szCP;
                }
                AlertBox(hwndNP, szNN, pchT, fUntitled ? szUntitled : szFileName,  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            }
            break;

        case M_PAGESETUP:
            lpfn = MakeProcInstance((FARPROC)PageSetupDlgProc, hInstanceNP);
            if (lpfn)
            {
                DialogBox(hInstanceNP, MAKEINTRESOURCE(IDD_PAGESETUP), hwndNP, (WNDPROC)lpfn);
                FreeProcInstance(lpfn);
            }
            break;

        case M_PRINTSETUP:
         {
            DWORD rc;

            /* invoke only the Setup dialog */

            PD.Flags = PD_PRINTSETUP;
TryPrintDlgAgain:
            LockData(0);
            PrintDlg(&PD); /* Use new library module */

            rc = CommDlgExtendedError();
            UnlockData(0);

	    if (rc == PDERR_PRINTERNOTFOUND || rc == PDERR_DNDMMISMATCH)
	      {
	        FreeGlobal();
                PD.hDevMode = PD.hDevNames = 0;
	        goto TryPrintDlgAgain;
	      }

            if(rc == CDERR_DIALOGFAILURE || rc == CDERR_INITIALIZATION ||
               rc == CDERR_LOADSTRFAILURE || rc == CDERR_LOADRESFAILURE ||
               rc == PDERR_LOADDRVFAILURE  || rc == PDERR_GETDEVMODEFAIL )/* Check for Dialog Failure. */
	    	AlertBox(hwndNP, szNN, (rc == PDERR_LOADDRVFAILURE)
                                          ? szLoadDrvFail : szErrSpace,
                                          0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

            break;
       }
        default:
            return FALSE;
    }
    return TRUE;
}

void FileDragOpen()
{
	if (CheckSave(FALSE)){

		/* Possible OEM to ANSI conversion of ptNewName required. */
	
		fp = OpenFile(szPath, &rgbOpen, 0);

		/* Try to load the file and reset fp if failed */
		if (!LoadFile(szPath))
			fp = OpenFile(szFileName, &rgbOpen, 0);
	}
}


/* Proccess file drop/drag options. */
void doDrop(WPARAM wParam, HWND hwnd)
{

	if(DragQueryFile((HANDLE)wParam,0xFFFF,(LPSTR)NULL,0)){ /* # of files dropped */
		/* If user draged/dropped a file regardless of keys pressed
		 * at the time, open the first selected file from file
		 * manager.
		 */
		DragQueryFile((HANDLE)wParam,0,szPath,(WORD)sizeof(szPath));
		SetActiveWindow(hwnd);
		FileDragOpen();
			
	}
	DragFinish((HANDLE)wParam);	/* Delete structure alocated for WM_DROPFILES*/
}

/* ** if notepad is dirty, check to see if user wants to save contents */
BOOL FAR CheckSave(
    BOOL    fSysModal)
    {
    INT mdResult = IDOK;
    CHAR     szNewName[CCHFILENAMEMAX] = "";	 /* New file name */

    char *pszFileName;

/* If it's untitled and there's no text, don't worry about it */
    if (fUntitled && !(SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, (LONG)0)))
        return(TRUE);

    if (SendMessage(hwndEdit, EM_GETMODIFY, 0, 0L))
        {
        if (fUntitled)
            pszFileName = szUntitled;
        else if (*(WORD *)(szFileName + 2) == 0x5C5C)
          {
            pszFileName = szFileName + 2;
          }
        else
          {
            pszFileName = szFileName;
          }
	mdResult = AlertBox(hwndNP, szNN, szSCBC, pszFileName,
      (WORD)((fSysModal ? MB_SYSTEMMODAL : MB_APPLMODAL) | MB_YESNOCANCEL | MB_ICONEXCLAMATION));
        if (mdResult == IDYES)
            {
	         if (fUntitled)
                  {
                lstrcpy(szNewName, szCustFilterSpec + 1);
SaveFilePrompt:
		         OFN.lpstrFile	      = szNewName;
		         OFN.lpstrInitialDir   = szCurDir;
		         OFN.lpstrTitle	      = szSaveCaption;
                /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
                 */
               OFN.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
                           OFN_NOREADONLYRETURN |       OFN_PATHMUSTEXIST;

		         /* ALL non-zero long pointers must be defined immediately
		         * before the call, as the DS might move otherwise.
		         */
		         OFN.lpstrFilter	      = (LPSTR)szFilterSpec;
		         OFN.lpstrCustomFilter = (LPSTR)szCustFilterSpec;
		         OFN.lpstrDefExt       = (LPSTR)szMyExt + 3;  /* points to TXT */

                fInSaveAsDlg = TRUE;
                LockData(0);
                if (GetSaveFileName (&OFN))
                {
                    lstrcpy (szNewName, OFN.lpstrFile); /* since SaveFile() uses near ptr. to name */
                    if (SaveFile(hwndNP, szNewName, TRUE))
                        lstrcpy (szFileName, szNewName);
                }

		else
                {
                   mdResult = IDCANCEL;       /* Don't exit Program */
                  if(CommDlgExtendedError()) /* SaveAs Dialog box failed, Lo-mem*/
		    AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
                }
                UnlockData(0);

                fInSaveAsDlg = FALSE;
            }
            else
            {
              if ((mdResult = IsNotepadEmpty(hwndNP, szFileName, FALSE))
                                                                   == FALSE)
                {
                  if (SaveFile(hwndNP, szFileName, FALSE))
                      return(TRUE);
                  lstrcpy (szNewName, szFileName);
                  goto SaveFilePrompt;
                }
            }
        }
   }
   return (mdResult != IDCANCEL);
}

/* Notepad window class procedure */
LONG APIENTRY NPWndProc(
	HWND	   hwnd,
	UINT     message,
	WPARAM	wParam,
	LONG	   lParam)
{
    RECT rc;
    LPFINDREPLACE lpfr;
    DWORD dwFlags;

    switch (message)
        {
/* If we're being run by Setup and it's the system menu, be certain that
 * the minimize menu item is disabled.  Note that hSysMenuSetup is only
 * initialized if Notepad is being run by Setup.  Don't use it outside
 * the fRunBySetup conditional!    
 */
        case WM_INITMENUPOPUP:
            if (fRunBySetup && HIWORD(lParam))
            {
              EnableMenuItem(hSysMenuSetup, SC_MINIMIZE, MF_GRAYED|MF_DISABLED);
            }
            break;

        case WM_SYSCOMMAND:
            if (fRunBySetup)
              {
                /* If we have been spawned by SlipUp we need to make sure the
		 * user doesn't minimize us or alt tab/esc away.
		 */
                if (wParam == SC_MINIMIZE ||
                    wParam == SC_NEXTWINDOW ||
                    wParam == SC_PREVWINDOW)
                    break;
              }
                DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
                if (!IsIconic(hwndNP))
                    SetFocus(hwndEdit);
            break;

        case WM_KILLFOCUS:
            SendMessage(hwndEdit, message, wParam, lParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            if (CheckSave(FALSE))
                {
                /* Exit help */
                if(!WinHelp(hwndNP, (LPSTR)szHelpFile, HELP_QUIT, 0))
		    AlertBox(hwndNP, szNN, szErrSpace, 0, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

                DestroyWindow(hwndNP);
                DeleteObject(hFont);
#ifdef JAPAN
		{
		    extern FARPROC lpEditSubClassProc;

		    /* Clear proc instance for sub class function */
		    FreeProcInstance(lpEditSubClassProc);
		}
#endif
                }
            break;

        case WM_QUERYENDSESSION:
            if (fInSaveAsDlg)
            {
                MessageBeep(0);
                MessageBeep(0);
                MessageBox(hwndNP, szCannotQuit, szNN, MB_OK|MB_SYSTEMMODAL);
                return FALSE;
            }
            else
                return (CheckSave(TRUE));
            break;

        /* Added in conjunction with adding ES_NOHIDESEL to the window style
         * of the edit control.  Hides selection when the application loses
         * the focus, but not when Notepad brings up dialog boxes.  Designed
         * to facilitate using the Find and Find/Replace dialogs.
         */
        case WM_ACTIVATEAPP:
            if (wParam)
            {
            /* This causes the caret position to be at the end of the selection
             * but there's no way to ask where it was or set it if known.  This
             * will cause a caret change when the selection is made from bottom
             * to top.
             */
                SendMessage(hwndEdit, EM_SETSEL, LOWORD(dwCurrentSelection),
                    HIWORD(dwCurrentSelection));
            }
            else
            {
                dwCurrentSelection = SendMessage(hwndEdit, EM_GETSEL, 0, 0L);
                if (LOWORD(dwCurrentSelection) == HIWORD(dwCurrentSelection))
                    dwCurrentSelection = 0L;
                else
                   SendMessage(hwndEdit, EM_SETSEL, HIWORD(dwCurrentSelection),
                      HIWORD(dwCurrentSelection));

            }
            break;

        case WM_ACTIVATE:
            if ( (GET_WM_ACTIVATE_STATE(wParam,lParam) == 1 ||
                  GET_WM_ACTIVATE_STATE(wParam,lParam) == 2) &&
                 !IsIconic(hwndNP)
               )
                SetFocus(hwndEdit);
            break;

        case WM_SIZE:
            switch (wParam)
                {
                case SIZENORMAL:
                case SIZEFULLSCREEN:
                    NPSize(MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
                    break;

                case SIZEICONIC:
                    return(DefWindowProc(hwnd, message, wParam, lParam));
                    break;
                }
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            SendMessage(hwndEdit, message, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            GetClientRect(hwndEdit, (LPRECT)&rc);

            if (MAKEPOINTS(lParam).x <= CXMARGIN)
                MAKEPOINTS(lParam).x = 0;
            else
                {
                MAKEPOINTS(lParam).x -= CXMARGIN;

                if ((LONG)(MAKEPOINTS(lParam).x) >= rc.right)
                  MAKEPOINTS(lParam).x = (SHORT)(rc.right-1);
                }

            if (MAKEPOINTS(lParam).y <= CYMARGIN)
                MAKEPOINTS(lParam).y = 0;
            else
                {
                MAKEPOINTS(lParam).y -= CYMARGIN;
                         
                if ((LONG)(MAKEPOINTS(lParam).y) >= rc.bottom)
                  MAKEPOINTS(lParam).y = (SHORT)(rc.bottom-1);
                }

            PostMessage(hwndEdit, message, wParam, lParam);
            break;

        case WM_INITMENU:
            NpResetMenu();
            break;

        case WM_COMMAND:
            if (GET_WM_COMMAND_HWND(wParam,lParam) == hwndEdit &&
                (GET_WM_COMMAND_CMD(wParam,lParam) == EN_ERRSPACE ||
                 GET_WM_COMMAND_CMD(wParam,lParam) == EN_MAXTEXT
                )
               )
                {
                if (wEmSetHandle == SETHANDLEINPROGRESS)
                    wEmSetHandle = SETHANDLEFAILED;
                else
                    AlertBox(hwndNP, szNN, szErrSpace, 0,
                           MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                return 0L;
                }

#ifndef WIN32
            if (NPCommand(hwnd, wParam, lParam))
                return (0L);
            /* Fall Thru */
#else
            if( !NPCommand(hwnd, wParam, lParam) )
               return(DefWindowProc(hwnd, message, wParam, lParam));
            break ;
#endif

        case WM_WININICHANGE:
            NpWinIniChange();
            break;

	case WM_DROPFILES: /*case added 03/26/91 for file drag/drop support*/
			doDrop(wParam,hwnd);
			break;


        default:
            /* this can be a message from the modeless Find Text window */
            if (message == wFRMsg)
            {
                lpfr = (LPFINDREPLACE)lParam;
                dwFlags = lpfr -> Flags;

                fReverse = (dwFlags & FR_DOWN       ? FALSE : TRUE);
                fCase     = (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);

                if (dwFlags & FR_FINDNEXT)
                {
                    Search(szSearch);
                }
                else if (dwFlags & FR_DIALOGTERM)
                {
                    hDlgFind = NULL;   /* invalidate modeless window handle */
                }
                break;
            }
            return(DefWindowProc(hwnd, message, wParam, lParam));
            break;
    }
    return(0L);
}


/* ** Main loop */
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow
) {
    FARPROC lpfnRegisterPenApp;
    MSG msg;

/* PenWindow registration must be before creating an edit class window.
 * Moved here, along with goto statement below for appropriate cleanup.
 */
    if ((FARPROC)lpfnRegisterPenApp = GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS),
        "RegisterPenApp"))
        (*lpfnRegisterPenApp)(1, TRUE);

    if (!NPInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
    {
        msg.wParam = FALSE;
        goto UnRegisterPenWindows;
    }

    while (GetMessage((LPMSG)&msg, (HWND)NULL, 0, 0))
    {
        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg))
        {
            if (TranslateAccelerator(hwndNP, hAccel, (LPMSG)&msg) == 0)
            {
	            TranslateMessage((LPMSG)&msg);
               DispatchMessage((LPMSG)&msg);
            }
        }
    }

    FreeGlobal();

UnRegisterPenWindows:

    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    return(msg.wParam);
}

/* ** Set Window caption text */
void FAR SetTitle(
	CHAR    *sz)
{
    CHAR    szWindowText[256];
    LPSTR   pch;

    if (!fUntitled)
	AnsiUpper((LPSTR)sz);

    /* Strip path/drive specification from name if there is one */
    pch = PFileInPath(sz);
    lstrcat(lstrcpy((LPSTR)szWindowText, (LPSTR)szNpTitle), (LPSTR)pch);
    SetWindowText(hwndNP, (LPSTR)szWindowText);
}

/* ** Given filename which may or maynot include path, return pointer to
      filename (not including path part.) */
LPSTR PASCAL far PFileInPath(
    LPSTR sz)
{
    LPSTR pch = sz;
    LPSTR psz;

    /* Strip path/drive specification from name if there is one */
    /* Ripped out AnsiPrev calls.                               */
    for (psz = sz; *psz; psz = AnsiNext(psz))
      {
        if ((*psz == ':') || (*psz == '\\'))
            pch = psz;
      }

    if (pch != sz)   /* If found slash or colon, return the next character */
        pch++;       /* increment OK, pch not pointing to DB character     */

    return(pch);
}

/* ** Enable or disable menu items according to selection state
      This routine is called when user tries to pull down a menu. */
VOID NpResetMenu(
    VOID)
{
    LONG    lsel;
    INT     mfcc;   /* menuflag for cut, copy */
    BOOL    fCanUndo;
    WORD    wFmt;
    HANDLE hMenu;
    CHAR msgbuf[20];

    hMenu = GetMenu(hwndNP);
    lsel = SendMessage(hwndEdit, EM_GETSEL, 0, 0L);
    mfcc = LOWORD(lsel) == HIWORD(lsel) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CUT, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_COPY, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CLEAR, mfcc);
    fCanUndo = (BOOL) SendMessage(hwndEdit, EM_CANUNDO, 0, 0L);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_UNDO, fCanUndo ? MF_ENABLED : MF_GRAYED);
    CheckMenuItem(GetSubMenu(hMenu, 1), M_WW, fWrap ? MF_CHECKED : MF_UNCHECKED);


    EnableMenuItem(hMenu, M_PRINT, (PD.hDevNames ||
                          GetProfileString("windows","device","", msgbuf, 20))
                          ? MF_ENABLED : MF_GRAYED);

    mfcc = MF_GRAYED;
    if (OpenClipboard(hwndNP))
    {
        wFmt = 0;

	     /* If clipboard has any text data, enable paste item.  otherwise
	     leave it grayed. */
        while (wFmt = EnumClipboardFormats(wFmt))
            {
            if (wFmt == CF_TEXT)
                {
		mfcc = MF_ENABLED;
		break;
                }
            }
	CloseClipboard();
        }
    EnableMenuItem(GetSubMenu(hMenu, 1), M_PASTE, mfcc);
}


void FAR NpWinIniChange(VOID)
{
   InitCountry();
}

/* ** Scan sz1 for merge spec.    If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL MergeStrings(
    CHAR    *szSrc,
    CHAR    *szMerge,
	CHAR    *szDst)
    {
    register	CHAR *pchSrc;
    register	CHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

#ifdef DBCS
    /* Find merge spec if there is one. */
    while ((WORD)*pchSrc != wMerge) {
        if( IsDBCSLeadByte(*pchSrc) )
            *pchDst++ = *pchSrc++;
        *pchDst++ = *pchSrc;

	/* If we reach end of string before merge spec, just return. */
        if(!*pchSrc++)
            return FALSE;
    }
#else
    /* Find merge spec if there is one. */
    while ((WORD)*pchSrc != wMerge)
        {
	*pchDst++ = *pchSrc;

	/* If we reach end of string before merge spec, just return. */
	if (!*pchSrc++)
	    return FALSE;

        }
#endif

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge)
        {
	while (*szMerge)
	    *pchDst++ = *szMerge++;
        }

    /* Jump over merge spec */
    pchSrc++,pchSrc++;

    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;

    }

/* ** Post a message box */
INT FAR AlertBox(
    HWND    hwndParent,
    CHAR    *szCaption,
    CHAR    *szText1,
    CHAR    *szText2,
    WORD    style)
    {
#ifdef JAPAN
    CHAR    szMessage[256+32];
#else
    CHAR    szMessage[256];
#endif

    MergeStrings(szText1, szText2, szMessage);
    return (MessageBox(hwndParent, (LPSTR)szMessage, (LPSTR)szCaption, style));
}
