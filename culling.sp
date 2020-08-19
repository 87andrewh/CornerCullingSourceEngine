#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <culling>

public Plugin myinfo =
{
    name =          "CornerCulling",
    author =        "Andrew H",
    description =   "Improved anti-wallhack",
    version =       "1.0.0.0",
    url =           "https://github.com/87andrewh/CornerCulling"
};

bool g_bEnabled = true;
int ticks = 0;
int clientTeams[MAXPLAYERS + 1] = { 0 };
// Flattend visibility array. Player i can see player j
// if [i * (MAXPLAYERS + 1) + j] is true.
bool visibility[(MAXPLAYERS + 1) * (MAXPLAYERS + 1)];
float centers[(MAXPLAYERS + 2) * 3];
float yaws[MAXPLAYERS + 1];

/* Plugin Functions */
public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    return APLRes_Success;
}

public void OnPluginStart()
{
    if (!g_bEnabled)
    {
        return;
    }
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            Wallhack_Hook(i);
        }
    }
    SetCullingMap("de_dust2");
}

public void OnClientPutInServer(int client)
{
    if (g_bEnabled)
    {
        Wallhack_Hook(client);
    }
}

public void OnClientDisconnect(int client)
{
    // Stop checking clients right before they disconnect.
}

public void OnClientDisconnect_Post(int client)
{
    // Clear cache on post to ensure it's not updated again.
}

public void OnSettingsChanged(ConVar convar, char[] oldValue, char[] newValue)
{
    bool bNewValue = convar.BoolValue;

    if (bNewValue && !g_bEnabled)
    {
        Wallhack_Enable();
    }
    else if (!bNewValue && g_bEnabled)
    {
        Wallhack_Disable();
    }
}

void Wallhack_Enable()
{
    g_bEnabled = true;

    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            Wallhack_Hook(i);
        }
    }
}

void Wallhack_Disable()
{
    g_bEnabled = false;
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            Wallhack_Unhook(i);
        }
    }
}

void Wallhack_Hook(int client)
{
    SDKHook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}

void Wallhack_Unhook(int client)
{
    SDKUnhook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}

public void OnGameFrame()
{
    ticks++;
    if (ticks % 64 == 0)
    {
        //PrintToChatAll("Visibility, %d %d \n", visibility[65 + 1], visibility[65 + 2]);
    }
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            clientTeams[i] = GetClientTeam(i);
            float tempCenter[3];
            GetClientAbsOrigin(i, tempCenter);
            centers[i * 3] = tempCenter[0];
            centers[i * 3 + 1] = tempCenter[1];
            centers[i * 3 + 2] = tempCenter[2];
        }
        else
        {
            clientTeams[i] = 0;
        }
    }
    // Pass the latest location data to, and get updated visibility
    // from the CullingController extension.
    UpdateVisibility(clientTeams, centers, yaws, visibility);
}

public Action Hook_SetTransmit(int entity, int client)
{
    //PrintToServer("%d %d\n", entity, client);
    if (client == entity)
    {
        return Plugin_Continue;
    }
    else if (visibility[client * (MAXPLAYERS + 1) + entity])
    {
        return Plugin_Continue;
    }
    else
    {
        return Plugin_Handled;
    }
}

public Action OnPlayerRunCmd(int client, int& buttons, int& impulse, float vel[3], float angles[3],
        int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2])
{
    yaws[client] = angles[1];
    return Plugin_Continue;
}
