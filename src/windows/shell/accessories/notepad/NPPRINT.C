//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//	MODULE: 	npprint.c
//
//	PURPOSE:	Printing support routines.
//
//	PLATFORMS:	Chicago
//
//	FUNCTIONS:	
//
//
//	SPECIAL INSTRUCTIONS: N/A
//

#define NOMINMAX
#include "notepad.h"
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "dlgs.h"

#define HEADER 0
#define FOOTER 1

INT     tabSize;                    /* Size of a tab for print device */
HWND    hAbortDlgWnd;
INT     fAbort, xPrintChar, yPrintChar ;
FARPROC lpfnAbortProc;
INT     ExtPrintLeading;

/* Character width of page excluding margins. */
INT     xCharPage ;
/* We'll dynamically allocate this */
HANDLE hHeadFoot=NULL;
LPSTR  szHeadFoot;

INT xPrintRes;
INT yPrintRes;
INT yPixInch;        /* pixels/inch */
INT xPixInch;           /* pixels/inch */

INT xLeftSpace, xRightSpace; /* Space of margins */

INT dyTop;        /* width of top border */
INT dyBottom;        /* width of bottom border */
INT dxLeft;        /* width of left border */
INT dxRight;        /* width of right border (this doesn't get used) */
INT dyHeadFoot;        /* height from top/bottom of headers and footers */

INT iPageNum;        /* global page number currently being printed */
HMENU hSysMenu;

static int CheckMarginNums(
    HWND hWnd);

void DestroyAbortWnd(
   void) ;

SHORT TranslateString(
   CHAR *);

INT atopix(
   CHAR *ptr,
   INT pix_per_in) ;

int ExpandTabs(
    char *pLine,    /* start of the line */
    int nChars,     /* number of chars in the line */
    LPSTR lpBuffer, /* buffer for the expanded text */
    int PageWidth); /* Page width in chars */

INT FAR AbortProc(
        HDC hPrintDC,
        INT reserved)
{
    MSG msg;

	while ( !fAbort &&
		PeekMessage(
			(LPMSG)&msg,
			NULL,
			( UINT ) NULL,
			( UINT )NULL,
			TRUE)
	) {
    if (!hAbortDlgWnd || !IsDialogMessage(hAbortDlgWnd, (LPMSG)&msg))
        {
        TranslateMessage((LPMSG)&msg);
        DispatchMessage((LPMSG)&msg);
		}
	}

    return(!fAbort);

   hPrintDC ;
   reserved ;
}


INT APIENTRY AbortDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LONG lParam)
{
    switch(msg)
    {
    case WM_COMMAND:
        fAbort = TRUE;
        DestroyAbortWnd();
        return(TRUE);

    case WM_INITDIALOG:
        hSysMenu = GetSystemMenu(hwnd, FALSE);
        if (!fUntitled)
        SetDlgItemText(hwnd, ID_FILENAME,
        fUntitled ? (LPSTR)szUntitled : (LPSTR)PFileInPath(szFileName));
        SetFocus(hwnd);
        return(TRUE);

    case WM_INITMENU:
        EnableMenuItem(hSysMenu, (WORD)SC_CLOSE, (DWORD)MF_GRAYED);
        return(TRUE);
    }
    return(FALSE);

   wParam;
   lParam ;
}


/*
 * print out the translated header/footer string in proper position.
 * uses globals xPrintWidth, dyHeadFoot...
 */

VOID PrintHeaderFooter(
   HDC hDC,
   SHORT nHF)
{
    CHAR    buf[80];
    SHORT   len;

    lstrcpy(buf, chPageText[nHF]);

    szHeadFoot=GlobalLock(hHeadFoot);
    len=TranslateString(buf);

    if (*szHeadFoot)
    {
        if (nHF==HEADER)
            TabbedTextOut(hDC, dxLeft, dyHeadFoot - yPrintChar, szHeadFoot, len, 1, &tabSize, dxLeft);
        else
            TabbedTextOut(hDC, dxLeft, yPrintRes-yPrintChar-dyHeadFoot, szHeadFoot, len, 1, &tabSize, dxLeft);
    }
    GlobalUnlock(hHeadFoot);

}

