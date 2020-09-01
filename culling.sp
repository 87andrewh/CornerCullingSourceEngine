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
// It uses extra bits.
native GetRenderedCuboid(Float:edges[]);

public Plugin:myinfo =
{
	name =			"CornerCulling",
	author =		"Andrew H, Playa, old syntax by lkb",
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
new bool:visibilityFlat[(MAXPLAYERS + 1) * (MAXPLAYERS + 1)];

public APLRes:AskPluginLoad2(Handle:myself, bool:late, String:error[], err_max)
{
	return APLRes_Success;
}

public OnPluginStart()
{
	if (!g_bEnabled)
		return;
	
	for (new i = 1; i <= MaxClients; i++)
    {
        visibilityFlat[(MAXPLAYERS + 1) * i + i] = true;
        if (IsClientInGame(i))
			Wallhack_Hook(i);
    }

	AddNormalSoundHook(SoundHook)
	UpdateCullingMap();
}

stock UpdateCullingMap()
{
    new String:mapName[128];
    GetCurrentMap(mapName, 127);
	SetCullingMap(mapName);
}

public void OnPluginEnd()
{
	RemoveNormalSoundHook(SoundHook);
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
    if (   StrContains(sampleName, "footsteps") == -1
        && StrContains(sampleName, "physics") == -1)
    {
    	return Plugin_Continue;
    }
    // Discretize sounds from invisible enemies.
    for (int i = 0; i < numClients; i++)
    {
        if (!IsVisible(clients[i], source))
        {
            EmitDiscretizedSound(
                    clients[i],
                    sampleName,
                    source,
                    channel,
                    volume,
                    level,
                    pitch,
                    flags); // , cSoundEntry[], &iSeed
            // Remove the client from the array.
            for (int j = i; j < numClients-1; j++)
            {
                clients[j] = clients[j+1];
            }
            numClients--;
            i--;
        }
    }
    return Plugin_Changed;
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
    
    // Fix CSGO bad flag
    int fixedFlags = flags & ~(1 << 10);
    // Asterisk trick for CSGO 
    char fixedSampleName[128] = "*";
    StrCat(fixedSampleName, 124, sampleName);
    // Precache sound
    AddToStringTable(FindStringTable("soundprecache"), fixedSampleName);
    EmitSoundToClient(
            listener,
            sampleName,
            SOUND_FROM_WORLD,
            channel,
            level,
            fixedFlags,
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
    float angle = ArcTangent2(deltaY, deltaX);
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
	if (g_bEnabled)
		Wallhack_Hook(client);
}

public OnGameFrame()
{
    ticks++;
    if (ticks % 120 == 0)
    {
        // TODO: Remove when not testing.
	    UpdateCullingMap();
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
    UpdateVisibility(teams, eyesFlat, basesFlat, yaws, pitches, visibilityFlat);
}

public Action:Hook_SetTransmit(entity, client)
{
	if (IsVisible(client, entity))
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
