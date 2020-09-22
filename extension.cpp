#include "extension.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include "CornerCulling/CullingIO.h"

CullingController cullingController = CullingController();

// Initializes the C++ code.
cell_t SetCullingMap(IPluginContext *pContext, const cell_t *params)
{
	char *mapName;
	pContext->LocalToString(params[1], &mapName);
    cullingController.tickRate = params[2];
    cullingController.maxLookahead = params[3];
    cullingController.BeginPlay(mapName);
    return 1;
}

// Interface between SM plugin and C++ occlusion culling code.
cell_t UpdateVisibility(IPluginContext *pContext, const cell_t *params)
{
    cell_t* teams;
    pContext->LocalToPhysAddr(params[1], &teams);
    cell_t* intEyesFlat;
    pContext->LocalToPhysAddr(params[2], &intEyesFlat);
    cell_t* intBasesFlat;
    pContext->LocalToPhysAddr(params[3], &intBasesFlat);
    cell_t* intYaws;
    pContext->LocalToPhysAddr(params[4], &intYaws);
    cell_t* intPitches;
    pContext->LocalToPhysAddr(params[5], &intPitches);
    cell_t* intSpeeds;
    pContext->LocalToPhysAddr(params[6], &intSpeeds);
    cell_t* visibility;
    pContext->LocalToPhysAddr(params[7], &visibility);

    float eyesFlat[(MAX_CHARACTERS + 2) * 3];
    float basesFlat[(MAX_CHARACTERS + 2) * 3];
    float yaws[MAX_CHARACTERS + 1];
    float pitches[MAX_CHARACTERS + 1];
    float speeds[MAX_CHARACTERS + 1];
    for (int i = 1; i <= MAX_CHARACTERS; i++)
    {
        basesFlat[i * 3] = sp_ctof(intBasesFlat[i * 3]);
        basesFlat[i * 3 + 1] = sp_ctof(intBasesFlat[i * 3 + 1]);
        basesFlat[i * 3 + 2] = sp_ctof(intBasesFlat[i * 3 + 2]);
        eyesFlat[i * 3] = sp_ctof(intEyesFlat[i * 3]);
        eyesFlat[i * 3 + 1] = sp_ctof(intEyesFlat[i * 3 + 1]);
        eyesFlat[i * 3 + 2] = sp_ctof(intEyesFlat[i * 3 + 2]);
        yaws[i] = sp_ctof(intYaws[i]);
        pitches[i] = sp_ctof(intPitches[i]);
        speeds[i] = sp_ctof(intSpeeds[i]);
    }

    cullingController.UpdateCharacters(
        teams, eyesFlat, basesFlat, yaws, pitches, speeds);
    cullingController.Tick();
    for (int i = 1; i <= MAX_CHARACTERS; i++)
    {
        if (teams[i] != 0)
        {
            for (int j = 1; j <= MAX_CHARACTERS; j++)
            {
                if (teams[j] != 0)
                {
                    visibility[i * (MAX_CHARACTERS + 1) + j] =
                        cullingController.IsVisible(i, j);
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
    char* mapName;
    pContext->LocalToString(params[1], &mapName);

    cell_t* edges;
    pContext->LocalToPhysAddr(params[2], &edges);

    std::vector<vec3> firstObject = GetFirstCuboidVertices(mapName);
    int pairs[12][2]
    {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    for (int i = 0; i < 12; i++)
    {
        edges[i * 6 + 0] = sp_ftoc(firstObject[pairs[i][0]].x);
        edges[i * 6 + 1] = sp_ftoc(firstObject[pairs[i][0]].y);
        edges[i * 6 + 2] = sp_ftoc(firstObject[pairs[i][0]].z);
        edges[i * 6 + 3] = sp_ftoc(firstObject[pairs[i][1]].x);
        edges[i * 6 + 4] = sp_ftoc(firstObject[pairs[i][1]].y);
        edges[i * 6 + 5] = sp_ftoc(firstObject[pairs[i][1]].z);
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
