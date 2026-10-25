#include "cabinet.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    InitializeCriticalSection(&g_csMain);

    DDEInit();

    g_hInstance = hInstance;
    g_bCleanBoot = GetSystemMetrics(SM_CLEANBOOT);

    if (ShouldCreateShell())
    {
        RegisterShellServices();

        MSG msg;
        PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE);

        if(!RegisterShellHook(0, 4))
        {
            RunDateTime(TRUE);
        }
    }
}

BOOL ShouldCreateShell()
{
    TCHAR moduleFileName[MAX_PATH];
    TCHAR shellName[MAX_PATH];

    if (GetShellWindow() || FindWindow("Proxy Desktop", NULL) || FindWindow("OTClass", NULL))
        return FALSE;

    GetModuleFileName(NULL, &moduleFileName, MAX_PATH);
    GetPrivateProfileString("Boot", "Shell", PathFindFileName(&moduleFileName), shellName, MAX_PATH, "system.ini");

    // TODO: the rest of this
}

void RunDateTime(BOOL param_1)
{
    BOOL local1 = RefreshDaylightInformation(TRUE);
    if (!local1)
        return;
    
    if (param_1)
    {
        SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

        if (1 < local1)
            WinExec(szTimeDateCommand, SW_NORMAL);
    }
    else
    {
        SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

        if (1 < local1)
        {
            HKEY hkRunOnce;

            if(RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", &hkRunOnce) == ERROR_SUCCESS)
            {
                RegSetValueEx(hkRunOnce, "WarnTimeChanged", 0, REG_SZ, (BYTE*)szTimeDateCommand, strlen(szTimeDateCommand) + 1);
                RegCloseKey(hkRunOnce);
            }
        }
    }
}