INT FAR NpPrint(void)
{
//#ifdef ESCAPE
    TEXTMETRIC Metrics;
    BOOL    bLongLine;
    CHAR    msgbuf[128];
    HDC     hPrintDC;
    INT     nLinesPerPage;
    INT     y;
    CHAR    *pStartText;
    HANDLE  hText;
    DWORD   Offset;          /* Line Offset into edit control's buffer */
    INT     LineNum, nPrintedLines;
    INT     nChars, nPrintedChars;
    INT     iErr;
    LPSTR   lpLine;
    HANDLE  hBuffer;
	FARPROC lpfnAbortPrinterProc;
#if defined ( WIN32 ) || defined ( WIN32S )
	SIZE	size;
#else
	INT 	nHeight;
	INT 	size;
#endif

    SetCursor(hWaitCursor);
    fAbort = FALSE;

    hPrintDC = GetPrinterDC();

    if(!hPrintDC)
        return SP_OUTOFMEMORY;

    if (hPrintDC == (HANDLE)-1)
        return SP_ERROR;


    SetBkMode(hPrintDC, TRANSPARENT);
    GetTextMetrics(hPrintDC, (LPTEXTMETRIC)(&Metrics));  /* find out what kind of font it really is */
    yPrintChar = Metrics.tmHeight + Metrics.tmExternalLeading;    /* the height */
    xPrintChar = Metrics.tmAveCharWidth;          /* the average width */

    tabSize = xPrintChar*8; /* 8 ave char width pixels for tabs */
    ExtPrintLeading = Metrics.tmExternalLeading;
    xPrintRes = GetDeviceCaps(hPrintDC, HORZRES);
    yPrintRes = GetDeviceCaps(hPrintDC, VERTRES);
    xPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSX);
    yPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSY);

    dyHeadFoot = yPixInch/4;

    dyTop      = atopix(chPageText[4], yPixInch);
    dyBottom   = atopix(chPageText[5], yPixInch);
    dxLeft     = atopix(chPageText[2], xPixInch);
    dxRight    = atopix(chPageText[3], xPixInch);

    /* Number of lines on a page with margins */
    nLinesPerPage = (yPrintRes-dyTop-dyBottom)/yPrintChar;

#if defined ( WIN32 ) || defined ( WIN32S )
	GetTextExtentPoint ( hPrintDC, " ", 1,	&size );
#else
	MGatTextExtent(hPrintDC, " ", 1, &nHeight, &size);
#endif
	xLeftSpace=dxLeft/size.cx;
	xRightSpace=dxRight/size.cx;

    /* Number of characters between margins */
    xCharPage=(xPrintRes/xPrintChar)-xLeftSpace-xRightSpace;

    lpfnAbortPrinterProc = (FARPROC)MakeProcInstance(AbortProc, hInstanceNP);
    lpfnAbortProc = MakeProcInstance((FARPROC)AbortDlgProc, hInstanceNP);

    if (!lpfnAbortPrinterProc || !lpfnAbortProc)
    {
        DeleteDC(hPrintDC);
        return SP_OUTOFMEMORY;
    }

    Escape(hPrintDC, SETABORTPROC, 0, (LPSTR)lpfnAbortPrinterProc, NULL);

    strcat(strcpy(msgbuf, szNpTitle),
            fUntitled ? (LPSTR)szUntitled : (LPSTR)szFileName);

    EnableWindow(hwndNP, FALSE);    // Disable window to prevent reentrancy

    hAbortDlgWnd = CreateDialog(hInstanceNP, MAKEINTRESOURCE(IDD_ABORTPRINT), hwndNP, (WNDPROC)lpfnAbortProc);

    if (!hAbortDlgWnd)
    {
        iErr = SP_OUTOFMEMORY;
ErrorExit:

        DeleteDC(hPrintDC);
        if (!fAbort)
            DestroyAbortWnd();
        FreeProcInstance(lpfnAbortPrinterProc);
        FreeProcInstance(lpfnAbortProc);

        if (hHeadFoot) {
            GlobalFree(hHeadFoot);
            hHeadFoot = NULL;           // this is a global
        }

        if (hBuffer)
            GlobalFree(hBuffer);

        return iErr;
    }

    if ((iErr = Escape(hPrintDC, STARTDOC, lstrlen(msgbuf), msgbuf, (LPSTR)0)) < 0)
        goto ErrorExit;

