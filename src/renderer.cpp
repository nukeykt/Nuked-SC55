/*
 * Copyright (C) 2024 nukeykt
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "renderer.h"
#include "mcu.h"
#include "lcd.h"

rendapi rend_api = rendapi::opengl;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_GLContext context;

const int lcd_maxtextures = 8;

struct lcdtexture_t {
    bool use = false;
    GLuint gl_texture;
    SDL_Texture *sdl_texture;
    uint32_t *ptr;
    int width;
    int height;
    int pitch;
};

lcdtexture_t lcd_texture[lcd_maxtextures];

#if 0
GLuint shaders[2];
GLuint program;
GLuint vbo;
GLuint vao;
GLint texture_loc;
#endif

#define GLSLVERSION "#version 150"

static bool LoadOpenGL()
{
    const int major = 3;
    const int minor = 2;
#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    context = SDL_GL_CreateContext(window);
    if (!context)
        return false;

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

#if 0
    if (gl3wInit())
    {
        return false;
    }

    if (!gl3wIsSupported(major, minor))
    {
        return false;
    }
#endif

    return true;
}

#if 0
static bool CompileShader(GLuint &shader, GLint stage, const char *src)
{
    shader = glCreateShader(stage);
    const GLchar *shader_src = (const GLchar*)src;
    glShaderSource(shader, 1, &shader_src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint logsize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logsize);
        std::vector<GLchar> log;
        log.resize(logsize);
        glGetShaderInfoLog(shader, logsize, &logsize, log.data());
        glDeleteShader(shader);

        printf("Could not compile shader %s\n", log.data());
        return false;
    }

    return true;
}
#endif

static bool SetupOpenGL()
{
    LoadOpenGL();

#if 0
    static const char vertex_shader[] =
        GLSLVERSION "\n"
        "\n"
        "in vec4 vertex;\n"
        "out vec2 texcoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(vertex.xy, 0.0, 1.0);\n"
        "    texcoord = vertex.zw;\n"
        "}\n"
        ;
    static const char fragment_shader[] =
        GLSLVERSION "\n"
        "\n"
        "in vec2 texcoord;\n"
        "out vec4 color;\n"
        "\n"
        "uniform sampler2D tex;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    color = texture(tex, texcoord);\n"
        "}\n"
        ;

    if (!CompileShader(shaders[0], GL_VERTEX_SHADER, vertex_shader))
        return false;
    if (!CompileShader(shaders[1], GL_FRAGMENT_SHADER, fragment_shader))
        return false;

    program = glCreateProgram();
    glAttachShader(program, shaders[0]);
    glAttachShader(program, shaders[1]);

    glBindAttribLocation(program, 0, "vertex");

    texture_loc = glGetUniformLocation(program, "tex");

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint logsize;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logsize);
        std::vector<GLchar> log;
        log.resize(logsize);
        glGetProgramInfoLog(program, logsize, &logsize, log.data());
        glDeleteProgram(program);
        glDeleteShader(shaders[0]);
        glDeleteShader(shaders[1]);

        printf("Could not link shader program %s\n", log.data());
        return false;
    }

    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    static const float vertex[] = {
        -1.f, -1.f, 0.f, 1.f,
        -1.f, 1.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 0.f,
        1.f, -1.f, 1.f, 1.f
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
#endif
    return true;
}

static void DestroyOpenGL()
{
#if 0
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
#endif
}

bool REND_Init(rendapi api)
{
    rend_api = api;

    std::string title = "Nuked SC-55: ";

    title += rs_name[romset];

    int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (rend_api == rendapi::opengl)
        flags |= SDL_WINDOW_OPENGL;

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, flags);
    if (!window)
        return false;

    switch (rend_api)
    {
        case rendapi::opengl:
            if (!SetupOpenGL())
                return false;
            break;
        case rendapi::sdl2:
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
            if (!renderer)
                return false;
            break;
    }

    return true;
}

void REND_SetupImGui()
{
    switch (rend_api)
    {
        case rendapi::opengl:
            ImGui_ImplSDL2_InitForOpenGL(window, context);
            ImGui_ImplOpenGL3_Init(GLSLVERSION);
            break;
        case rendapi::sdl2:
            ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
            ImGui_ImplSDLRenderer2_Init(renderer);
            break;
    }
}

void REND_ShutdownImGui()
{
    switch (rend_api)
    {
        case rendapi::opengl:
            ImGui_ImplOpenGL3_Shutdown();
            break;
        case rendapi::sdl2:
            ImGui_ImplSDLRenderer2_Shutdown();
            break;
    }
    ImGui_ImplSDL2_Shutdown();
}

void REND_BeginFrameImGui()
{
    switch (rend_api)
    {
        case rendapi::opengl:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case rendapi::sdl2:
            ImGui_ImplSDLRenderer2_NewFrame();
            break;
    }
}

int REND_SetupLCDTexture(uint32_t* ptr, int width, int height, int pitch)
{
    int i;
    for (i = 0; i < lcd_maxtextures; i++)
    {
        if (!lcd_texture[i].use)
            break;
    }
    if (i == lcd_maxtextures)
        return -1;

    lcdtexture_t &tex = lcd_texture[i];

    tex.use = true;
    tex.ptr = ptr;
    tex.width = width;
    tex.height = height;
    tex.pitch = pitch;

    switch (rend_api)
    {
        case rendapi::opengl:
            glGenTextures(1, &tex.gl_texture);
            glBindTexture(GL_TEXTURE_2D, tex.gl_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, NULL);
            break;
        case rendapi::sdl2:
            tex.sdl_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, width, height);

            if (!tex.sdl_texture)
                return -1;
            break;
    }

    return i;
}

ImTextureID REND_GetLCDTextureID(int id)
{
    if ((unsigned)id >= (unsigned)lcd_maxtextures)
        return NULL;

    lcdtexture_t &tex = lcd_texture[id];

    if (!tex.use)
        return NULL;

    switch (rend_api)
    {
        case rendapi::opengl:
            return (ImTextureID)tex.gl_texture;
        case rendapi::sdl2:
            return (ImTextureID)tex.sdl_texture;
    }

    return NULL;
}

void REND_FreeLCDTexture(int id)
{
    if ((unsigned)id >= (unsigned)lcd_maxtextures)
        return;

    lcdtexture_t &tex = lcd_texture[id];

    if (!tex.use)
        return;

    tex.use = false;

    switch (rend_api)
    {
        case rendapi::opengl:
            glDeleteTextures(1, &tex.gl_texture);
            break;
        case rendapi::sdl2:
            SDL_DestroyTexture(tex.sdl_texture);
            break;
    }

}

void REND_UpdateLCDTexture(int id)
{
    if ((unsigned)id >= (unsigned)lcd_maxtextures)
        return;

    lcdtexture_t &tex = lcd_texture[id];
    switch (rend_api)
    {
        case rendapi::opengl:
            glBindTexture(GL_TEXTURE_2D, tex.gl_texture);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, tex.pitch);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, tex.ptr);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            break;
        case rendapi::sdl2:
            SDL_UpdateTexture(tex.sdl_texture, NULL, tex.ptr, tex.pitch * 4);
            break;
    }
}


void REND_Shutdown()
{
    for (int i = 0; i < lcd_maxtextures; i++)
    {
        REND_FreeLCDTexture(i);
    }

    switch (rend_api)
    {
        case rendapi::opengl:
            SDL_GL_DeleteContext(context);
            DestroyOpenGL();
            break;
        case rendapi::sdl2:
            SDL_DestroyRenderer(renderer);
            break;
    }

    SDL_DestroyWindow(window);
}


void REND_Render()
{
    ImGuiIO& io = ImGui::GetIO();
    switch (rend_api)
    {
        case rendapi::opengl:
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#if 0
            glViewport(0, 0, lcd_width, lcd_height);
            glUseProgram(program);
            glBindVertexArray(vao);
            glBindTexture(GL_TEXTURE_2D, lcd_texture[0].gl_texture);
            glProgramUniform1i(program, texture_loc, 0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#endif

            SDL_GL_SwapWindow(window);
            break;
        case rendapi::sdl2:
            SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
            SDL_RenderPresent(renderer);
            break;
    }
}
