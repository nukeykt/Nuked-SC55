#pragma once
#include <SDL.h>

enum class rendapi {
    opengl,
    sdl2
};

bool REND_Init(rendapi api);
int REND_SetupLCDTexture(uint32_t *ptr, int width, int height, int pitch);
void REND_UpdateLCDTexture(int);
void REND_Render();
void REND_Shutdown();
