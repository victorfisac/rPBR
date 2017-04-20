/***********************************************************************************
*
*   rPBR [ui] - Physically Based Rendering user interface drawing functions
*
*   DEPENDENCIES:
*       raylib (@raysan5) for raylib framework
*       raygui (@raysan5) for user interface functions
*
*   LICENSE: zlib/libpng
*
*   rPBR is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software:
*
*   Copyright (c) 2017 Victor Fisac
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
***********************************************************************************/

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include "raylib.h"         // Required for shapes and text drawing
#include "raygui.h"         // Required for user interface functions

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         MAX_TEXTURES                    7

#define         UI_MENU_WIDTH                   225
#define         UI_MENU_BORDER                  5
#define         UI_MENU_PADDING                 15
#define         UI_TEXTURES_PADDING             230
#define         UI_TEXTURES_SIZE                180
#define         UI_COLOR_BACKGROUND             (Color){ 5, 26, 36, 255 }
#define         UI_COLOR_SECONDARY              (Color){ 245, 245, 245, 255 }
#define         UI_COLOR_PRIMARY                (Color){ 234, 83, 77, 255 }
#define         UI_TEXT_SIZE_H2                 20
#define         UI_TEXT_SIZE_H3                 10
#define         UI_TEXT_MATERIAL_TITLE          "Material properties"
#define         UI_TEXT_TEXTURES_TITLE          "Textures"
#define         UI_TEXT_DRAG_HERE               "DRAG TEXTURE HERE"

//----------------------------------------------------------------------------------
// Structs and enums
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
int texTitleLength = 0;
int titlesLength[MAX_TEXTURES] = { 0 };
const char *textureTitles[MAX_TEXTURES] = {
    "Albedo",
    "Tangent normals",
    "Metalness",
    "Roughness",
    "Ambient occlusion",
    "Emission",
    "Parallax"
};

//----------------------------------------------------------------------------------
// Functions Declaration
//----------------------------------------------------------------------------------
void InitInterface(void);                                                                      // Initialize interface texts lengths
void DrawInterface(int width, int height, int scroll, Texture2D *textures, int texCount);      // Draw interface based on current window dimensions
void DrawTextureMap(int id, Texture2D texture, Vector2 position);                              // Draw interface PBR texture or alternative text

//----------------------------------------------------------------------------------
// Functions Definition
//----------------------------------------------------------------------------------
// Initialize interface texts lengths
void InitInterface(void)
{
    texTitleLength = MeasureText(UI_TEXT_TEXTURES_TITLE, UI_TEXT_SIZE_H2);
    titlesLength[0] = MeasureText(textureTitles[0], UI_TEXT_SIZE_H3);
    titlesLength[1] = MeasureText(textureTitles[1], UI_TEXT_SIZE_H3);
    titlesLength[2] = MeasureText(textureTitles[2], UI_TEXT_SIZE_H3);
    titlesLength[3] = MeasureText(textureTitles[3], UI_TEXT_SIZE_H3);
    titlesLength[4] = MeasureText(textureTitles[4], UI_TEXT_SIZE_H3);
    titlesLength[5] = MeasureText(textureTitles[5], UI_TEXT_SIZE_H3);
    titlesLength[6] = MeasureText(textureTitles[6], UI_TEXT_SIZE_H3);

    // TODO: work in progress
}

// Draw interface based on current window dimensions
void DrawInterface(int width, int height, int scrolling, Texture2D *textures, int texCount)
{
    int padding = scrolling;

    // Draw interface right menu background
    DrawRectangle(width - UI_MENU_WIDTH, 0, UI_MENU_WIDTH, height, UI_COLOR_BACKGROUND);
    DrawRectangle(width - UI_MENU_WIDTH - UI_MENU_BORDER, 0, UI_MENU_BORDER, height, UI_COLOR_PRIMARY);

    // Draw textures title
    DrawText(UI_TEXT_TEXTURES_TITLE, width - UI_MENU_WIDTH + UI_MENU_WIDTH/2 - texTitleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);

    // Draw textures
    padding = scrolling + UI_MENU_PADDING*2 + UI_MENU_PADDING*2.5f + UI_MENU_PADDING*1.25f;
    for (int i = 0; i < texCount; i++)
    {
        Vector2 pos = { width - UI_MENU_WIDTH + UI_MENU_WIDTH/2, padding + UI_MENU_WIDTH*0.375f - UI_TEXT_SIZE_H3/2 + i*UI_TEXTURES_PADDING };
        DrawTextureMap(i, textures[i], pos);
    }
}

// Draw interface PBR texture or alternative text
void DrawTextureMap(int id, Texture2D texture, Vector2 position)
{
    Rectangle rect = { position.x - UI_TEXTURES_SIZE/2, position.y - UI_TEXTURES_SIZE/2, UI_TEXTURES_SIZE, UI_TEXTURES_SIZE };
    DrawRectangle(rect.x - UI_MENU_BORDER, rect.y - UI_MENU_BORDER, rect.width + UI_MENU_BORDER*2, rect.height + UI_MENU_BORDER*2, UI_COLOR_PRIMARY);
    DrawText(textureTitles[id], position.x - titlesLength[id]/2, position.y - UI_TEXT_SIZE_H3/2 - rect.height*0.6f, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);

    // Draw PBR texture or display help message
    if (texture.id != 0) DrawTexturePro(texture, (Rectangle){ 0, 0, texture.width, texture.height }, rect, (Vector2){ 0, 0 }, 0, WHITE);
    else
    {
        DrawRectangleRec(rect, UI_COLOR_SECONDARY);
        DrawText(UI_TEXT_DRAG_HERE, position.x - MeasureText(UI_TEXT_DRAG_HERE, UI_TEXT_SIZE_H3)/2, position.y, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    }
}
