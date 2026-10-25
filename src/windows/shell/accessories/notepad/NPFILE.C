//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//	PROGRAM:	npfile.c
//
//	PURPOSE:	Routines for file i/o for notepad
//
//	PLATFORMS:	Chicago 
//
//	FUNCTIONS:	
//	    IsNotepadEmpty
//      SaveFile
//      LoadFile
//      New
//      AddExt
//      Remove
//		
//	SPECIAL INSTRUCTIONS: N/A
//

#include "notepad.h"

HANDLE hFirstMem;

/* IsNotepadEmpty
 * Check if the edit control is empty.  If it is, put up a messagebox
 * offering to delete the file if it already exists, or just warning
 * that it can't be saved if it doesn't already exist
 *
 * Return value:  TRUE, warned, no further action should take place
 *                FALSE, not warned, or further action is necessary
 */
short FAR IsNotepadEmpty(
HWND hwndParent,
char *szFileSave,
BOOL fNoDelete)
{
  unsigned cch;
  short nRetVal = FALSE;

  cch = (unsigned) SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, (LONG)0);

/* If notepad is empty, complain and delete file if necessary. */
  if (!cch)
    {
      if (fNoDelete)
          nRetVal = AlertBox(hwndParent, szNN, szCSEF, 0,
                                     MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
      else if ((nRetVal = AlertBox(hwndNP, szNN, szEFD, szFileSave,
                    MB_APPLMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION)) == IDOK)
        {
          Remove((LPSTR)rgbOpen.szPathName);
          New(FALSE);
        }
    }
  return nRetVal;
}

