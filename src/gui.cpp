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

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "mcu.h"
#include "gui.h"
#include "lcd.h"
#include "renderer.h"

static ImFont *imgui_font;

static const int font_size_min = 6;
static const int font_size_max = 60;

static ImFont *face_font[font_size_max + 1];

void GUI_Init()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Fonts

    imgui_font = io.Fonts->AddFontDefault();
    for (int i = font_size_min; i <= font_size_max; i++)
    {
        face_font[i] = io.Fonts->AddFontFromFileTTF("Karla-Regular.ttf", i);
    }

    REND_SetupImGui();
}

void GUI_SDLEvent(SDL_Event *event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

float top_delta;

static void WindowAspectRatio(ImGuiSizeCallbackData* data)
{
    float aspect_ratio = *(float*)data->UserData;
    data->DesiredSize.y = (float)(int)(data->DesiredSize.x / aspect_ratio)
        + ImGui::GetFrameHeight();
}

static void PushFontSize(float size)
{
    int siz = (int)(size + 0.5f);
    if (siz < font_size_min)
        siz = font_size_min;
    else if (siz >= font_size_max)
        siz = font_size_max;

    ImGui::PushFont(face_font[siz]);
}

ImVec2 window_pos;
ImVec2 window_size;

void GUI_DrawNavigationTriangle(ImVec2 &center, int tri)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    float dx = window_size.y * 0.015;
    float dy = window_size.y * 0.015;
    if (tri == -1)
    {
        draw_list->AddTriangleFilled(
            ImVec2(center.x - dx, center.y),
            ImVec2(center.x + dx, center.y - dy),
            ImVec2(center.x + dx, center.y + dy),
            IM_COL32(128, 128, 128, 255)
        );
    }
    else if (tri == 1)
    {
        draw_list->AddTriangleFilled(
            ImVec2(center.x + dx, center.y),
            ImVec2(center.x - dx, center.y + dy),
            ImVec2(center.x - dx, center.y - dy),
            IM_COL32(128, 128, 128, 255)
        );
    }
}

void GUI_CircleButton(ImVec2 pos, float button_size, ImU32 *cols, int tri = 0)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImGui::SetCursorPos(pos);
    ImGui::InvisibleButton("", ImVec2(button_size, button_size));

    ImU32 col;
    if (ImGui::IsItemActive())
        col = cols[1];
    else if (ImGui::IsItemHovered())
        col = cols[2];
    else
        col = cols[0];

    ImVec2 cent(pos.x + window_pos.x + button_size * 0.5f,
        pos.y + window_pos.y + button_size * 0.5f);
    
    draw_list->AddCircleFilled(cent,
        button_size * 0.5f, col);

    if (tri)
        GUI_DrawNavigationTriangle(cent, tri);
}

void GUI_RectButton(ImVec2 pos, ImVec2 button_size, ImU32 *cols, int tri = 0)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImGui::SetCursorPos(pos);
    ImGui::InvisibleButton("", button_size);

    ImU32 col;
    if (ImGui::IsItemActive())
        col = cols[1];
    else if (ImGui::IsItemHovered())
        col = cols[2];
    else
        col = cols[0];

    ImVec2 p1(pos.x + window_pos.x,
        pos.y + window_pos.y);
    ImVec2 p2(p1.x + button_size.x,
        p1.y + button_size.y);
    ImVec2 cent(p1.x + button_size.x * 0.5f,
        p1.y + button_size.y * 0.5f);
    
    draw_list->AddRectFilled(p1, p2, col);

    if (tri)
        GUI_DrawNavigationTriangle(cent, tri);
}

