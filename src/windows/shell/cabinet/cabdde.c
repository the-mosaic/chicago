#include "cabinet.h"

HDDEDATA DDECallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData)
{
    switch(wType)
    {
        case XTYP_REQUEST:
            if (hsz1 == g_hszProgman || hsz2 == g_hszAppProps)
            {

            }
        case XTYP_CONNECT:
            return DDEHandleConnection(hsz1, hsz2);
        case XTYP_ADVDATA:
            return DDE_FACK;
        case XTYP_WILDCONNECT:
            return DDEHandleWildConnect();
        case XTYP_EXECUTE:
            break;
        case XTYP_CONNECT_CONFIRM:
            return DDEHandleConnectConfirm(hConv);
    }
}

void DDEInit()
{
    if (g_bDdeInitialized)
        return;
    
    g_atomProgman = GlobalAddAtom("PROGMAN");
    DdeInitialize(&g_ddeInstance, DDECallback, CBF_FAIL_POKES | CBF_FAIL_ADVISES, 0);

    g_hszProgman       = DdeCreateStringHandle(g_ddeInstance, "Progman", CP_WINANSI);
    g_hszProgman2      = DdeCreateStringHandle(g_ddeInstance, "Progman", CP_WINANSI);
    g_hszStar          = DdeCreateStringHandle(g_ddeInstance, "*", CP_WINANSI);
    g_hszShell         = DdeCreateStringHandle(g_ddeInstance, "Shell", CP_WINANSI);
    g_hszAppProps      = DdeCreateStringHandle(g_ddeInstance, "AppProperties", CP_WINANSI);
    g_hszFolders       = DdeCreateStringHandle(g_ddeInstance, "Folders", CP_WINANSI);

    DdeNameService(g_ddeInstance, g_hszFolders, NULL, DNS_REGISTER);

    g_bDdeInitialized = TRUE;
}

HDDEDATA DDEHandleRequest(HSZ hsz, HCONV hConv)
{
    
}

// request handlers
HDDEDATA DDEHandleConnection(HSZ hsz1, HSZ hsz2)
{
    if ((hsz1 == g_hszProgman && hsz2 == g_hszProgman2)
        || (hsz1 == g_hszAppProps && hsz2 == g_hszShell)
        || (hsz1 == g_hszAppProps && hsz2 == g_hszFolders))
        return DDE_FACK;
    
    return DDE_FNOTPROCESSED;
}

HDDEDATA DDEHandleWildConnect()
{
    HSZPAIR hszPairs[4];

    hszPairs[0].hszSvc = g_hszProgman2;
    hszPairs[0].hszTopic = g_hszProgman;
    hszPairs[1].hszSvc = g_hszShell;
    hszPairs[1].hszTopic = g_hszAppProps;
    hszPairs[2].hszSvc = g_hszFolders;
    hszPairs[2].hszTopic = g_hszAppProps;
    hszPairs[3].hszSvc = NULL;
    hszPairs[3].hszTopic = NULL;

    return DdeCreateDataHandle(g_ddeInstance, hszPairs, sizeof(hszPairs), 0, NULL, 1, 0);
}

HDDEDATA DDEHandleConnectConfirm(HCONV hConv)
{
    
}

void RegisterShellServices()
{
    DdeNameService(g_ddeInstance, g_hszProgman2, NULL, DNS_REGISTER);
    DdeNameService(g_ddeInstance, g_hszShell, NULL, DNS_REGISTER);
}