#ifdef WIN32S
    {
        /*
         * Win32s does not support the EM_GETHANDLE message.  Allocate
         * hText and copy the edit control text into it.
         */

        INT cbText;

        cbText = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0L) + 1;
        if (!(hText = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, cbText)))
        {
            iErr = SP_OUTOFMEMORY;
            goto ErrorExit;
        }
        if (!(pStartText = LocalLock(hText)))
        {
            LocalFree(hText);
            iErr = SP_OUTOFMEMORY;
            goto ErrorExit;
        }
        SendMessage(hwndEdit, WM_GETTEXT, cbText, (LONG)pStartText);
        LocalUnlock(hText);
    }
#else
    hText = (HANDLE)SendMessage(hwndEdit, EM_GETHANDLE, 0, 0L);
#endif
    y       = 0;
    Offset  = 0;
    LineNum = 0;
    nPrintedLines = 0;
    iPageNum = 1;

    /* Allocate memory for the header.footer string.  Will allow any size
     * of paper and still have enough for the string.
     */
    hHeadFoot=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)xCharPage+2);
    /* assuming lines won't be longer than 4K after expanding the TABs */
    hBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 1024*4);
    if (!hHeadFoot || !hBuffer) {
        iErr = SP_OUTOFMEMORY;
        goto ErrorExit;
    }

    lpLine = GlobalLock(hBuffer);
    pStartText = LocalLock(hText);

    do {
        if (nPrintedLines == 0)
            PrintHeaderFooter(hPrintDC, HEADER);

        nChars=SendMessage(hwndEdit, EM_LINELENGTH, Offset, 0L);
        nChars = ExpandTabs(pStartText + Offset, nChars, lpLine, xCharPage);
        nPrintedChars=0;

        if (nChars > xCharPage)
            bLongLine = TRUE;

FinishLine:
        if (nChars > xCharPage)
        {
            TabbedTextOut(hPrintDC, dxLeft, y+dyTop-yPrintChar, lpLine+nPrintedChars, xCharPage, 1, &tabSize, dxLeft);
            nPrintedChars+=xCharPage;
            nChars -=xCharPage;
        }
        else
        {
            TabbedTextOut(hPrintDC, dxLeft, y+dyTop-yPrintChar, lpLine+nPrintedChars, nChars, 1, &tabSize, dxLeft);
            bLongLine=FALSE;
        }

        y += yPrintChar ;
        if (++nPrintedLines >= nLinesPerPage)
        {
            PrintHeaderFooter(hPrintDC, FOOTER);
            nPrintedLines = 0;
            y = 0;
            iPageNum++;

            if ((iErr = Escape(hPrintDC, NEWFRAME, 0, (LPSTR)NULL, (LPSTR)0)) < 0)
                goto ErrorExit;

            if (bLongLine)
                PrintHeaderFooter(hPrintDC, HEADER);
        }

        if (bLongLine)
            goto FinishLine;
    } while (!fAbort &&
     ((Offset=SendMessage(hwndEdit, EM_LINEINDEX, ++LineNum, 0L)) != (DWORD)-1));

    LocalUnlock(hText);
#ifdef WIN32S
    /*
     * Win32s version allocated this above, so free it here.
     */
    LocalFree(hText);
#endif
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);
    hBuffer = NULL;

    if (nPrintedLines)
        PrintHeaderFooter(hPrintDC, FOOTER);

    if (!fAbort)
        if ((iErr = Escape(hPrintDC, NEWFRAME, 0, (LPSTR)NULL, (LPSTR)0) < 0))
            goto ErrorExit;

    if (!fAbort) {
        Escape(hPrintDC, ENDDOC, 0, (LPSTR)0, (LPSTR)0);
        DestroyAbortWnd();
    }

    DeleteDC(hPrintDC);
    FreeProcInstance(lpfnAbortPrinterProc);
    FreeProcInstance(lpfnAbortProc);

    /* Bye, bye memory. */
    GlobalFree(hHeadFoot);
    hHeadFoot=NULL;
//#endif
    return 0;
}

/*
 * Expand TABs in the line to TAB_WIDTH spaces.
 */
