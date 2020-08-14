#include "extension.h"

const int MAXPLAYERS = 64;

cell_t SetCullingMap(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);
    return 1;
}

cell_t UpdateVisibility(IPluginContext *pContext, const cell_t *params)
{
    cell_t* clientInGame;
    pContext->LocalToPhysAddr(params[1], &clientInGame);
    cell_t* centers;
    pContext->LocalToPhysAddr(params[2], &centers);
    cell_t* yaws;
    pContext->LocalToPhysAddr(params[3], &yaws);
    cell_t* visibility;
    pContext->LocalToPhysAddr(params[4], &visibility);

    for (int i = 1; i <= MAXPLAYERS; i++)
    {
        if (clientInGame[i])
        {
            for (int j = 1; j <= MAXPLAYERS; j++)
            {
                if (clientInGame[j])
                {
                    visibility[i * (MAXPLAYERS + 1) + j] = (sp_ctof(yaws[i]) > 0);
                }
            }
        }
        visibility[i * (MAXPLAYERS + 1) + i] = true;
    }
    return 1;
}

const sp_nativeinfo_t MyNatives[] = 
{
	{"SetCullingMap",	    SetCullingMap},
	{"UpdateVisibility",	UpdateVisibility},
	{NULL, NULL},
};

void Culling::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
}

Culling g_Culling;
SMEXT_LINK(&g_Culling);
