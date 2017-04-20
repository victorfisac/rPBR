/***********************************************************************************
*
*   rPBR [core] - Physically Based Rendering 3D drawing functions for raylib
*
*   FEATURES:
*       - Phyiscally based rendering for any 3D model.
*       - Metalness/Roughness PBR workflow.
*       - Split-Sum Approximation for specular reflection calculations.
*       - Support for normal mapping, parallax mapping and emission mapping.
*       - Simple and easy-to-use implementation code.
*       - Multi-material scene supported.
*       - Point and directional lights supported.
*       - Internal shader values and locations points handled automatically.
*
*   NOTES:
*       Physically based rendering shaders paths are set up by default
*       Remember to call UnloadMaterialPBR and UnloadEnvironment to deallocate required memory and unload textures
*       Physically based rendering requires OpenGL 3.3 or ES2
*
*   DEPENDENCIES:
*       stb_image (Sean Barret) for images loading (JPEG, PNG, BMP, HDR)
*       GLAD for OpenGL extensions loading (3.3 Core profile)
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

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         UI_MENU_PADDING             15
#define         UI_MENU_RIGHT_WIDTH         250
#define         UI_MENU_TEXTURES_Y          250
#define         UI_COLOR_BACKGROUND         (Color){ 5, 26, 36, 255 }
#define         UI_COLOR_SECONDARY          (Color){ 255, 255, 255, 255 }
#define         UI_COLOR_PRIMARY            (Color){ 234, 83, 77, 255 }
#define         UI_TEXT_SIZE_H2             20
#define         UI_TEXT_MATERIAL_TITLE      "Material properties"
#define         UI_TEXT_TEXTURES_TITLE      "Textures"

//----------------------------------------------------------------------------------
// Structs and enums
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
int matTitleLength = 0;
int texTitleLength = 0;

//----------------------------------------------------------------------------------
// Functions Declaration
//----------------------------------------------------------------------------------
void InitInterface(void);                       // Initialize interface texts lengths
void DrawInterface(int width, int height);      // Draw interface based on current window dimensions

//----------------------------------------------------------------------------------
// Functions Definition
//----------------------------------------------------------------------------------
// Initialize interface texts lengths
void InitInterface(void)
{
    matTitleLength = MeasureText(UI_TEXT_MATERIAL_TITLE, UI_TEXT_SIZE_H2);
    texTitleLength = MeasureText(UI_TEXT_TEXTURES_TITLE, UI_TEXT_SIZE_H2);

    // TODO: work in progress
}

// Draw interface based on current window dimensions
void DrawInterface(int width, int height)
{
    DrawRectangle(width - UI_MENU_RIGHT_WIDTH, 0, UI_MENU_RIGHT_WIDTH, height, UI_COLOR_BACKGROUND);
    DrawText(UI_TEXT_MATERIAL_TITLE, width - UI_MENU_RIGHT_WIDTH + UI_MENU_RIGHT_WIDTH/2 - matTitleLength/2, UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);
    DrawText(UI_TEXT_TEXTURES_TITLE, width - UI_MENU_RIGHT_WIDTH + UI_MENU_RIGHT_WIDTH/2 - texTitleLength/2, UI_MENU_TEXTURES_Y, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);

    // TODO: work in progress
}