int ExpandTabs(
    char *pLine,    /* start of the line */
    int nChars,     /* number of chars in the line */
    LPSTR lpBuffer, /* buffer for the expanded text */
    int PageWidth)  /* Page width in chars */
{
#define TAB_WIDTH       8
    int i,      /* Char count in pLine */
        count,  /* Char count in lpBuffer */
        t,      /* Index into the Temp[] */
        n, k;
    char Temp[200]; /* accomodates PageWidth chars */

    /* replace TABs with appropriate number of spaces */
    count = 0;
    t = 0;
    for (i = 0; i < nChars; pLine++, i++)
    {
        /* If line is full flush it into lpBuffer */
        if (t >= PageWidth)
        {
            Temp[PageWidth] = 0;
            lstrcpy(lpBuffer, Temp);
            lpBuffer += PageWidth;
            t = 0;
        }
        /* process the char */
        if (*pLine == '\t')
        {
            n = ((t+TAB_WIDTH)/TAB_WIDTH)*TAB_WIDTH;        /* get next multiple of TAB_WIDTH */
            if (n >= PageWidth)     /* atmost it can be PageWidth */
                n = PageWidth;
            for (k = t; k < n; k++)
                Temp[k] = ' ';
            count += n - t;
            t = n;
        }
        else
        {
            Temp[t++] = *pLine;
            count++;
        }
   }
    /* flush last partial line into lpBuffer */
    Temp[t] = 0;
    lstrcpy(lpBuffer, Temp);

    return count;
}

VOID DestroyAbortWnd()
    {
    EnableWindow(hwndNP, TRUE);
    DestroyWindow(hAbortDlgWnd);
    hAbortDlgWnd = NULL;
    }

/*
 * dialog procedure for page setup
 *
 * set global variables that define how printing is to be done
 * (ie margins, headers, footers)
 */

BOOL APIENTRY PageSetupDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LONG lParam)
    {
    INT id;         /* ID of dialog edit controls */

    switch (msg)
        {
        case WM_INITDIALOG:
            for (id = ID_HEADER; id <= ID_BOTTOM; id++)
                {
                /* Allow longer strings. */
                SendDlgItemMessage(hwnd, id, EM_LIMITTEXT, PT_LEN-1, 0L);
                SetDlgItemText(hwnd, id, (LPSTR)chPageText[id - ID_HEADER]);
                }

            SendDlgItemMessage(hwnd, ID_HEADER,
#ifdef WIN32
               EM_SETSEL, 0, PT_LEN-1 );
#else
               EM_SETSEL, 0, MAKELONG(0, PT_LEN-1));
#endif
            return(TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDOK:
                    /* Check if margin values are valid. */
                    id = CheckMarginNums(hwnd);
                    if ( id <= 0)   /* invalid */
                        {
                        AlertBox(hwnd, szNN, szBadMarg, 0, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                        if (id == 0)    /* can't guess which margin is invalid */
                            return TRUE;    /* continue the dialog */
                        else if (id < 0)    /* -id is the ID of the child with invalid value */
                            {
                            SetFocus(GetDlgItem(hwnd, -id));
                            return FALSE;
                            }
                        }
                    /* store the changes made only if valid. */
                    for (id = ID_HEADER; id <= ID_BOTTOM; id++)
                        GetDlgItemText(hwnd, id, chPageText[id - ID_HEADER], PT_LEN);

                /* fall through... */

                case IDCANCEL:
					EndDialog(hwnd, ( INT )NULL);
                    break;
                }
        return(TRUE);
        }

    return(FALSE);

    lParam ;
    }


/* Check valididity of margin values specified.
 *  return TRUE if margins are valid.
 *
 *  returns  -ID_LEFT if Left margin is invalid,
 *           -ID_RIGHT if Right margin is invalid
 *           -ID_TOP   if Top margin is invalid
 *           -ID_BOTTOM if Bottom margin is invalid
 *           0/FALSE if it cannot guess the invalid margin
 */

