#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

native SetCullingMap(const String:name[]);

// Allows the extension to calculate and update pairwise
// visibility between clients.
native UpdateVisibility(
        teams[],
        Float:eyesFlat[],
        Float:basesFlat[],
        Float:yaws[],
        Float:pitches[],
        bool:visibilityFlat[]);

// Returns the information needed to render the 12 edges
// of an occluding cuboid.
// Each edge is represented as [v1.x, v1.y, v1.z, v2.x, v2.y, v2.z]
// Yes, I realize that this uses extra bits.
native GetRenderedCuboid(Float:edges[]);

public Plugin:myinfo =
{
	name =			"CornerCulling",
	author =		"Andrew H / old syntax by lkb",
	description = 	"Improved anti-wallhack",
	version =		"1.0.0.0",
	url = 			"https://github.com/87andrewh/CornerCulling"
};

new bool:g_bEnabled = true;
new ticks = 0;
new teams[MAXPLAYERS + 1];
// Flattened arrays of client spatial information.
new Float:eyesFlat[(MAXPLAYERS + 2) * 3];
new Float:basesFlat[(MAXPLAYERS + 2) * 3];
new Float:yaws[MAXPLAYERS + 1];
new Float:pitches[MAXPLAYERS + 1];
// Flattend visibility array. Player i can see player j if
// [i * (MAXPLAYERS + 1) + j] is true.
new bool:visibility[(MAXPLAYERS + 1) * (MAXPLAYERS + 1)];

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	return APLRes_Success;
}

public OnPluginStart()
{
	if (!g_bEnabled)
		return;
	
	for (new i = 1; i <= MaxClients; i++)
		if (IsClientInGame(i))
			Wallhack_Hook(i);
		
	SetCullingMap("de_dust2");
}

public OnClientPutInServer(client)
{
	if (g_bEnabled)
		Wallhack_Hook(client);
}

public OnGameFrame()
{
	ticks++;
    // TODO: Remove when not testing.
    if (ticks % 120 == 0)
    {
	    SetCullingMap("de_dust2");
    }
	for (new i = 1; i <= MaxClients; i++)
	{
		if (IsClientConnected(i) && IsClientInGame(i))
		{
			teams[i] = GetClientTeam(i);
			decl Float:temp[3];
			GetClientEyePosition(i, temp);
			eyesFlat[i * 3] = temp[0];
			eyesFlat[i * 3 + 1] = temp[1];
			eyesFlat[i * 3 + 2] = temp[2];
			GetClientAbsOrigin(i, temp);
			basesFlat[i * 3] = temp[0];
			basesFlat[i * 3 + 1] = temp[1];
			basesFlat[i * 3 + 2] = temp[2];
			GetClientEyeAngles(i, temp);
            yaws[i] = temp[1];
            pitches[i] = temp[0];
		}
		else
			teams[i] = 0;
	}
	// Pass the latest location data to, and get updated visibility
	// from the CullingController extension.
	UpdateVisibility(teams, eyesFlat, basesFlat, yaws, pitches, visibility);
}

public Action:Hook_SetTransmit(entity, client)
{
	if (client == entity)
		return Plugin_Continue;
	else if (visibility[client * (MAXPLAYERS + 1) + entity])
		return Plugin_Continue;
	else
		return Plugin_Handled;
}

stock Wallhack_Enable()
{
	g_bEnabled = true;

	for (new i = 1; i <= MaxClients; i++)
		if (IsClientInGame(i))
			Wallhack_Hook(i);
}

stock Wallhack_Disable()
{
	g_bEnabled = false;
	
	for (new i = 1; i <= MaxClients; i++)
		if (IsClientInGame(i))
			Wallhack_Unhook(i);
}

stock Wallhack_Hook(client)
{
	SDKHook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}

stock Wallhack_Unhook(client)
{
	SDKUnhook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}
