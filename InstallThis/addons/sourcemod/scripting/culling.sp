#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <culling>

public Plugin:myinfo =
{
	name =			"Culling",
	author =		"Andrew H, Playa, old syntax by lkb",
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

ConVar maxLookahead = null;
bool isFFA = false;

public APLRes:AskPluginLoad2(
        Handle:myself, bool:late, String:error[], err_max)
{
	return APLRes_Success;
}

public OnPluginStart()
{
    if (!enabled)
    	return;

    for (new i = 1; i <= MaxClients; i++)
    {
        visibilityFlat[(MAXPLAYERS + 1) * i + i] = true;
        if (IsClientInGame(i))
    		Wallhack_Hook(i);
    }
    AddNormalSoundHook(SoundHook)

    isFFA = GetConVarInt(FindConVar("mp_teammates_are_enemies")) == 1;
    maxLookahead = CreateConVar(
            "culling_maxlookahead",
            "120",
            "ms to look ahead");
    AutoExecConfig(true, "culling");

    UpdateCullingMap();
}

public void OnMapStart() {
    UpdateCullingMap();
}

public void OnPluginEnd()
{
	RemoveNormalSoundHook(SoundHook);
}

public on

public OnClientPutInServer(client)
{
	if (enabled)
		Wallhack_Hook(client);
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

stock UpdateCullingMap()
{
    new String:mapName[128];
    GetCurrentMap(mapName, 127);

    int tickRate = RoundToNearest(1.0 / GetTickInterval());
    // Culling system lookahead (millisceonds).
    // A low value enforces strict culling,
    // but laggy clients may experience popping.
    // A high value will grant a greater advantage to wallhackers.
    if (maxLookahead != null)
    {
        SetCullingMap(mapName, tickRate, GetConVarInt(maxLookahead));
    }
    else
    {
        SetCullingMap(mapName, tickRate, 110);
    }
}

public OnGameFrame()
{
    ticks++;
    decl Float:tmp[3];
    for (new i = 1; i <= MaxClients; i++)
	{
		if (IsClientConnected(i) && IsClientInGame(i))
		{
            teams[i] = GetClientTeam(i);
            if (isFFA && (teams[i] > 1))
            {
                teams[i] = i + 2; 
            }

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
    	return Plugin_Continue;

    // Workarounds for "headshot too loud" bug.
    if (StrContains(sampleName, "bhit_helmet") != -1)
    	return Plugin_Continue;
    if (StrContains(sampleName, "headshot") != -1)
    	return Plugin_Continue;

    // Fix CSGO bad flag
    int fixedFlags = flags & ~(1 << 10);

    // Asterisk trick for CSGO sounds
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
            // Fixes "self footsteps too loud" bug
            float fixedVolume = volume;
            if (clients[i] == source)
                fixedVolume = volume * 0.45;

            EmitSoundToClient(
                    clients[i],
                    fixedSampleName,
                    source,
                    channel,
                    level,
                    fixedFlags,
                    fixedVolume,
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
    
    // Round XY distance to the nearest multiple of 20.
    float deltaX = location[0] - origin[0];
    float deltaY = location[1] - origin[1];
    float r = SquareRoot(deltaX * deltaX + deltaY * deltaY);
    r = RoundToNearestK(r, 20.0);
    
    // Round XY angle to nearest 20 degree (0.35 radian) slice.
    float angle = RoundToNearestK(ArcTangent2(deltaY, deltaX), 0.35);

    location[0] = origin[0] + r * Cosine(angle);
    location[1] = origin[1] + r * Sine(angle);
}

// Rounds a float to the nearest multiple of k
float RoundToNearestK(float num, float k)
{
    return RoundToNearest(num/k) * k;
}

public Action:Hook_SetTransmit(entity, client)
{
	return IsVisible(client, entity) ? Plugin_Continue : Plugin_Handled;
}

// Returns if the client can see the entity
public bool IsVisible(client, entity)
{
    // Let SourceTV see everyone
    if (IsClientSourceTV(client))
        return true;
    
    // Let spectators see everyone
    if (GetClientTeam(client) == 1)
        return true;

    return (visibilityFlat[client * (MAXPLAYERS + 1) + entity]);
}

stock Wallhack_Hook(client)
{
	SDKHook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}

stock Wallhack_Unhook(client)
{
	SDKUnhook(client, SDKHook_SetTransmit, Hook_SetTransmit);
}