void GUI_DrawFaceSC55(bool sc155 = false)
{
    ImGuiIO& io = ImGui::GetIO();

    static float sc55_aspect = 4.63f;
    static float bord_off = 20.f;
    float sc55_screenaspect = float(lcd_width - bord_off * 2.f) / float(lcd_height);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(600, -1), ImVec2(FLT_MAX, FLT_MAX),
        WindowAspectRatio, &sc55_aspect
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    ImGui::Begin("SC-55");
    window_pos = ImGui::GetWindowPos();
    window_size = ImGui::GetWindowSize();
    float frame_offset = ImGui::GetFrameHeight();
    window_size.y -= frame_offset;
    window_pos.y += frame_offset;

    // BG

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        ImVec2(2.95 * window_size.y + window_pos.x, window_pos.y),
        ImVec2(3.40 * window_size.y + window_pos.x, window_size.y + window_pos.y),
        IM_COL32_BLACK);

    bool standby = (mcu_led & (1 << MCU_LED_STANDBY)) != 0;

    draw_list->AddCircleFilled(
        ImVec2(0.56 * window_size.y + window_pos.x, 0.20 * window_size.y + window_pos.y),
        window_size.y * 0.015,
        standby ? IM_COL32(255, 95, 15, 255) : IM_COL32_BLACK);

    // LCD Screen
    ImTextureID id = REND_GetLCDTextureID(0);
    ImVec2 screen_size;
    screen_size.y = window_size.y * 0.64f;
    screen_size.x = screen_size.y * sc55_screenaspect;
    ImVec2 lcd_pos;
    lcd_pos.x = window_size.y * 1.13f;
    lcd_pos.y = window_size.y * 0.18f;

    float bord_thick = 0.04 * window_size.y;

    draw_list->AddRectFilled(
        ImVec2(lcd_pos.x + window_pos.x - bord_thick, lcd_pos.y + window_pos.y - bord_thick),
        ImVec2(lcd_pos.x + screen_size.x + window_pos.x + bord_thick, lcd_pos.y + window_pos.y + screen_size.y + bord_thick),
        IM_COL32(0, 0, 0, 80));

    lcd_pos.y += frame_offset;

    ImGui::SetCursorPos(lcd_pos);
    ImGui::Image(id, screen_size,
        ImVec2(bord_off / lcd_width, 0),
        ImVec2(1.f - bord_off / lcd_width, 1));

    // Text

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

    PushFontSize(window_size.y * 0.08f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 0.23f, window_size.y * 0.06f + frame_offset));
    ImGui::Text("POWER");
    ImGui::PopFont();

    PushFontSize(window_size.y * 0.07f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 0.19f, window_size.y * 0.27f + frame_offset));
    ImGui::Text("STANDBY");
    ImGui::PopFont();

    PushFontSize(window_size.y * 0.06f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 1.17, window_size.y * 0.08f + frame_offset));
    ImGui::Text("PART");
    ImGui::PopFont();

    PushFontSize(window_size.y * 0.06f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 1.45, window_size.y * 0.08f + frame_offset));
    ImGui::Text("INSTRUMENT");
    ImGui::PopFont();

    PushFontSize(window_size.y * 0.07f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 3.06, window_size.y * 0.14f + frame_offset));
    ImGui::Text("ALL");
    ImGui::PopFont();

    PushFontSize(window_size.y * 0.07f);
    ImGui::SetCursorPos(ImVec2(window_size.y * 3, window_size.y * 0.35f + frame_offset));
    ImGui::Text("MUTE");
    ImGui::PopFont();

    ImGui::PopStyleColor();

    // Buttons

    static ImU32 cols1[] = {
        IM_COL32(10, 10, 10, 255),
        IM_COL32(0, 0, 0, 255),
        IM_COL32(35, 35, 35, 255)
    };

    static ImU32 cols3[] = {
        IM_COL32(128, 128, 128, 255),
        IM_COL32(80, 80, 80, 255),
        IM_COL32(160, 160, 160, 255)
    };

    static ImU32 cols4[] = {
        IM_COL32(255, 95, 15, 255),
        IM_COL32(200, 50, 0, 255),
        IM_COL32(255, 130, 50, 255)
    };

    mcu_button_pressed_gui = 0;

    ImGui::PushStyleColor(ImGuiCol_Button, cols1[0]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, cols1[2]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, cols1[1]);

    ImGui::SetCursorPos(ImVec2(window_size.y * 0.2f, window_size.y * 0.14f + frame_offset));
    ImGui::PushID(MCU_BUTTON_POWER);
    ImGui::Button("", ImVec2(window_size.y * 0.3f, window_size.y * 0.12f));
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_POWER;
    ImGui::PopID();

    bool led_all = (mcu_led & (1 << MCU_LED_INST_ALL)) != 0;
    bool led_mute = (mcu_led & (1 << MCU_LED_INST_MUTE)) != 0;

    ImGui::PushID(MCU_BUTTON_INST_ALL);
    GUI_CircleButton(ImVec2(window_size.y * 3.20f, window_size.y * 0.13f + frame_offset),
        window_size.y * 0.1f,
        led_all ? cols4 : cols3);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_INST_ALL;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_INST_MUTE);
    GUI_CircleButton(ImVec2(window_size.y * 3.20f, window_size.y * 0.34f + frame_offset),
        window_size.y * 0.1f,
        led_mute ? cols4 : cols3);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_INST_MUTE;
    ImGui::PopID();

    // 0

    float posx[4] = {
        window_size.y * 3.50f,
        window_size.y * 3.75f,
        window_size.y * 4.04f,
        window_size.y * 4.29f,
    };

    ImVec2 button_size = ImVec2(window_size.y * 0.22f, window_size.y * 0.09f);

    float posy = window_size.y * 0.14f + frame_offset;

    ImGui::PushID(MCU_BUTTON_PART_L);
    GUI_CircleButton(ImVec2(window_size.y * 3.57f, posy),
        window_size.y * 0.09f,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_PART_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_PART_R);
    GUI_CircleButton(ImVec2(window_size.y * 3.81f, posy),
        window_size.y * 0.09f,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_PART_R;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_INST_L);
    GUI_RectButton(ImVec2(posx[2], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_INST_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_INST_R);
    GUI_RectButton(ImVec2(posx[3], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_INST_R;
    ImGui::PopID();

    // 1
    posy += window_size.y * 0.21;

    ImGui::PushID(MCU_BUTTON_LEVEL_L);
    GUI_RectButton(ImVec2(posx[0], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_LEVEL_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_LEVEL_R);
    GUI_RectButton(ImVec2(posx[1], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_LEVEL_R;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_PAN_L);
    GUI_RectButton(ImVec2(posx[2], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_PAN_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_PAN_R);
    GUI_RectButton(ImVec2(posx[3], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_PAN_R;
    ImGui::PopID();

    // 2
    posy += window_size.y * 0.21;

    ImGui::PushID(MCU_BUTTON_REVERB_L);
    GUI_RectButton(ImVec2(posx[0], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_REVERB_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_REVERB_R);
    GUI_RectButton(ImVec2(posx[1], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_REVERB_R;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_CHORUS_L);
    GUI_RectButton(ImVec2(posx[2], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_CHORUS_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_CHORUS_R);
    GUI_RectButton(ImVec2(posx[3], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_CHORUS_R;
    ImGui::PopID();

    // 3
    posy += window_size.y * 0.21;

    ImGui::PushID(MCU_BUTTON_KEY_SHIFT_L);
    GUI_RectButton(ImVec2(posx[0], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_KEY_SHIFT_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_KEY_SHIFT_R);
    GUI_RectButton(ImVec2(posx[1], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_KEY_SHIFT_R;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_MIDI_CH_L);
    GUI_RectButton(ImVec2(posx[2], posy),
        button_size,
        cols1,
        -1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_MIDI_CH_L;
    ImGui::PopID();

    ImGui::PushID(MCU_BUTTON_MIDI_CH_R);
    GUI_RectButton(ImVec2(posx[3], posy),
        button_size,
        cols1,
        1);
    mcu_button_pressed_gui |= ImGui::IsItemActive() << MCU_BUTTON_MIDI_CH_R;
    ImGui::PopID();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void GUI_DrawFaceDummy()
{
}

void GUI_Update()
{
    REND_BeginFrameImGui();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    GUI_DrawFaceSC55();

    ImGui::Render();
}

void GUI_Shutdown()
{
    REND_ShutdownImGui();

    ImGui::DestroyContext();
}
