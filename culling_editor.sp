#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <culling_editor>

int g_enable_editor = true;
int g_BeamSprite = -1;
int g_HaloSprite = -1;
int ticks = 0;

public Plugin myinfo =
{
    name =          "CullingEditor",
    author =        "Andrew H",
    description =   "Occluder editor",
    version =       "1.0.0.0",
    url =           "https://github.com/87andrewh"
};

/* Plugin Functions */
public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
    return APLRes_Success;
}

public void OnMapStart()
{
    // $ignorez 1
	g_BeamSprite = PrecacheModel("sprites/bluelaser2.vmt", true);
	g_HaloSprite = PrecacheModel("sprites/glow01.vmt", true);

    // not csgo
	//g_BeamSprite = PrecacheModel("sprites/laser.vmt", true);
	//g_HaloSprite = PrecacheModel("sprites/halo01.vmt", true);

}

public void OnGameFrame()
{
    ticks++;
    float start[3];
    float end[3];
    if (g_enable_editor && (ticks % 8 == 0))
    {
        // Each edge is represented as [v1.x, v1.y, v1.z, v2.x, v2.y, v2.z]
        float edges[72]
        GetRenderedCuboid(edges);
        for (int i = 0; i < 12; i++)
        {
            start[0] = edges[i * 6];
            start[1] = edges[i * 6 + 1];
            start[2] = edges[i * 6 + 2];
            end[0] = edges[i * 6 + 3];
            end[1] = edges[i * 6 + 4];
            end[2] = edges[i * 6 + 5];
            //PrintToChatAll("%.2f %.2f", start[0], start[1]);
            renderBeam(1, start, end);
        }
    }
}

void renderBeam(int client, float start[3], float end[3]) {
    int color[4] = { 255, 0, 0, 255 };
    TE_SetupBeamPoints(
            start, end,
            g_BeamSprite, g_HaloSprite,
            0,      // start frame
            0,      // framerate
            0.4,      // life
            0.3,   // width
            0.3,   // endwidth
            0.2,      // fadelength
            0,      // amplitude
            color,
            0);     // speed???
    TE_SendToAll();
}


public Action:OnPlayerRunCmd(
        client, &buttons, &impulse, Float:vel[3], Float:angles[3],
        &weapon, &subtype, &cmdnum, &tickcount, &seed, mouse[2])
{
    if (   (client == 1)
        && (buttons & IN_ATTACK))
    {
        float pos[3];
        pos = GetAimPosition();
        PrintToConsole(1, "%.2f %.2f %.2f\n", pos[0], pos[1], pos[2]);
    }
	return Plugin_Continue;
}

public bool TraceFilter_NoClients(int entity, int contentsMask, any data)
{
	return (entity != data && !IsClientInGame(data));
}

float[] GetAimPosition()
{
	float pos[3];
	GetClientEyePosition(1, pos);
	float angles[3];
	GetClientEyeAngles(1, angles);
	TR_TraceRayFilter(
            pos, angles, MASK_SHOT, 
            RayType_Infinite, TraceFilter_NoClients, 1);
	if(TR_DidHit())
	{
		float end[3];
		TR_GetEndPosition(end);
        return end;
	}
	return pos;
}
