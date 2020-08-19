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

public void OnPluginStart()
{
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
    if (g_enable_editor && (ticks % 16 == 0))
    {
        // Each edge is represented as [v1.x, v1.y, v1.z, v2.x, v2.y, v2.z]
        float edges[72]
        GetRenderedCuboid(edges);
        for (int i = 0; i < 12; i++)
        {
            float start[3];
            start[0] = edges[i * 6];
            start[1] = edges[i * 6 + 1];
            start[2] = edges[i * 6 + 2];
            float end[3];
            end[0] = edges[i * 6 + 3];
            end[1] = edges[i * 6 + 4];
            end[2] = edges[i * 6 + 5];
            doLaserBeam(1, start, end);
        }
    }
}

void doLaserBeam(int client, float start[3], float end[3]) {
    int color[4] = { 255, 0, 0, 255 };
    TE_SetupBeamPoints(
            start, end,
            g_BeamSprite, g_HaloSprite,
            0,      // start frame
            0,      // framerate
            0.4,      // life
            2.0,   // width
            2.0,   // endwidth
            0.2,      // fadelength
            0,      // amplitude
            color,
            0);     // speed???
    TE_SendToAll();
}
