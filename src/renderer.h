#pragma once
#include <SDL.h>
#include "imgui.h"

enum class rendapi {
    opengl,
    sdl2
};

bool REND_Init(rendapi api);
void REND_SetupImGui();
void REND_ShutdownImGui();
void REND_BeginFrameImGui();
int REND_SetupLCDTexture(uint32_t *ptr, int width, int height, int pitch);
void REND_UpdateLCDTexture(int id);
ImTextureID REND_GetLCDTextureID(int id);
void REND_Render();
void REND_Shutdown();