/* ** Save notepad file to disk.  szFileSave points to filename.  fSaveAs
      is TRUE iff we are being called from SaveAsDlgProc.  This implies we must
      open file on current directory, whether or not it already exists there
      or somewhere else in our search path. 
*/
/*  Assumes that text exists within hwndEdit. */
BOOL FAR SaveFile(
        HWND    hwndParent,
        char    *szFileSave,
        BOOL     fSaveAs)
{
    LPSTR lpch;
    unsigned cch;
    BOOL    fFormatted;
    BOOL    fNew = FALSE;

    /* Display the hour glass cursor */
    SetCursor(hWaitCursor);

    /* If saving to an existing file, make sure correct disk is in drive */
    if (!fSaveAs)
    {
            fp = OpenFile(szFileSave, &rgbOpen,
               (DWORD)(OF_REOPEN | OF_PROMPT | OF_CANCEL | OF_READWRITE | OF_SHARE_DENY_WRITE));
        /* Otherwise open file on current directory. */
    }
    else
    {
            /* Try lopen before lcreate so that if file exists and write fails,
           file is not trashed. */
        if ((fp = _lopen(szFileSave, READ_WRITE)) < 0)
                fNew = ((fp = _lcreat(szFileSave, 0)) >=0);
    }

    if (fp == -1)
    {
        if (fSaveAs)
        AlertBox(hwndParent, szNN, szCREATEERR, szFileSave,
                 MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
    }
    else
    {
        /* if wordwrap, remove soft carriage returns */
        fFormatted = SendMessage(hwndEdit, EM_FMTLINES, (WPARAM)FALSE, 0L);

#ifdef WIN32S
        /* Must get text length again after formatting. */
        cch = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, (LONG)0);

        /*
         * For Win32s, realloc hEdit to make room for the current contents
         * of the edit control.
         *
         * !!! Could choose a better error message.
         */
        if (!(hEdit = LocalReAlloc(hEdit, cch+1, LHND)))
        {
            AlertBox(hwndParent, szNN, szWE, szFileSave,
                MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }

        lpch = LocalLock(hEdit);
        SendMessage(hwndEdit, WM_GETTEXT, cch+1, (LONG)lpch);
#else
            lpch  = (LPSTR)LocalLock(hEdit);

        /* Must get text length again after formatting */
            cch = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, (LONG)0);
#endif

        if ((unsigned int)_lwrite(fp, lpch, cch) != cch)
        {
            LocalUnlock(hEdit);
                _lclose(fp);
            if (fNew)
            {
                    AnsiToOem(szFileSave, szFileSave);
                    Remove((LPSTR)szFileSave);
                    OemToAnsi(szFileSave, szFileSave);
            }
            AlertBox(hwndParent, szNN, szWE, szFileSave, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
        }
        else
        {
            _lwrite(fp, lpch, 0);   /* This resets eof */
                SendMessage(hwndEdit, EM_SETMODIFY, FALSE, 0L);
                SetTitle(szFileSave);
            fUntitled = FALSE;
        }

        _lclose(fp);
         /* if wordwrap, insert soft carriage returns */
            if (fFormatted)
            SendMessage(hwndEdit, EM_FMTLINES, (WPARAM)TRUE, 0L);
        /* Reset OpenFile's OFSTRUCT buffer */
            if (fSaveAs)
                _lclose(OpenFile(szFileSave, &rgbOpen, OF_READ));

        LocalUnlock(hEdit);
    }

    return TRUE;
}

#include <stdio.h>

/* **Read contents of file from disk.  File is already open, referenced by
    handle fp */
BOOL FAR LoadFile(
        char *sz)
{
    unsigned  cch, i;
    long      lcch;
    LPSTR     lpch;
    BOOL      fLog;
    char      *p;
    char     szSave[CCHFILENAMEMAX]; /* Private copy of current filename */

    if (fp == -1)
    {
            AlertBox(hwndNP, szNN, szDiskError, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    /* Display the hour glass */
    SetCursor(hWaitCursor);

    /* Get size of file, then reset file pointer */
    lcch = _llseek(fp, 0L, 2);
#ifdef WIN32
    cch = (unsigned)lcch ;
#else
    cch = LOWORD(lcch);
#endif
    _llseek(fp, 0L, 0);

    /* If file too big, or not enough memory, inform user */
#ifndef WIN32
    if ((lcch > (LONG)CCHNPMAX) || !(LocalReAlloc(hEdit, cch+1, LHND)))
#else
    if ( !(hEdit = LocalReAlloc(hEdit, cch+1, LHND)) )
#endif
    {
/* New() causes szFileName to be set to "Untitled".  Save a
 * copy of the filename to pass to AlertBox.
 */
        lstrcpy(szSave, sz);
#ifndef WIN32
        if (lcch <= (LONG)CCHNPMAX)
        {
#endif
                SendMessage(hwndEdit, WM_SETTEXT, 0, (LONG)(LPSTR)"");
            New(FALSE);

            if (hEdit = LocalReAlloc(hEdit, cch+1,LHND))
                 goto GotMemory;
            else
            {
                _lclose(fp);
                    AlertBox(hwndNP, szNN, szFTL, szSave, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }
#ifndef WIN32
        }
#endif
        _lclose(fp);
        New(FALSE);
        AlertBox(hwndNP, szNN, szFTL, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
    }

GotMemory:
    /* Reset selection to 0 */
    SendMessage(hwndEdit, EM_SETSEL, 0, 0L);

    /* Read file into buffer */
    lpch = LocalLock(hEdit);

    _lread(fp, lpch, (unsigned)cch);
    _lclose(fp);

    /* Fix NUL chars if any, in the file */
    for (i = 0, p = lpch; i < cch; i++, p++)
    {
        if (*p == 0)
            *p = ' ';
    }
    *(lpch + cch) = 0;      /* zero terminate the thing */

    /* if size of file not same as string length of file, it's not text. */
    /* If the file is of 0 length, don't fail.
     */
    if (cch && lstrlen(lpch) < (int)(cch-1))
    {
        LocalUnlock(hEdit);
            AlertBox(hwndNP, szNN, szINF, sz, MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION);

             New(FALSE);
        return (FALSE);
    }

    fLog = *lpch++ == '.' && *lpch++ == 'L' && *lpch++ == 'O' && *lpch == 'G';
    LocalUnlock(hEdit);

    lstrcpy(szFileName, sz);
    SetTitle(sz);
    fUntitled = FALSE;

    /* Don't display text until all done. */
    SendMessage(hwndEdit, WM_SETREDRAW, (WORD)FALSE, (LONG)0);

    /* Pass handle to edit control.  This is more efficient than WM_SETTEXT
       which would require twice the buffer space. */

/* If EM_SETHANDLE doesn't have enough memory to complete things,
 * it will send the EN_ERRSPACE message.  If this happens, don't put up the
 * out of memory notification, put up the file to large message instead.
 */
    wEmSetHandle = SETHANDLEINPROGRESS;
#ifdef WIN32S
    /*
     * For Win32s, use WM_SETTEXT.
     *
     * Win32s has 32-bit local memory handles which it cannot pass to
     * Win3.1.  This means that the EM_SETHANDLE and EM_GETHANDLE messages
     * are not supported.  A private 16-bit heap (unknown to the app)
     * provides space for the multiline edit control.
     */
    lpch = LocalLock(hEdit);
    SendMessage(hwndEdit, WM_SETTEXT, 0, (LONG)lpch);
    LocalUnlock(hEdit);
#else
	SendMessage(hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, (LONG));
#endif
    if (wEmSetHandle == SETHANDLEFAILED)
      {
        wEmSetHandle = 0;
        AlertBox(hwndNP, szNN, szFTL, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LONG)0);
        New(FALSE);
        return FALSE;
      }
    wEmSetHandle = 0;



#ifndef WIN32
    /* added 01-Jul-1987 by davidhab. */
    PostMessage(hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L);
#endif

    /* If file starts with ".LOG" go to end and stamp date time */
    if (fLog)
    {
#ifdef WIN32
            SendMessage(hwndEdit, EM_SETSEL, (DWORD)cch, (DWORD)cch );
#else
            SendMessage(hwndEdit, EM_SETSEL, 0, MAKELONG(cch,cch));
#endif
        InsertDateTime(TRUE);
    }

    /* Move vertical thumb to correct position */
    SetScrollPos(hwndNP, SB_VERT,
                 (int)SendMessage(hwndEdit, WM_VSCROLL, EM_GETTHUMB, 0L), TRUE);

    /* Now display text */
    SendMessage(hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LONG)0);
    InvalidateRect(hwndEdit, (LPRECT)NULL, TRUE);
    UpdateWindow(hwndEdit);

    return TRUE;
}

/* ** New Command - reset everything */
void FAR New(
    BOOL    fCheck)
{

    if (!fCheck || CheckSave(FALSE))
    {
        SendMessage(hwndEdit, WM_SETTEXT, (WPARAM)0, (DWORD)(LPSTR)"");
            fUntitled = TRUE;
        lstrcpy(szFileName, szUntitled);
        SetTitle(szFileName);
        SendMessage(hwndEdit, EM_SETSEL, 0, 0L);
        hEdit = LocalReAlloc(hEdit, 0, LHND);
#ifndef WIN32S
        /*
         * For Win32s version, hEdit is a local copy of edit control text
         * which must be explicitly updated before accessing it.  Win32s
         * does not support the EM_SETHANDLE and EM_GETHANDLE messages
         * because it cannot share local memory handles with Win3.1.
         */
		SendMessage(hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, 0L);
#endif
            szSearch[0] = 0;
    }
}

/* ** If sz does not have extension, append ".TXT" */
void FAR AddExt(
        char *sz)
{
    char *pch1;
    int ch;

    pch1 = sz + lstrlen(sz);

    while ((ch = *pch1) != '.' && ch != '\\' && ch != ':' && pch1 > sz)
        pch1 = (char *)AnsiPrev(sz, pch1);

    if (*pch1 != '.')
        lstrcat(sz, ".TXT");
}

#ifdef WIN32

INT FAR Remove(
        LPSTR szFileName)
{
   return (INT)DeleteFile( szFileName ) ;
}
#endif
