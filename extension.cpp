#include "extension.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <math.h>
#include "CornerCulling/CullingIO.h"

CullingController cullingController = CullingController();

// Initializes the C++ code.
// NOTE: currently doesn't set the map
cell_t SetCullingMap(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

    cullingController.BeginPlay();

    return 1;
}

// Interface between SM plugin and C++ occlusion culling code.
cell_t UpdateVisibility(IPluginContext *pContext, const cell_t *params)
{
    cell_t* teams;
    pContext->LocalToPhysAddr(params[1], &teams);
    cell_t* intCentersFlat;
    pContext->LocalToPhysAddr(params[2], &intCentersFlat);
    cell_t* intYaws;
    pContext->LocalToPhysAddr(params[3], &intYaws);
    cell_t* visibility;
    pContext->LocalToPhysAddr(params[4], &visibility);

    float yaws[MAX_CHARACTERS + 1];
    float centersFlat[(MAX_CHARACTERS + 2) * 3];
    for (int i = 1; i <= MAX_CHARACTERS; i++)
    {
        yaws[i] = sp_ctof(intYaws[i]);
        centersFlat[i * 3] = sp_ctof(intCentersFlat[i * 3]);
        centersFlat[i * 3 + 1] = sp_ctof(intCentersFlat[i * 3 + 1]);
        centersFlat[i * 3 + 2] = sp_ctof(intCentersFlat[i * 3 + 2]);
    }

    cullingController.UpdateCharacters(teams, centersFlat, yaws);
    cullingController.Tick();
    for (int i = 1; i <= MAX_CHARACTERS; i++)
    {
        if (teams[i] != 0)
        {
            for (int j = 1; j <= MAX_CHARACTERS; j++)
            {
                if (teams[j] != 0)
                {
                    visibility[i * (MAX_CHARACTERS + 1) + j] = cullingController.IsVisible(i, j);
                }
            }
        }
    }
    return 1;
}

// Grabs and renders a cuboid from a text file.
// Only used for editing.
cell_t GetRenderedCuboid(IPluginContext* pContext, const cell_t* params)
{
    cell_t* edges;
    pContext->LocalToPhysAddr(params[1], &edges);

    std::ifstream cuboidFile;
    cuboidFile.open("RenderedCuboid.txt");
    if (!cuboidFile)
    {
        char buf[256];
        GetCurrentDirectoryA(256, buf);
        printf(buf);
        return 1;
    }

    std::vector<vec3> extents = TextToCuboidVertices(cuboidFile);
    int pairs[12][2]
    {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    for (int i = 0; i < 12; i++)
    {
        edges[i * 6 + 0] = sp_ftoc(extents[pairs[i][0]].x);
        edges[i * 6 + 1] = sp_ftoc(extents[pairs[i][0]].y);
        edges[i * 6 + 2] = sp_ftoc(extents[pairs[i][0]].z);
        edges[i * 6 + 3] = sp_ftoc(extents[pairs[i][1]].x);
        edges[i * 6 + 4] = sp_ftoc(extents[pairs[i][1]].y);
        edges[i * 6 + 5] = sp_ftoc(extents[pairs[i][1]].z);
    }
	return 1;
}

const sp_nativeinfo_t MyNatives[] = 
{
	{"SetCullingMap",	    SetCullingMap},
	{"UpdateVisibility",	UpdateVisibility},
	{"GetRenderedCuboid",	GetRenderedCuboid},
	{NULL, NULL},
};

void Culling::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
}

Culling g_Culling;
SMEXT_LINK(&g_Culling);
