#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

native SetCullingMap(
        const String:name[],
        int tickRate,
        int maxLookahead);

// Allows the extension to calculate and update pairwise
// visibility between clients.
native UpdateVisibility(
        teams[],
        Float:eyesFlat[],
        Float:basesFlat[],
        Float:yaws[],
        Float:pitches[],
        Float:speeds[],
        bool:visibilityFlat[]);

public Plugin:myinfo =
{
	name =			"CornerCulling",
	author =		"Andrew H, lkb, Playa",
	description = 	"Improved anti-wallhack",
	version =		"1.0.0.0",
	url = 			"https://github.com/87andrewh/CornerCulling"
};

new bool:enabled = true;
new ticks = 0;
new teams[MAXPLAYERS + 1];
// Flattened arrays of client spatial information.
new Float:eyesFlat[(MAXPLAYERS + 2) * 3];
new Float:basesFlat[(MAXPLAYERS + 2) * 3];
new Float:yaws[MAXPLAYERS + 1];
new Float:pitches[MAXPLAYERS + 1];
new Float:speeds[MAXPLAYERS + 1];
// Flattend visibility array. Player i can see player j if
// [i * (MAXPLAYERS + 1) + j] is true.
new bool:visibilityFlat[(MAXPLAYERS + 1) * (MAXPLAYERS + 1)];

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	return APLRes_Success;
}

public OnPluginStart()
{
    if (!enabled)
    	return;
    // Initialize visibility to true, and hook clients
    for (new i = 1; i <= MaxClients; i++)
    {
        visibilityFlat[(MAXPLAYERS + 1) * i + i] = true;
        if (IsClientInGame(i))
    		Wallhack_Hook(i);
    }
    AddNormalSoundHook(SoundHook)
    // Try to load the map's culling data files
    UpdateCullingMap();
}

stock UpdateCullingMap()
{
    new String:mapName[128];
    GetCurrentMap(mapName, 127);
    int tickRate = RoundToNearest(1.0 / GetTickInterval());
    // Culling system lookahead (millisceonds).
    // A low value enforces strict culling, but lag may cause popping.
    // A high value will grant a greater advantage to wallhackers.
    ConVar maxLookahead = FindConVar("culling_maxlookahead");
    if (maxLookahead != null)
    {
        SetCullingMap(mapName, tickRate, GetConVarInt(maxLookahead));
    }
    else
    {
        SetCullingMap(mapName, tickRate, 110);
    }
}

public void OnPluginEnd()
{
	RemoveNormalSoundHook(SoundHook);
}

stock Enable()
{
	enabled = true;

	for (new i = 1; i <= MaxClients; i++)
		if (IsClientInGame(i))
			Wallhack_Hook(i);
}

