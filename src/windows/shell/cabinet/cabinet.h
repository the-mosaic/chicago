#include <Windows.h>

// Global variables
HINSTANCE   g_hInstance;
// ...
DWORD       g_ddeInstance;
HSZ         g_hszProgman;
HSZ         g_hszProgman2;
HSZ         g_hszStar;
HSZ         g_hszShell;
HSZ         g_hszAppProps;
HSZ         g_hszFolders;

BOOL g_bDdeInitialized;
ATOM g_atomProgman;

CRITICAL_SECTION g_csMain;

BOOL g_bCleanBoot;

// cabdde.c
HDDEDATA DDECallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData);
void DDEInit();

HDDEDATA DDEHandleRequest(HSZ hsz, HCONV hConv);
HDDEDATA DDEHandleConnection(HSZ hsz1, HSZ hsz2);
HDDEDATA DDEHandleWildConnect();
HDDEDATA DDEHandleConnectConfirm(HCONV hConv);
void RegisterShellServices();

// cabinit.c
BOOL ShouldCreateShell();
void RunDateTime(BOOL param_1);

// constants
const CHAR* szTimeDateCommand = "rundll32 shell32.dll,Control_RunDLL timedate.cpl,,/m";