INT CheckMarginNums(HWND hWnd)
    {
    SHORT   n;
    CHAR    *pStr;
    CHAR    szStr[PT_LEN];
    INT     Left, Right, Top, Bottom;
    HANDLE  hPrintDC;
    INT     xPixInch, yPixInch, xPrintRes, yPrintRes;

    for (n = ID_HEADER+2; n <= ID_BOTTOM; n++)
        {
        GetDlgItemText(hWnd, n, szStr, PT_LEN);
        pStr = szStr;

        while (*pStr)
            if (isdigit(*pStr) || *pStr == szDec[0])
                pStr = AnsiNext(pStr);
            else
                return (-n);
        }

    if (((INT)(hPrintDC = GetPrinterDC())) < 0)
        return TRUE;    /* can't do any range check, assume OK */

    xPrintRes = GetDeviceCaps(hPrintDC, HORZRES);
    yPrintRes = GetDeviceCaps(hPrintDC, VERTRES);
    xPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSX);
    yPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSY);

    DeleteDC(hPrintDC);

    /* margin values have int/float values. Do range check */
    GetDlgItemText(hWnd, ID_LEFT, szStr, PT_LEN);
    Left     = atopix(szStr,xPixInch);

    GetDlgItemText(hWnd, ID_RIGHT, szStr, PT_LEN);
    Right    = atopix(szStr, xPixInch);

    GetDlgItemText(hWnd, ID_TOP, szStr, PT_LEN);
    Top      = atopix(szStr, yPixInch);

    GetDlgItemText(hWnd, ID_BOTTOM, szStr, PT_LEN);
    Bottom   = atopix(szStr, yPixInch);

    /* try to guess the invalid margin */

    if (Left >= xPrintRes)
        return -ID_LEFT;            /* Left margin is invalid */
    else if (Right >= xPrintRes)
        return -ID_RIGHT;           /* Right margin is invalid */
    else if (Top >= yPrintRes)
        return -ID_TOP;             /* Top margin is invalid */
    else if (Bottom >= yPrintRes)
        return -ID_BOTTOM;          /* Bottom margin is invalid */
    else if (Left >= (xPrintRes-Right))
        return FALSE;                   /* can't guess, return FALSE */
    else if (Top >= (yPrintRes-Bottom))
        return FALSE;                   /* can't guess, return FALSE */

    return TRUE;
    }

/***************************************************************************
 * void TranslateString(char *src)
 *
 * purpose:
 *    translate a header/footer strings
 *
 * supports the following:
 *
 *    &&    insert a & char
 *    &f      current file name or (untitled)
 *    &d    date in Day Month Year
 *    &t    time
 *    &p    page number
 *    &p+num  set first page number to num
 *
 * Alignment:
 *    &l, &c, &r for left, center, right
 *
 * params:
 *    IN/OUT  src     this is the string to translate
 *
 *
 * used by:
 *    Header Footer stuff
 *
 * uses:
 *    lots of c lib stuff
 *
 ***************************************************************************/


SHORT TranslateString(CHAR *src)
    {
    CHAR         letters[15];
    CHAR         chBuff[3][80], buf[80], buf2[40];
    CHAR         *ptr, *dst=buf, *save_src=src;
    INT          page;
    INT          nAlign=1, foo, nx,
                 nIndex[3];
    struct tm    *newtime;
    time_t       long_time;

    nIndex[0]=0;
    nIndex[1]=0;
    nIndex[2]=0;

    /* Get the time we need in case we use &t. */
    time(&long_time);
    newtime=localtime(&long_time);

    LoadString(hInstanceNP, IDS_LETTERS, letters, 15);

    while (*src)   /* look at all of source */
        {
        while (*src && *src != '&')
            {
            chBuff[nAlign][nIndex[nAlign]]=*src++;
            nIndex[nAlign] += 1;
            }

        if (*src == '&')   /* is it the escape char? */
            {
            src++;

            if (*src == letters[0] || *src == letters[1])
                {                      /* &f file name (no path) */
                LoadString(hInstanceNP, IDS_NOTEPAD, buf2, 40);
                GetWindowText(hwndNP, buf, 80);
                for (foo=0; buf[foo] == buf2[foo]; foo++)
                    ptr = buf+foo;

                /* Copy to the currently aligned string. */
                strcpy((LPSTR)(chBuff[nAlign]+nIndex[nAlign]), ptr);

                /* Update insertion position. */
                nIndex[nAlign] += strlen((LPSTR)ptr);
                }
            else if (*src == letters[2] || *src == letters[3])
                {                      /* &P or &P+num page */
                src++;
                page = 0;
                if (*src == '+')       /* &p+num case */
                    {
                    src++;
                    while (isdigit(*src))
                        {
                        /* Convert to int on-the-fly*/
                        page = (10*page)+(*src)-'0';
                        src++;
                        }
                    }

                itoa(iPageNum+page, buf, 10);
                lstrcpy((LPSTR)(chBuff[nAlign]+nIndex[nAlign]), (LPSTR)buf);
                nIndex[nAlign] += lstrlen((LPSTR)buf);
                src--;
                }
            else if (*src == letters[4] || *src == letters[5])
                {                      /* &t time */
                    ptr = asctime(newtime);

                    /* extract time */
                    strncpy(chBuff[nAlign]+nIndex[nAlign], ptr+11, 8);
                    nIndex[nAlign] += 8;
                }
            else if (*src == letters[6] || *src == letters[7])
                {                      /* &d date */
                ptr = asctime(newtime);

                /* extract day month day */
                strncpy(chBuff[nAlign]+nIndex[nAlign], ptr, 11);
                nIndex[nAlign] += 11;

                /* extract year */
                strncpy(chBuff[nAlign]+nIndex[nAlign], ptr+20, 4);
                nIndex[nAlign] += 4;
                }
            else if (*src == '&')       /* quote a single & */
                {
                chBuff[nAlign][nIndex[nAlign]]='&';
                nIndex[nAlign] += 1;
                }
            /* Set the alignment for whichever has last occured. */
            else if (*src == letters[8] || *src == letters[9])  /* &c center */
                nAlign=1;
            else if (*src == letters[10] || *src == letters[11])/* &r right */
                nAlign=2;
            else if (*src == letters[12] || *src == letters[13])/* &d date */
                nAlign=0;

            src++;
            }
        }
        /* Make sure all strings are null-terminated. */
        for (nAlign=0; nAlign<3; nAlign++)
            chBuff[nAlign][nIndex[nAlign]]=0;

        /* Initialize Header/Footer string */
        for (nx=0; nx<xCharPage; nx++)
            *(szHeadFoot+nx)=32;

        /* Copy Left aligned text. */
        for (nx=0; nx < nIndex[0]; nx++)
            *(szHeadFoot+nx)=chBuff[0][nx];

        /* Calculate where the centered text should go. */
        foo=(xCharPage-nIndex[1])/2;
        for (nx=0; nx<nIndex[1]; nx++)
            *(szHeadFoot+foo+nx)=(CHAR)chBuff[1][nx];

        /* Calculate where the right aligned text should go. */
        foo=xCharPage-nIndex[2];
        for (nx=0; nx<nIndex[2]; nx++)
            *(szHeadFoot+foo+nx)=(CHAR)chBuff[2][nx];

        return (SHORT)lstrlen(szHeadFoot);
    }