stock Disable()
{
	enabled = false;
	
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

// Discretizes (reduces position accuracy of) sounds from enemies
// that are not visible to a client.
public Action:SoundHook(
        clients[64],
        &numClients,
        String:sampleName[PLATFORM_MAX_PATH],
        &source,
        &channel,
        &Float:volume,
        &level,
        &pitch,
        &flags)
{
    if (source < 1 || source > MaxClients)
    {
    	return Plugin_Continue;
    }
    // Fix CSGO bad flag
    int fixedFlags = flags & ~(1 << 10);
    // Asterisk trick for CSGO 
    char fixedSampleName[PLATFORM_MAX_PATH] = "*";
    StrCat(fixedSampleName, PLATFORM_MAX_PATH - 2, sampleName);
    // Precache sound
    AddToStringTable(FindStringTable("soundprecache"), fixedSampleName);
    for (int i = 0; i < numClients; i++)
    {
        // Discretize sounds from invisible enemies.
        if (!IsVisible(clients[i], source))
        {
            EmitDiscretizedSound(
                    clients[i],
                    fixedSampleName,
                    source,
                    channel,
                    volume,
                    level,
                    pitch,
                    fixedFlags);
        }
        else
        {
            EmitSoundToClient(
                    clients[i],
                    fixedSampleName,
                    source,
                    channel,
                    level,
                    fixedFlags,
                    volume,
                    pitch);
        }
    }
    return Plugin_Stop;
}

/*
* Emits a discretized sound to the listener.
*/
stock EmitDiscretizedSound(
        int listener,
        String:sampleName[PLATFORM_MAX_PATH],
        int source,
        int channel,
        float volume,
        int level,
        int pitch,
        int flags)
{
    float soundLocation[3];
    GetEntPropVector(source, Prop_Send, "m_vecOrigin", soundLocation);
    float listenerLocation[3];
    GetClientAbsOrigin(listener, listenerLocation);
    DiscretizeLocation(soundLocation, listenerLocation);
    EmitSoundToClient(
            listener,
            sampleName,
            SOUND_FROM_WORLD,
            channel,
            level,
            flags,
            volume,
            pitch,
            _,
            soundLocation);
}
// Discretizes the location relative to the origin.
// Divides the XY plane centered at the origin into radial slices (pizza)
// Rounds the angle from the origin to the location, as well as the distance
void DiscretizeLocation(float location[3], float origin[3])
{
    // Round Z coordinate to the nearest multiple of 20.
    location[2] = RoundToNearestK(location[2], 20.0);
    float deltaX = location[0] - origin[0];
    float deltaY = location[1] - origin[1];
    float r = SquareRoot(deltaX * deltaX + deltaY * deltaY);
    // Round XY distance to the nearest multiple of 20.
    r = RoundToNearestK(r, 20.0);
    float angle = ArcTangent2(deltaY, deltaX);
    // Round XY angle to nearest 20 degree (0.35 radian) slice.
    angle = RoundToNearestK(angle, 0.35);
    location[0] = origin[0] + r * Cosine(angle);
    location[1] = origin[1] + r * Sine(angle);
}

// Returns if the client can see the entity
public bool IsVisible(client, entity)
{
    return (visibilityFlat[client * (MAXPLAYERS + 1) + entity]);
}

// Rounds a float to the nearest multiple of k
float RoundToNearestK(float num, float k)
{
    return RoundToNearest(num/k) * k;
}

public OnClientPutInServer(client)
{
	if (enabled)
		Wallhack_Hook(client);
}

public OnGameFrame()
{
    ticks++;
    // DEBUG
    if (ticks % 100 == 0)
    {
        UpdateCullingMap();
    }
    for (new i = 1; i <= MaxClients; i++)
	{
		if (IsClientConnected(i) && IsClientInGame(i))
		{
            teams[i] = GetClientTeam(i);
            decl Float:tmp[3];
            GetClientEyePosition(i, tmp);
            eyesFlat[i * 3] = tmp[0];
            eyesFlat[i * 3 + 1] = tmp[1];
            eyesFlat[i * 3 + 2] = tmp[2];
            GetClientAbsOrigin(i, tmp);
            basesFlat[i * 3] = tmp[0];
            basesFlat[i * 3 + 1] = tmp[1];
            basesFlat[i * 3 + 2] = tmp[2];
            GetClientEyeAngles(i, tmp);
            yaws[i] = tmp[1];
            pitches[i] = tmp[0];
            GetEntPropVector(i, Prop_Data, "m_vecAbsVelocity", tmp);
            speeds[i] = GetVectorLength(tmp, false);
		}
		else
        {
			teams[i] = 0;
        }
	}
	// Pass the latest location data to, and get updated visibility
	// from the CullingController extension.
    UpdateVisibility(
            teams,
            eyesFlat,
            basesFlat,
            yaws,
            pitches,
            speeds,
            visibilityFlat);
}

public Action:Hook_SetTransmit(entity, client)
{
	if (IsVisible(client, entity))
		return Plugin_Continue;
	return Plugin_Handled;
}
