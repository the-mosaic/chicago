//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//	MODULE: 	npdate.c
//
//	PURPOSE:	Code for getting and inserting current date and time.
//
//	PLATFORMS:	Chicago
//
//	FUNCTIONS:
//
//		InsertDateTime		- Replaces current selection with current date and time
//		GetDateTimeWrapper	- Gets current date and time string
//		GetDateTime 		- Gets and formats date and time from DOS.
//
//	SPECIAL INSTRUCTIONS: N/A
//

#include "notepad.h"
#include <time.h>

#define CCHDATE         11
#define CCHTIME         12

TIME Time = {
    ':',        /* Separator character for time string */
    "AM",       /* Time string suffix for morning hours */
    "PM",       /* Time string suffix for afternoon */
    FALSE,      /* True if military time, else False */
    FALSE};     /* TRUE if Leading zero for hour */

DATE Date = {
    0,          /* 0 => MDY */
    '/',        /* '/' */
    "d/M/yy",   /* Short Date format */
    0,          /* Century - 1990 vs. 90, lead zeros for day and month */
};

static void GetDateTime(char *szTime, char *szDate);

/* ** Replace current selection with date/time string.
      if fCrlf is true, date/time string should begin with crlf */
VOID FAR InsertDateTime(
	BOOL fCrlf)
{
   CHAR szTime[CCHTIME];
   CHAR szDate[CCHDATE];
   CHAR szBuf[60];

   GetDateTime(szTime, szDate);
   if (fCrlf)
       wsprintf(szBuf, "\r\n%s  %s\r\n", (LPSTR)szTime, (LPSTR)szDate);
   else
       wsprintf(szBuf, "%s  %s", (LPSTR)szTime, (LPSTR)szDate);
        
   SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)(LPSTR)szBuf);
}

void FAR GetDateTimeWrapper(LPSTR lpszTime, LPSTR lpszDate)
{
  char szTime[CCHTIME];
  char szDate[CCHDATE];

  GetDateTime(szTime, szDate);
  if (lpszTime)
      lstrcpy(lpszTime, szTime);
  if (lpszDate)
      lstrcpy(lpszDate, szDate);
}

/* ** Get current date and time from dos, and build string showing same.
      String must be formatted according to country info obtained from
      win.ini. */
void GetDateTime(
    CHAR *szTime,
    CHAR *szDate)
{
    char szMonth[3], szDay[3], szYear[5];
    char szHour[3];
    int fPM = FALSE;

    struct tm *stime ;
    time_t    ltime ;

    time( &ltime ) ;
    stime = localtime( &ltime ) ;

    /* Get Time */
    if ( stime->tm_hour > 12 )
      fPM = TRUE ;
    if( !Time.f24Hr && stime->tm_hour > 12 )
      stime->tm_hour -= 12 ;
    if (Time.fLeadZeroHour)
        wsprintf(szHour, "%02d", stime->tm_hour);
    else
        wsprintf(szHour, "%d", stime->tm_hour);
        
    wsprintf(szTime, "%s%c%02d%s", (LPSTR)szHour, Time.chSep, stime->tm_min, 
        fPM ? (LPSTR)Time.sz2359 : (LPSTR)Time.sz1159);

    /* Get Date */
    if (Date.Flags & LEADZEROMONTH)
        wsprintf(szMonth, "%02d", stime->tm_mon+1);
    else
        wsprintf(szMonth, "%d", stime->tm_mon+1);
    
    if (Date.Flags & LEADZERODAY)
        wsprintf(szDay, "%02d", stime->tm_mday);
    else
        wsprintf(szDay, "%d", stime->tm_mday);

    wsprintf(szYear, "%04d", stime->tm_year+1900);
    if (!(Date.Flags & CENTURY))
    {
        szYear[0] = szYear[2];
        szYear[1] = szYear[3];
        szYear[2] = 0;
    }

    if (Date.Order == 0)
        wsprintf(szDate, "%s%c%s%c%s", (LPSTR)szMonth, Date.chSep, 
                (LPSTR)szDay, Date.chSep, (LPSTR)szYear);
    else if (Date.Order == 1)
        wsprintf(szDate, "%s%c%s%c%s", (LPSTR)szDay, Date.chSep, 
                (LPSTR)szMonth, Date.chSep, (LPSTR)szYear);
    else
        wsprintf(szDate, "%s%c%s%c%s", (LPSTR)szYear, Date.chSep, 
                (LPSTR)szMonth, Date.chSep, (LPSTR)szDay);


}