/*
 * convert floating point strings (like 2.75 1.5 2) into number of pixels
 * given the number of pixels per inch
 */

INT atopix(CHAR *ptr, INT pix_per_in)
{
    CHAR *dot_ptr;
    CHAR sz[20];
    INT decimal;

    lstrcpy(sz, ptr);

    dot_ptr = strchr(sz, szDec[0]);

    if (dot_ptr)
        {
        *dot_ptr++ = 0;        /* terminate the inches */
        if (*(dot_ptr + 1) == 0)
            {
            *(dot_ptr + 1) = '0';   /* convert decimal part to hundredths */
            *(dot_ptr + 2) = 0;
            }
        decimal = ((INT)atol(dot_ptr) * pix_per_in) / 100;    /* first part */
        }
    else
        decimal = 0;        /* there is not fraction part */

    return ((INT)atol(sz) * pix_per_in) + decimal;     /* second part */
}


/* ** GetPrinterDc()
        Get Dc for current device on current output port according to
        info in win.ini.
            returns
                DC > 0 if success
                error code < 0 if failure

            error codes (to be extended):
                -1 error reading user profile
                0  CreateDC failed (memory?)
*/
HANDLE FAR GetPrinterDC(
        VOID)
{
    extern BOOL bPrinterSetupDone;
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;

    if(!PD.hDevNames){ /* Retrieve default printer if none selected. */
        PD.Flags = PD_RETURNDEFAULT|PD_PRINTSETUP;
        PrintDlg(&PD);
    }

    if (!PD.hDevNames)
        return (HANDLE)-1;

    lpDevNames  = (LPDEVNAMES)GlobalLock(PD.hDevNames);

    if (PD.hDevMode)
        lpDevMode = (LPDEVMODE)GlobalLock(PD.hDevMode);
    else
        lpDevMode = NULL;

    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *  will be NULL after GlobalLock()
     */

    PD.hDC = CreateDC((LPSTR)lpDevNames+lpDevNames->wDriverOffset,
                           (LPSTR)lpDevNames+lpDevNames->wDeviceOffset,
                           (LPSTR)lpDevNames+lpDevNames->wOutputOffset,
                           lpDevMode);
    GlobalUnlock(PD.hDevNames);

    if (PD.hDevMode)
        GlobalUnlock(PD.hDevMode);

    return PD.hDC;
}
