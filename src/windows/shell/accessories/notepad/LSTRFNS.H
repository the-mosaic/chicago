#define OFFSET(x) ((PSTR)(LOWORD((DWORD)(x))))

extern LPSTR PASCAL StrChr(LPSTR lpStart, WORD wMatch);
extern LPSTR PASCAL StrRChr(LPSTR lpStart, LPSTR lpEnd, WORD wMatch);
extern LPSTR PASCAL StrChrI(LPSTR lpStart, WORD wMatch);
extern LPSTR PASCAL StrRChrI(LPSTR lpStart, LPSTR lpEnd, WORD wMatch);
extern INT PASCAL StrCmpN(LPSTR lpStr1, LPSTR lpStr2, INT nChar);
extern INT PASCAL StrCmpNI(LPSTR lpStr1, LPSTR lpStr2, INT nChar);
extern INT PASCAL StrCpyN(LPSTR lpDest, LPSTR lpSource, INT nBufSize);
extern INT PASCAL StrNCmp(LPSTR lpStr1, LPSTR lpStr2, INT nChar);
extern INT PASCAL StrNCmpI(LPSTR lpStr1, LPSTR lpStr2, INT nChar);
extern INT PASCAL StrNCpy(LPSTR lpDest, LPSTR lpSource, INT nChar);
extern LPSTR PASCAL StrStr(LPSTR lpFirst, LPSTR lpSrch);
extern LPSTR PASCAL StrRStr(LPSTR lpSource, LPSTR lpLast, LPSTR lpSrch);
extern LPSTR PASCAL StrStrI(LPSTR lpFirst, LPSTR lpSrch);
extern LPSTR PASCAL StrRStrI(LPSTR lpSource, LPSTR lpLast, LPSTR lpSrch);
