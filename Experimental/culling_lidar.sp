#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

File out;

int g_Sprite = 0;

float minX = -3500.0;
float maxX =  1000.0;
float minY = -2000.0;
float maxY =  2800.0;
float minZ = -240.0;
float maxZ =  280.0;

float smallStep = 8.0;
float bigStep = 80.0;

float g_x;
float g_y;
float g_z;

bool scannedAlongX = false;
bool scannedAlongY = false;
bool scannedAlongZ = false;

public Plugin myinfo =
{
    name =          "CullingLidar",
    author =        "Andrew H",
    description =   "Point cloud generator",
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
    out = OpenFile("test.xyz", "a");

    g_x = minX;
    g_y = minY;
    g_z = minZ;
}

public void OnMapStart()
{
	char buffer[PLATFORM_MAX_PATH];
	Format( buffer, sizeof( buffer ), "decals/paint/%s.vmt", "paint_red");
	g_Sprite = PrecachePaint(buffer);
}

public void OnGameFrame()
{
    if (!scannedAlongX)
    {
        ScanAlongX();
    }
    else if (!scannedAlongY)
    {
        ScanAlongY();
    }
    else if (!scannedAlongZ)
    {
        ScanAlongZ();
    }
}

public void ScanAlongX()
{
    float start[3];
    float end[3];
    
    if (g_x >= maxX)
    {
        scannedAlongX = true;
        return;
    }

    start[0] = g_x;
    end[0] = g_x + bigStep;

    for (float y = minY; y <= maxY; y += smallStep)
    {
        start[1] = y;
        end[1] = y;

        for (float z = minZ; z <= maxZ; z += smallStep)
        {
            start[2] = z;
            end[2] = z;
       
            WriteTraceEnd(start, end);
           
            // Scan both directions
            end[0] = g_x - bigStep;
            WriteTraceEnd(start, end);
            // Swap back
            end[0] = g_x + bigStep;
        }
    }
    g_x += bigStep;
}

// Is code duplication the lesser evil?
public void ScanAlongY()
{
    float start[3];
    float end[3];
    
    if (g_y >= maxY)
    {
        scannedAlongY = true;
        return;
    }

    start[1] = g_y;
    end[1] = g_y + bigStep;

    for (float x = minX; x <= maxX; x += smallStep)
    {
        start[0] = x;
        end[0] = x;

        for (float z = minZ; z <= maxZ; z += smallStep)
        {
            start[2] = z;
            end[2] = z;
       
            WriteTraceEnd(start, end);

            // Scan both directions
            end[1] = g_y - bigStep;
            WriteTraceEnd(start, end);
            // Swap back
            end[1] = g_y + bigStep;
        }
    }
    g_y += bigStep;
}

public void ScanAlongZ()
{
    float start[3];
    float end[3];
    
    if (g_z >= maxZ)
    {
        scannedAlongZ = true;
        return;
    }

    start[2] = g_z;
    end[2] = g_z + bigStep;

    for (float x = minX; x <= maxX; x += smallStep)
    {
        start[0] = x;
        end[0] = x;

        for (float y = minY; y <= maxY; y += smallStep)
        {
            start[1] = y;
            end[1] = y;
       
            WriteTraceEnd(start, end);
           
            // Scan both directions
            end[2] = g_z - bigStep;
            WriteTraceEnd(start, end);
            // Swap back
            end[2] = g_z + bigStep;
        }
    }
    g_z += bigStep;
}

public void WriteTraceEnd(float start[3], float end[3])
{
    TR_TraceRay(start, end, MASK_VISIBLE, RayType_EndPoint);

    if (TR_StartSolid())
        return;

    if(!TR_DidHit())
        return;
   
    float hit[3]
    TR_GetEndPosition(hit);

    if (IsNaN(hit[0] + hit[1] + hit[2]))
        return;

    float normal[3]
    TR_GetPlaneNormal(INVALID_HANDLE, normal);

    WriteFileLine(
            out,
            "%.2f %.2f %.2f %.2f %.2f %.2f",
            hit[0], hit[1], hit[2],
            normal[0], normal[1], normal[2]);
}

public bool IsNaN(float f)
{
    return f != f;
}

public Action:OnPlayerRunCmd(
        client, &buttons, &impulse, Float:vel[3], Float:angles[3],
        &weapon, &subtype, &cmdnum, &tickcount, &seed, mouse[2])
{
    if (client != 1)
	    return Plugin_Continue;

    if (!(buttons & IN_ATTACK))
	    return Plugin_Continue;

    // I experimented with manual scanning, but the automatic full scan seemed to work.
    // Perhaps I should remove the corresponding dead code.
    // ManualScan();

	return Plugin_Continue;
}

public void ManualScan() {
	float start[3];
	GetClientEyePosition(1, start);

	float eyeAngles[3];
    GetClientEyeAngles(1, eyeAngles);

	float sampleAngles[3];
    for (float dPitch = -20.0; dPitch < 20.0; dPitch += 2.0)
    {
        for (float dYaw = -20.0; dYaw < 20.0; dYaw += 2.0)
        {
            sampleAngles[0] = eyeAngles[0] + dPitch;
            sampleAngles[1] = eyeAngles[1] + dYaw;
            WriteTraceAngle(start, sampleAngles);
        }
    }
}

void WriteTraceAngle(const float start[3], const float angles[3])
{
	TR_TraceRayFilter(
            start, angles, MASK_VISIBLE, RayType_Infinite,
            TraceFilter_NoClients, 1);
	if(TR_DidHit())
	{
		float end[3];
		TR_GetEndPosition(end);
        AddPaint(end)
        WriteFileLine(out, "%.2f %.2f %.2f", end[0], end[1], end[2]);
	}
}

public bool TraceFilter_NoClients(int entity, int contentsMask, any data)
{
	return (entity != data && !IsClientInGame(data));
}

// Code to visualize manually scanned surfaces.
// From hmmmmm's Paint plugin:
// https://forums.alliedmods.net/showthread.php?p=2541664
void AddPaint(float pos[3])
{
	TE_SetupWorldDecal(pos);
	TE_SendToAll();
}

stock void TE_SetupWorldDecal( const float vecOrigin[3])
{    
    TE_Start( "World Decal" );
    TE_WriteVector( "m_vecOrigin", vecOrigin);
    TE_WriteNum( "m_nIndex", g_Sprite );
}

int PrecachePaint( char[] filename )
{
	char tmpPath[PLATFORM_MAX_PATH];
	Format( tmpPath, sizeof( tmpPath ), "materials/%s", filename );
	AddFileToDownloadsTable( tmpPath );
	
	return PrecacheDecal( filename, true );
}
