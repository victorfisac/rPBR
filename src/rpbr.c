/***********************************************************************************
*
*   rPBR - Physically based rendering viewer for raylib
*
*   FEATURES:
*       - Load OBJ models and texture images in real-time by drag and drop.
*       - Use right mouse button to rotate lighting.
*       - Use middle mouse button to rotate and pan camera.
*       - Use interface to adjust material, textures, render and effects settings (space bar - display/hide interface).
*       - Press F1-F11 to switch between different render modes.
*       - Press F12 or use Screenshot button to capture a screenshot and save it as PNG file.
*
*   Use the following line to compile:
*
*   gcc -o $(NAME_PART).exe $(FILE_NAME) -s icon\rpbr_icon -IC:\raylib\raylib\src -LC:\raylib\raylib\release\win32
*   -LC:\raylib\raylib\src\external\glfw3\lib\win32 -LC:\raylib\raylib\src\external\openal_soft\lib\win32\ -lraylib
*   -lglfw3 -lopengl32 -lgdi32 -lopenal32 -lwinmm -std=c99 -Wl,--subsystem,windows -Wl,-allow-multiple-definition
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
#include "raylib.h"                         // Required for raylib framework
#include "raymath.h"                        // Required for matrix, vectors and other math functions
#include "pbrcore.h"                        // Required for lighting, environment and drawing functions

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                         // Required for user interface functions

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         WINDOW_WIDTH                1440                // Default screen width during program initialization
#define         WINDOW_HEIGHT               810                 // Default screen height during program initialization
#define         WINDOW_MIN_WIDTH            960                 // Resizable window minimum width
#define         WINDOW_MIN_HEIGHT           540                 // Resizable window minimum height

#define         KEY_NUMPAD_SUM              43                  // Decimal value of numeric pad sum button
#define         KEY_NUMPAD_SUBTRACT         45                  // Decimal value of numeric pad subtract button

#define         PATH_ICON                   "resources/textures/rpbr_icon.png"                      // Path to rPBR icon to replace default window icon
#define         PATH_MODEL                  "resources/models/cerberus.obj"                         // Path to default OBJ model to load
#define         PATH_TEXTURES_HDR           "resources/textures/hdr/pinetree.hdr"                   // Path to default HDR environment map to load
#define         PATH_TEXTURES_ALBEDO        "resources/textures/cerberus/cerberus_albedo.png"       // Path to default PBR albedo texture
#define         PATH_TEXTURES_NORMALS       "resources/textures/cerberus/cerberus_normals.png"      // Path to default PBR tangent normals texture
#define         PATH_TEXTURES_METALNESS     "resources/textures/cerberus/cerberus_metalness.png"    // Path to default PBR metalness texture
#define         PATH_TEXTURES_ROUGHNESS     "resources/textures/cerberus/cerberus_roughness.png"    // Path to default PBR roughness texture
// #define      PATH_TEXTURES_AO            "resources/textures/cerberus/cerberus_ao.png"           // Path to default PBR ambient occlusion texture (NO AVAILABLE)
// #define      PATH_TEXTURES_EMISSION      "resources/textures/cerberus/cerberus_emission.png"     // Path to default PBR emission texture  (NO AVAILABLE)
// #define      PATH_TEXTURES_HEIGHT        "resources/textures/cerberus/cerberus_height.png"       // Path to default PBR parallax height texture  (NO AVAILABLE)
#define         PATH_SHADERS_POSTFX_VS      "resources/shaders/postfx.vs"                           // Path to screen post-processing effects vertex shader
#define         PATH_SHADERS_POSTFX_FS      "resources/shaders/postfx.fs"                           // Path to screen post-processing effects fragment shader
#define         PATH_GUI_STYLE              "resources/rpbr_gui.style"                              // Path to rPBR custom GUI style

#define         MAX_TEXTURES                7                   // Max number of supported textures in a PBR material
#define         MAX_RENDER_SCALES           5                   // Max number of available render scales (RenderScale type)
#define         MAX_RENDER_MODES            11                  // Max number of render modes to switch (RenderMode type)
#define         MAX_CAMERA_TYPES            2                   // Max number of camera modes to switch (CameraType type)
#define         MAX_SUPPORTED_EXTENSIONS    5                   // Max number of supported image file extensions (JPG, PNG, BMP, TGA and PSD)
#define         MAX_SCROLL                  850                 // Max mouse wheel for interface scrolling

#define         SCROLL_SPEED                50                  // Interface scrolling speed
#define         CAMERA_FOV                  60.0f               // Camera global field of view
#define         MODEL_SCALE                 1.75f               // Model scale transformation for rendering

#define         LIGHT_SPEED                 0.1f                // Light rotation input speed
#define         LIGHT_DISTANCE              3.5f                // Light distance from center of world
#define         LIGHT_HEIGHT                1.0f                // Light height from center of world
#define         LIGHT_RADIUS                0.05f               // Light gizmo drawing radius
#define         LIGHT_OFFSET                0.03f               // Light gizmo drawing radius when mouse is over

#define         CUBEMAP_SIZE                1024                // Cubemap texture size
#define         IRRADIANCE_SIZE             32                  // Irradiance map from cubemap texture size
#define         PREFILTERED_SIZE            256                 // Prefiltered HDR environment map texture size
#define         BRDF_SIZE                   512                 // BRDF LUT texture map size

#define         UI_MENU_WIDTH               225
#define         UI_MENU_BORDER              5
#define         UI_MENU_PADDING             15
#define         UI_TEXTURES_PADDING         230
#define         UI_TEXTURES_SIZE            180
#define         UI_SLIDER_WIDTH             250
#define         UI_SLIDER_HEIGHT            20
#define         UI_CHECKBOX_SIZE            20
#define         UI_BUTTON_WIDTH             120
#define         UI_BUTTON_HEIGHT            35
#define         UI_LIGHT_WIDTH              200
#define         UI_LIGHT_HEIGHT             140
#define         UI_COLOR_BACKGROUND         (Color){ 5, 26, 36, 255 }
#define         UI_COLOR_SECONDARY          (Color){ 245, 245, 245, 255 }
#define         UI_COLOR_PRIMARY            (Color){ 234, 83, 77, 255 }
#define         UI_TEXT_SIZE_H1             30
#define         UI_TEXT_SIZE_H2             20
#define         UI_TEXT_SIZE_H3             10
#define         UI_TEXT_TEXTURES_TITLE      "Textures"
#define         UI_TEXT_DRAG_HERE           "DRAG TEXTURE HERE"
#define         UI_TEXT_MATERIAL_TITLE      "Material Properties"
#define         UI_TEXT_RENDER_TITLE        "Render Settings"
#define         UI_TEXT_RENDER_SCALE        "Render Scale"
#define         UI_TEXT_RENDER_MODE         "Render Mode"
#define         UI_TEXT_RENDER_EFFECTS      "Screen Effects"
#define         UI_TEXT_CAMERA_MODE         "Camera Mode"
#define         UI_TEXT_EFFECTS_TITLE       "Screen Effects"
#define         UI_TEXT_EFFECTS_FXAA        "   Antialiasing"
#define         UI_TEXT_EFFECTS_BLOOM       "   Bloom"
#define         UI_TEXT_EFFECTS_VIGNETTE    "   Vignette"
#define         UI_TEXT_EFFECTS_WIRE        "   Wireframe"
#define         UI_TEXT_DRAW_LOGO           "   Show Logo"
#define         UI_TEXT_DRAW_LIGHTS         "   Show Lights"
#define         UI_TEXT_DRAW_GRID           "   Show Grid"
#define         UI_TEXT_BUTTON_SS           "Screenshot (F12)"
#define         UI_TEXT_BUTTON_HELP         "Help (H)"
#define         UI_TEXT_BUTTON_RESET        "Reset Scene (R)"
#define         UI_TEXT_BUTTON_CLOSE_HELP   "Close Help"
#define         UI_TEXT_CONTROLS            "Controls"
#define         UI_TEXT_CREDITS             "Credits"
#define         UI_TEXT_CREDITS_VICTOR      "- Victor Fisac"
#define         UI_TEXT_CREDITS_RAMON       "[Thanks to Ramon Santamaria]"
#define         UI_TEXT_TITLE               "raylib Physically Based Renderer"
#define         UI_TEXT_CONTROLS_01         "- RMB for lighting rotation."
#define         UI_TEXT_CONTROLS_02         "- MMB (+ ALT) for camera panning (and rotation)."
#define         UI_TEXT_CONTROLS_03         "- From F1 to F11 to display each shading mode."
#define         UI_TEXT_CONTROLS_04         "- Drag and drop models (OBJ) and textures in real time."
#define         UI_TEXT_CREDITS_WEB         "Visit www.victorfisac.com for more information about the tool."
#define         UI_TEXT_DELETE              "CLICK TO DELETE TEXTURE"
#define         UI_TEXT_DISPLAY             "Use SPACE BAR to display/hide interface"
#define         UI_TEXT_LIGHT_ENABLED       "   Enabled"
#define         UI_TEXT_LIGHT_R             "R"
#define         UI_TEXT_LIGHT_G             "G"
#define         UI_TEXT_LIGHT_B             "B"

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { DEFAULT, ALBEDO, NORMALS, METALNESS, ROUGHNESS, AMBIENT_OCCLUSION, EMISSION, LIGHTING, FRESNEL, IRRADIANCE, REFLECTIVITY } RenderMode;
typedef enum { RENDER_SCALE_0_5X, RENDER_SCALE_1X, RENDER_SCALE_2X, RENDER_SCALE_4X, RENDER_SCALE_8X } RenderScale;
typedef enum { CAMERA_TYPE_FREE, CAMERA_TYPE_ORBITAL } CameraType;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
int texTitleLength = 0;                                                 // Interface textures menu title length
int matTitleLength = 0;                                                 // Interface material menu title length
int renderTitleLength = 0;                                              // Interface render settings title length
int renderScaleLength = 0;                                              // Interface render scale title length
int renderModeLength = 0;                                               // Interface render mode title length
int renderEffectsLength = 0;                                            // Interface screen effects title length
int cameraModeLength = 0;                                               // Interface camera mode title length
int effectsTitleLength = 0;                                             // Interface screen effects title length
int fxaaLength = 0;                                                     // Interface FXAA effect enabled title length
int bloomLength = 0;                                                    // Interface bloom effect enabled title length
int vignetteLength = 0;                                                 // Interface vignette effect enabled title length
int wireLength = 0;                                                     // Interface wireframe enabled title length
int logoLength = 0;                                                     // Interface draw logo enabled title length
int controlsLength = 0;                                                 // Interface controls title length
int creditsLength = 0;                                                  // Interface credits title length
int titleLength = 0;                                                    // Interface main title length
int creditsVictorLength = 0;                                            // Interface credits Victor title length
int creditsRamonLength = 0;                                             // Interface credits Ramon title length
int dragLength = 0;                                                     // Interface textures drag title length
int creditsWebLength = 0;                                               // Interface credits web title length
int deleteLength = 0;                                                   // Interface textures delete title length
int displayLength = 0;                                                  // Interface interface display help message length
int titlesLength[MAX_TEXTURES] = { 0 };                                 // Interface material properties lengths
const char *imageExtensions[MAX_SUPPORTED_EXTENSIONS] = {               // Supported image extensions for texture loading
    ".jpg",
    ".png",
    ".bmp",
    ".tga",
    ".psd"
};
const char *textureTitles[MAX_TEXTURES] = {                             // Interface textures properties titles
    "Albedo",
    "Tangent normals",
    "Metalness",
    "Roughness",
    "Ambient occlusion",
    "Emission",
    "Parallax"
};
const char *renderScalesTitles[MAX_RENDER_SCALES] = {                   // Interface render scale settings titles
    "0.5X",
    "1.0X",
    "2.0X",
    "4.0X",
    "8.0X"
};
const char *renderModesTitles[MAX_RENDER_MODES] = {                     // Interface render modes settings titles
    "PBR (default)",
    "Albedo",
    "Tangent Normals",
    "Metalness",
    "Roughness",
    "Ambient Occlusion",
    "Emission",
    "Lighting",
    "Fresnel",
    "Irradiance (GI)",
    "Reflectivity"
};
const char *cameraTypesTitles[MAX_CAMERA_TYPES] = {                     // Interface camera type titles
    "Free Camera",
    "Orbital Camera",
};
const float renderScales[MAX_RENDER_SCALES] = {                         // Availables render scales
    0.5f,
    1.0f,
    2.0f,
    4.0f,
    8.0f
};

// Interface settings values
RenderMode renderMode = DEFAULT;
RenderScale renderScale = RENDER_SCALE_2X;
CameraType cameraType = CAMERA_TYPE_FREE;
CameraType lastCameraType = CAMERA_TYPE_FREE;
Texture2D textures[7] = { 0 };
int selectedLight = -1;
bool resetScene = false;
bool drawGrid = false;
bool drawWire = false;
bool drawLights = true;
bool drawSkybox = true;
bool drawLogo = true;
bool drawUI = true;
bool drawHelp = false;
bool enabledFxaa = true;
bool enabledBloom = true;
bool enabledVignette = true;

// Scene resources values
Model model = { 0 };
Environment environment = { 0 };
MaterialPBR matPBR = { 0 };
Camera camera = { 0 };

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
void InitInterface(void);                                                                       // Initialize interface texts lengths
void DrawLight(Light light, bool over);                                                         // Draw a light gizmo based on light attributes
void DrawInterface(Vector2 size, int scrolling);                                                // Draw interface based on current window dimensions
void DrawLightInterface(Light *light);                                                          // Draw specific light settings interface
void DrawTextureMap(int id, Texture2D texture, Vector2 position);                               // Draw interface PBR texture or alternative text

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //------------------------------------------------------------------------------
    // Enable V-Sync and window resizable state
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "rPBR - Physically based rendering 3D model viewer");
    InitInterface();

    // Change default window icon
    Image icon = LoadImage(PATH_ICON);
    Texture2D iconTex = LoadTextureFromImage(icon);
    SetWindowIcon(icon);
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);

    // Define render settings states
    drawUI = true;
    bool canMoveCamera = true;
    bool overUI = false;
    int scrolling = 0;

    // Initialize lighting rotation
    int mousePosX = 0;
    int lastMousePosX = 0;
    float lightAngle = 0.0f;

    // Define the camera to look into our 3d world, its mode and model drawing position
    camera.position = (Vector3){ 3.5f, 3.0f, 3.5f };
    camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = CAMERA_FOV;
    SetCameraMode(camera, (((cameraType == CAMERA_TYPE_FREE) ? CAMERA_FREE : CAMERA_ORBITAL)));

    // Define environment attributes
    environment = LoadEnvironment(PATH_TEXTURES_HDR, CUBEMAP_SIZE, IRRADIANCE_SIZE, PREFILTERED_SIZE, BRDF_SIZE);

    // Load external resources
    model = LoadModel(PATH_MODEL);
    matPBR = SetupMaterialPBR(environment, (Color){ 255, 255, 255, 255 }, 255, 255);
#if defined(PATH_TEXTURES_ALBEDO)
    SetMaterialTexturePBR(&matPBR, PBR_ALBEDO, LoadTexture(PATH_TEXTURES_ALBEDO));
    SetTextureFilter(matPBR.albedo.bitmap, FILTER_BILINEAR);
    textures[PBR_ALBEDO] = matPBR.albedo.bitmap;
#endif
#if defined(PATH_TEXTURES_NORMALS)
    SetMaterialTexturePBR(&matPBR, PBR_NORMALS, LoadTexture(PATH_TEXTURES_NORMALS));
    SetTextureFilter(matPBR.normals.bitmap, FILTER_BILINEAR);
    textures[PBR_NORMALS] = matPBR.normals.bitmap;
#endif
#if defined(PATH_TEXTURES_METALNESS)
    SetMaterialTexturePBR(&matPBR, PBR_METALNESS, LoadTexture(PATH_TEXTURES_METALNESS));
    SetTextureFilter(matPBR.metalness.bitmap, FILTER_BILINEAR);
    textures[PBR_METALNESS] = matPBR.metalness.bitmap;
#endif
#if defined(PATH_TEXTURES_ROUGHNESS)
    SetMaterialTexturePBR(&matPBR, PBR_ROUGHNESS, LoadTexture(PATH_TEXTURES_ROUGHNESS));
    SetTextureFilter(matPBR.roughness.bitmap, FILTER_BILINEAR);
    textures[PBR_ROUGHNESS] = matPBR.roughness.bitmap;
#endif
#if defined(PATH_TEXTURES_AO)
    SetMaterialTexturePBR(&matPBR, PBR_AO, LoadTexture(PATH_TEXTURES_AO));
    SetTextureFilter(matPBR.ao.bitmap, FILTER_BILINEAR);
    textures[PBR_AO] = matPBR.ao.bitmap;
#endif
#if defined(PATH_TEXTURES_EMISSION)
    SetMaterialTexturePBR(&matPBR, PBR_EMISSION, LoadTexture(PATH_TEXTURES_EMISSION));
    SetTextureFilter(matPBR.emission.bitmap, FILTER_BILINEAR);
    textures[PBR_EMISSION] = matPBR.emission.bitmap;
#endif
#if defined(PATH_TEXTURES_HEIGHT)
    SetMaterialTexturePBR(&matPBR, PBR_HEIGHT, LoadTexture(PATH_TEXTURES_HEIGHT));
    SetTextureFilter(matPBR.height.bitmap, FILTER_BILINEAR);
    textures[PBR_HEIGHT] = matPBR.height.bitmap;
#endif
    Shader fxShader = LoadShader(PATH_SHADERS_POSTFX_VS, PATH_SHADERS_POSTFX_FS);

    // Set up materials and lighting
    Material material = { 0 };
    material.shader = matPBR.env.pbrShader;
    model.material = material;

    // Get shaders required locations
    int shaderModeLoc = GetShaderLocation(environment.pbrShader, "renderMode");
    int fxResolutionLoc = GetShaderLocation(fxShader, "resolution");
    int enabledFxaaLoc = GetShaderLocation(fxShader, "enabledFxaa");
    int enabledBloomLoc = GetShaderLocation(fxShader, "enabledBloom");
    int enabledVignetteLoc = GetShaderLocation(fxShader, "enabledVignette");

    // Define lights attributes
    Light lights[MAX_LIGHTS] = {
        CreateLight(LIGHT_POINT, (Vector3){ LIGHT_DISTANCE, LIGHT_HEIGHT, 0.0f }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 255, 0, 0, 255 }, environment),
        CreateLight(LIGHT_POINT, (Vector3){ 0.0f, LIGHT_HEIGHT, LIGHT_DISTANCE }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 0, 255, 0, 255 }, environment),
        CreateLight(LIGHT_POINT, (Vector3){ -LIGHT_DISTANCE, LIGHT_HEIGHT, 0.0f }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 0, 0, 255, 255 }, environment),
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 0.0f, LIGHT_HEIGHT*2.0f, -LIGHT_DISTANCE }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 255, 0, 255, 255 }, environment)
    };
    int totalLights = GetLightsCount();

    // Create a render texture for antialiasing post-processing effect and initialize Bloom shader
    RenderTexture2D fxTarget = LoadRenderTexture(GetScreenWidth()*renderScales[renderScale], GetScreenHeight()*renderScales[renderScale]);

    // Send resolution values to post-processing shader
    float resolution[2] = { (float)GetScreenWidth()*renderScales[renderScale], (float)GetScreenHeight()*renderScales[renderScale] };
    SetShaderValue(fxShader, fxResolutionLoc, resolution, 2);
    SetShaderValue(environment.skyShader, environment.skyResolutionLoc, resolution, 2);

    // Set our game to run at 60 frames-per-second
    SetTargetFPS(60);
    //------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //--------------------------------------------------------------------------
        // Update mouse collision states
        overUI = CheckCollisionPointRec(GetMousePosition(), (Rectangle){ GetScreenWidth() - UI_MENU_WIDTH, 0, UI_MENU_WIDTH, GetScreenHeight() });

        // Check if current camera type changed since last frame
        if (lastCameraType != cameraType)
        {
            // Reset camer values and update camera mode to switch properly
            camera.position = (Vector3){ 3.5f, 3.0f, 3.5f };
            camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.fovy = CAMERA_FOV;
            SetCameraMode(camera, (((cameraType == CAMERA_TYPE_FREE) ? CAMERA_FREE : CAMERA_ORBITAL)));
            lastCameraType = cameraType;
        }

        // Check if scene needs to be reset
        if (resetScene || IsKeyPressed(KEY_R))
        {
            // Reset camera values and return to free mode
            camera.position = (Vector3){ 3.5f, 3.0f, 3.5f };
            camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.fovy = CAMERA_FOV;
            cameraType = CAMERA_TYPE_FREE;
            SetCameraMode(camera, CAMERA_FREE);

            // Reset current light angle and lights positions
            lightAngle = 0.0f;
            for (int i = 0; i < totalLights; i++)
            {
                float angle = lightAngle + 90*i;
                lights[i].position.x = LIGHT_DISTANCE*cosf(angle*DEG2RAD);
                lights[i].position.z = LIGHT_DISTANCE*sinf(angle*DEG2RAD);

                // Send lights values to environment PBR shader
                UpdateLightValues(environment, lights[i]);
            }

            // Reset scene reset state
            resetScene = false;
        }

        // Check if a file is dropped
        if (IsFileDropped())
        {
            int fileCount = 0;
            char **droppedFiles = GetDroppedFiles(&fileCount);

            // Check file extensions for drag-and-drop
            if (IsFileExtension(droppedFiles[0], ".hdr"))
            {
                UnloadEnvironment(environment);
                environment = LoadEnvironment(droppedFiles[0], CUBEMAP_SIZE, IRRADIANCE_SIZE, PREFILTERED_SIZE, BRDF_SIZE);
                resolution[0] = (float)GetScreenWidth()*renderScales[renderScale];
                resolution[1] = (float)GetScreenHeight()*renderScales[renderScale];
                SetShaderValue(environment.skyShader, environment.skyResolutionLoc, resolution, 2);
                matPBR = SetupMaterialPBR(environment, (Color){ 255, 255, 255, 255 }, 255, 255);

                // Apply previously imported albedo texture to new PBR material
                if (textures[PBR_ALBEDO].id != 0) SetMaterialTexturePBR(&matPBR, PBR_ALBEDO, textures[PBR_ALBEDO]);
                if (textures[PBR_NORMALS].id != 0) SetMaterialTexturePBR(&matPBR, PBR_NORMALS, textures[PBR_NORMALS]);
                if (textures[PBR_METALNESS].id != 0) SetMaterialTexturePBR(&matPBR, PBR_METALNESS, textures[PBR_METALNESS]);
                if (textures[PBR_ROUGHNESS].id != 0) SetMaterialTexturePBR(&matPBR, PBR_ROUGHNESS, textures[PBR_ROUGHNESS]);
                if (textures[PBR_AO].id != 0) SetMaterialTexturePBR(&matPBR, PBR_AO, textures[PBR_AO]);
                if (textures[PBR_EMISSION].id != 0) SetMaterialTexturePBR(&matPBR, PBR_EMISSION, textures[PBR_EMISSION]);
                if (textures[PBR_HEIGHT].id != 0) SetMaterialTexturePBR(&matPBR, PBR_HEIGHT, textures[PBR_HEIGHT]);

                // Set up materials and lighting
                material = (Material){ 0 };
                material.shader = matPBR.env.pbrShader;
                model.material = material;
            }
            else if (IsFileExtension(droppedFiles[0], ".obj"))
            {
                UnloadModel(model);
                model = LoadModel(droppedFiles[0]);
                model.material = material;
            }
            else
            {
                // Check for supported image file extensions
                bool supportedImage = false;
                for (int i = 0; i < MAX_SUPPORTED_EXTENSIONS; i++)
                {
                    if (IsFileExtension(droppedFiles[0], imageExtensions[i]))
                    {
                        supportedImage = true;
                        break;
                    }
                }

                // Check for texture rectangle drop for texture updating
                if (supportedImage)
                {
                    for (int i = 0; i < MAX_TEXTURES; i++)
                    {
                        // Calculate texture rectangle based on current scrolling and other values
                        Vector2 pos = { GetScreenWidth() - UI_MENU_WIDTH + UI_MENU_WIDTH/2, scrolling + UI_MENU_PADDING*2 + UI_MENU_PADDING*2.5f + 
                                        UI_MENU_PADDING*1.25f + UI_MENU_WIDTH*0.375f - UI_TEXT_SIZE_H3/2 + i*UI_TEXTURES_PADDING };
                        Rectangle rect = { pos.x - UI_TEXTURES_SIZE/2, pos.y - UI_TEXTURES_SIZE/2, UI_TEXTURES_SIZE, UI_TEXTURES_SIZE };

                        // Check if file is droppen in texture rectangle
                        if (CheckCollisionPointRec(GetMousePosition(), rect))
                        {
                            Texture2D newTex = LoadTexture(droppedFiles[0]);
                            if (textures[i].id != 0) UnsetMaterialTexturePBR(&matPBR, i);
                            SetMaterialTexturePBR(&matPBR, i, newTex);
                            textures[i] = newTex;
                            break;
                        }
                    }
                }
            }

            ClearDroppedFiles();
        }

        // Check for display UI switch states
        if (IsKeyPressed(KEY_SPACE))
        {
            // Update current draw interface state
            drawUI = !drawUI;

            // Reset current selected light id
            if (selectedLight != -1) selectedLight = -1;
        }

        // Check for display help UI shortcut input
        if (IsKeyPressed(KEY_H))
        {
            // Update current draw help interface state
            drawHelp = true;

            // Reset current selected light
            if (selectedLight != -1) selectedLight = -1;
        }

        // Check for render mode shortcut inputs
        if (IsKeyPressed(KEY_F1)) renderMode = DEFAULT;
        else if (IsKeyPressed(KEY_F2)) renderMode = ALBEDO;
        else if (IsKeyPressed(KEY_F3)) renderMode = NORMALS;
        else if (IsKeyPressed(KEY_F4)) renderMode = METALNESS;
        else if (IsKeyPressed(KEY_F5)) renderMode = ROUGHNESS;
        else if (IsKeyPressed(KEY_F6)) renderMode = AMBIENT_OCCLUSION;
        else if (IsKeyPressed(KEY_F7)) renderMode = EMISSION;
        else if (IsKeyPressed(KEY_F8)) renderMode = LIGHTING;
        else if (IsKeyPressed(KEY_F9)) renderMode = FRESNEL;
        else if (IsKeyPressed(KEY_F10)) renderMode = IRRADIANCE;
        else if (IsKeyPressed(KEY_F11)) renderMode = REFLECTIVITY;

        // Check for render scale shortcut inputs
        if ((GetKeyPressed() == KEY_NUMPAD_SUM) && (renderScale < (MAX_RENDER_SCALES - 1))) renderScale++;
        else if ((GetKeyPressed() == KEY_NUMPAD_SUBTRACT) && (renderScale > 0)) renderScale--;

        // Check for lights movement input
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
        {
            // Update mouse delta position
            lastMousePosX = mousePosX;
            mousePosX = GetMouseX();

            // Update lights positions based on delta position with an orbital movement
            lightAngle += (mousePosX - lastMousePosX)*LIGHT_SPEED;
            for (int i = 0; i < totalLights; i++)
            {
                float angle = lightAngle + 90*i;
                lights[i].position.x = LIGHT_DISTANCE*cosf(angle*DEG2RAD);
                lights[i].position.z = LIGHT_DISTANCE*sinf(angle*DEG2RAD);

                // Send lights values to environment PBR shader
                UpdateLightValues(environment, lights[i]);
            }
        }
        else mousePosX = GetMouseX();

        // Check for interface scrolling
        if (GetMouseWheelMove() != 0 && overUI)
        {
            scrolling += GetMouseWheelMove()*SCROLL_SPEED;
            if (scrolling < -MAX_SCROLL) scrolling = -MAX_SCROLL;
            else if (scrolling > 0) scrolling = 0;
        }

        // Apply camera movement just if movement start is in viewport range
        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON) && overUI) canMoveCamera = false;
        else if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON)) canMoveCamera = true;

        // Check for texture map deletion input
        for (int i = 0; i < MAX_TEXTURES; i++)
        {
            // Calculate texture rectangle based on current scrolling and other values
            Vector2 pos = { GetScreenWidth() - UI_MENU_WIDTH + UI_MENU_WIDTH/2, scrolling + UI_MENU_PADDING*2 + UI_MENU_PADDING*2.5f + 
                            UI_MENU_PADDING*1.25f + UI_MENU_WIDTH*0.375f - UI_TEXT_SIZE_H3/2 + i*UI_TEXTURES_PADDING };
            Rectangle rect = { pos.x - UI_TEXTURES_SIZE/2, pos.y - UI_TEXTURES_SIZE/2, UI_TEXTURES_SIZE, UI_TEXTURES_SIZE };

            // Check if texture map rectangle is pressed
            if (CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                UnsetMaterialTexturePBR(&matPBR, i);
                textures[i] = (Texture2D){ 0 };
                break;
            }
        }

        // Avoid conflict between camera zoom and interface scroll
        if (GetMouseWheelMove() != 0) canMoveCamera = !overUI;

        // Check for camera movement inputs
        if (canMoveCamera) UpdateCamera(&camera);

        // Fix camera move state if camera mode is orbital and mouse middle button is not down
        if (!canMoveCamera && ((!IsMouseButtonDown(MOUSE_MIDDLE_BUTTON) && (cameraType == CAMERA_ORBITAL)))) canMoveCamera = true;

        // Check for light select input
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            // Store last selected light value
            int lastSelected = selectedLight;

            // Check mouse-light collision for all lights
            for (int i = 0; i < totalLights; i++)
            {
                Ray ray = GetMouseRay(GetMousePosition(), camera);
                if (CheckCollisionRaySphere(ray, lights[i].position, LIGHT_RADIUS)) selectedLight = i;
            }

            bool currentWindow = false;
            if (selectedLight != -1)
            {
                Vector2 screenPos = GetWorldToScreen(lights[selectedLight].position, camera);
                if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){ screenPos.x + UI_MENU_PADDING/2, screenPos.y + 
                                           UI_MENU_PADDING/2, UI_LIGHT_WIDTH, UI_LIGHT_HEIGHT })) currentWindow = true;
            }

            // Deselect current light if mouse position is not over any light
            if ((selectedLight == lastSelected) && !currentWindow) selectedLight = -1;
        }

        // Update camera values and send them to all required shaders
        Vector2 screenRes = { (float)GetScreenWidth()*renderScales[renderScale], (float)GetScreenHeight()*renderScales[renderScale] };
        UpdateEnvironmentValues(environment, camera, screenRes);

        // Send resolution values to post-processing shader
        resolution[0] = screenRes.x;
        resolution[1] = screenRes.y;
        SetShaderValue(fxShader, fxResolutionLoc, resolution, 2);

        // Send current mode to PBR shader and enabled screen effects states to post-processing shader
        int shaderMode[1] = { renderMode };
        SetShaderValuei(environment.pbrShader, shaderModeLoc, shaderMode, 1);
        shaderMode[0] = enabledFxaa;
        SetShaderValuei(fxShader, enabledFxaaLoc, shaderMode, 1);
        shaderMode[0] = enabledBloom;
        SetShaderValuei(fxShader, enabledBloomLoc, shaderMode, 1);
        shaderMode[0] = enabledVignette;
        SetShaderValuei(fxShader, enabledVignetteLoc, shaderMode, 1);
        //--------------------------------------------------------------------------

        // Draw
        //--------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(DARKGRAY);

            // Render to texture for antialiasing post-processing
            BeginTextureMode(fxTarget);

                Begin3dMode(camera);

                    // Draw ground grid
                    if (drawGrid) DrawGrid(10, 1.0f);

                    // Draw loaded model using physically based rendering
                    DrawModelPBR(model, matPBR, (Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f }, 0.0f, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE });
                    if (drawWire) DrawModelWires(model, (Vector3){ 0.0f, 0.0f, 0.0f }, MODEL_SCALE, DARKGRAY);

                    // Draw light gizmos
                    if (drawLights) for (unsigned int i = 0; (i < totalLights); i++)
                    {
                        Ray ray = GetMouseRay(GetMousePosition(), camera);
                        DrawLight(lights[i], CheckCollisionRaySphere(ray, lights[i].position, LIGHT_RADIUS));
                    }

                    // Render skybox (render as last to prevent overdraw)
                    if (drawSkybox) DrawSkybox(environment, camera);

                End3dMode();

            EndTextureMode();

            BeginShaderMode(fxShader);

                DrawTexturePro(fxTarget.texture, (Rectangle){ 0, 0, fxTarget.texture.width, -fxTarget.texture.height }, 
                               (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() }, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            EndShaderMode();

            // Draw logo if enabled based on interface menu padding
            if (!drawHelp && drawLogo)
            {
                int padding = GetScreenWidth() - UI_MENU_PADDING*1.25f - iconTex.width;
                if (drawUI) padding -= UI_MENU_WIDTH;
                DrawTexture(iconTex, padding, GetScreenHeight() - UI_MENU_PADDING*1.25f - iconTex.height, WHITE);
            }

            // Draw help window if help menu is enabled
            if (drawHelp)
            {
                // Draw help background
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(UI_COLOR_BACKGROUND, 0.8f));

                // Draw rPBR logo and title
                int padding = UI_MENU_PADDING*3 + iconTex.height + UI_MENU_PADDING;
                DrawTexture(iconTex, GetScreenWidth()/2 - iconTex.width/2, UI_MENU_PADDING*3, WHITE);
                DrawText(UI_TEXT_TITLE, GetScreenWidth()/2 - titleLength/2, padding, UI_TEXT_SIZE_H3, WHITE);

                // Draw controls title
                padding += UI_MENU_PADDING*3.5f;
                DrawText(UI_TEXT_CONTROLS, GetScreenWidth()/2 - controlsLength/2, padding, UI_TEXT_SIZE_H1, UI_COLOR_PRIMARY);
                DrawRectangle(GetScreenWidth()/2 - controlsLength, padding + UI_TEXT_SIZE_H1 + UI_MENU_PADDING/2, controlsLength*2, 2, UI_COLOR_PRIMARY);

                // Draw camera controls labels
                padding += UI_TEXT_SIZE_H1 + UI_MENU_PADDING*2.5f;
                DrawText(UI_TEXT_CONTROLS_01, GetScreenWidth()*0.35f, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING;
                DrawText(UI_TEXT_CONTROLS_02, GetScreenWidth()*0.35f, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING;
                DrawText(UI_TEXT_CONTROLS_03, GetScreenWidth()*0.35f, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING;
                DrawText(UI_TEXT_CONTROLS_04, GetScreenWidth()*0.35f, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);

                // Draw credits title
                padding += UI_MENU_PADDING*4;
                DrawText(UI_TEXT_CREDITS, GetScreenWidth()/2 - creditsLength/2, padding, UI_TEXT_SIZE_H1, UI_COLOR_PRIMARY);
                DrawRectangle(GetScreenWidth()/2 - creditsLength, padding + UI_TEXT_SIZE_H1 + UI_MENU_PADDING/2, creditsLength*2, 2, UI_COLOR_PRIMARY);

                // Draw credits labels
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING*2.5f;
                DrawText(UI_TEXT_CREDITS_VICTOR, GetScreenWidth()/2 - creditsVictorLength/2, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING;
                DrawText(UI_TEXT_CREDITS_RAMON, GetScreenWidth()/2 - creditsRamonLength/2, padding, UI_TEXT_SIZE_H2, UI_COLOR_SECONDARY);
                padding += UI_TEXT_SIZE_H2 + UI_MENU_PADDING*3;
                DrawText(UI_TEXT_CREDITS_WEB, GetScreenWidth()/2 - creditsWebLength/2, padding, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);

                // Draw close help menu button and check input
                if (GuiButton((Rectangle){ GetScreenWidth()/2 - UI_BUTTON_WIDTH/2, GetScreenHeight() - UI_BUTTON_HEIGHT - UI_MENU_PADDING*5, 
                    UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT }, UI_TEXT_BUTTON_CLOSE_HELP)) drawHelp = false;
            }
            else if (drawUI)
            {
                // Draw light settings interface if any light is selected
                if (selectedLight != -1) DrawLightInterface(&lights[selectedLight]);

                // Draw global interface to manage textures, material properties and render settings
                DrawInterface((Vector2){ GetScreenWidth(), GetScreenHeight() }, scrolling);
            }

        EndDrawing();
        //--------------------------------------------------------------------------
    }

    // De-Initialization
    //------------------------------------------------------------------------------
    // Clear internal buffers
    ClearDroppedFiles();

    // Unload loaded model mesh and binded textures
    UnloadModel(model);

    // Unload materialPBR assigned textures
    UnloadMaterialPBR(matPBR);

    // Unload environment loaded shaders and dynamic textures
    UnloadEnvironment(environment);

    // Unload other resources
    UnloadImage(icon);
    UnloadTexture(iconTex);
    UnloadRenderTexture(fxTarget);
    UnloadShader(fxShader);

    // Close window and OpenGL context
    CloseWindow();
    //------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
// Initialize interface texts lengths
void InitInterface(void)
{
    // Load interface style and colors from file
    LoadGuiStyle(PATH_GUI_STYLE);

    // Calculate interface right menu titles lengths
    texTitleLength = MeasureText(UI_TEXT_TEXTURES_TITLE, UI_TEXT_SIZE_H2);
    titlesLength[0] = MeasureText(textureTitles[0], UI_TEXT_SIZE_H3);
    titlesLength[1] = MeasureText(textureTitles[1], UI_TEXT_SIZE_H3);
    titlesLength[2] = MeasureText(textureTitles[2], UI_TEXT_SIZE_H3);
    titlesLength[3] = MeasureText(textureTitles[3], UI_TEXT_SIZE_H3);
    titlesLength[4] = MeasureText(textureTitles[4], UI_TEXT_SIZE_H3);
    titlesLength[5] = MeasureText(textureTitles[5], UI_TEXT_SIZE_H3);
    titlesLength[6] = MeasureText(textureTitles[6], UI_TEXT_SIZE_H3);

    // Calculate interface left menu titles lengths
    matTitleLength = MeasureText(UI_TEXT_MATERIAL_TITLE, UI_TEXT_SIZE_H2);
    renderTitleLength = MeasureText(UI_TEXT_RENDER_TITLE, UI_TEXT_SIZE_H2);
    renderScaleLength = MeasureText(UI_TEXT_RENDER_SCALE, UI_TEXT_SIZE_H3);
    renderModeLength = MeasureText(UI_TEXT_RENDER_MODE, UI_TEXT_SIZE_H3);
    renderEffectsLength = MeasureText(UI_TEXT_RENDER_EFFECTS, UI_TEXT_SIZE_H3);
    cameraModeLength = MeasureText(UI_TEXT_CAMERA_MODE, UI_TEXT_SIZE_H3);
    effectsTitleLength = MeasureText(UI_TEXT_EFFECTS_TITLE, UI_TEXT_SIZE_H2);
    fxaaLength = MeasureText(UI_TEXT_EFFECTS_FXAA, UI_TEXT_SIZE_H3);
    bloomLength = MeasureText(UI_TEXT_EFFECTS_BLOOM, UI_TEXT_SIZE_H3);
    vignetteLength = MeasureText(UI_TEXT_EFFECTS_VIGNETTE, UI_TEXT_SIZE_H3);
    wireLength = MeasureText(UI_TEXT_EFFECTS_WIRE, UI_TEXT_SIZE_H3);
    logoLength = MeasureText(UI_TEXT_DRAW_LOGO, UI_TEXT_SIZE_H3);
    creditsLength = MeasureText(UI_TEXT_CREDITS, UI_TEXT_SIZE_H1);
    controlsLength = MeasureText(UI_TEXT_CONTROLS, UI_TEXT_SIZE_H1);
    titleLength = MeasureText(UI_TEXT_TITLE, UI_TEXT_SIZE_H3);
    creditsVictorLength = MeasureText(UI_TEXT_CREDITS_VICTOR, UI_TEXT_SIZE_H2);
    creditsRamonLength = MeasureText(UI_TEXT_CREDITS_RAMON, UI_TEXT_SIZE_H2);
    dragLength = MeasureText(UI_TEXT_DRAG_HERE, UI_TEXT_SIZE_H3);
    creditsWebLength = MeasureText(UI_TEXT_CREDITS_WEB, UI_TEXT_SIZE_H2);
    deleteLength = MeasureText(UI_TEXT_DELETE, UI_TEXT_SIZE_H3);
    displayLength = MeasureText(UI_TEXT_DISPLAY, UI_TEXT_SIZE_H3);
}

// Draw a light gizmo based on light attributes
void DrawLight(Light light, bool over)
{
    switch (light.type)
    {
        case LIGHT_DIRECTIONAL:
        {
            DrawSphere(light.position, (over ? (LIGHT_RADIUS + LIGHT_OFFSET) : LIGHT_RADIUS), (light.enabled ? light.color : GRAY));
            DrawLine3D(light.position, light.target, (light.enabled ? light.color : DARKGRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 1.0f, 0.0f, 0.0f }, 90.0f, (light.enabled ? light.color : GRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 0.0f, 1.0f, 0.0f }, 90.0f, (light.enabled ? light.color : GRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 0.0f, 0.0f, 1.0f }, 90.0f, (light.enabled ? light.color : GRAY));
        } break;
        case LIGHT_POINT: DrawSphere(light.position, (over ? (LIGHT_RADIUS + LIGHT_OFFSET) : LIGHT_RADIUS), (light.enabled ? light.color : GRAY));
        default: break;
    }
}

// Draw interface based on current window dimensions
void DrawInterface(Vector2 size, int scrolling)
{
    int padding = scrolling;

    // Draw interface right menu background
    DrawRectangle(size.x - UI_MENU_WIDTH, 0, UI_MENU_WIDTH, size.y, UI_COLOR_BACKGROUND);
    DrawRectangle(size.x - UI_MENU_WIDTH - UI_MENU_BORDER, 0, UI_MENU_BORDER, size.y, UI_COLOR_PRIMARY);

    // Draw textures title
    DrawText(UI_TEXT_TEXTURES_TITLE, size.x - UI_MENU_WIDTH + UI_MENU_WIDTH/2 - texTitleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);
    DrawRectangle(size.x - UI_MENU_WIDTH + UI_MENU_WIDTH/2 - texTitleLength/2, padding + UI_MENU_PADDING*2.4f, texTitleLength, 2, UI_COLOR_PRIMARY);

    // Draw textures
    padding = scrolling + UI_MENU_PADDING*2 + UI_MENU_PADDING*2.5f + UI_MENU_PADDING*1.25f;
    for (int i = 0; i < MAX_TEXTURES; i++)
    {
        Vector2 pos = { size.x - UI_MENU_WIDTH + UI_MENU_WIDTH/2, padding + UI_MENU_WIDTH*0.375f - UI_TEXT_SIZE_H3/2 + i*UI_TEXTURES_PADDING };
        DrawTextureMap(i, textures[i], pos);
    }

    // Reset padding to start with left menu drawing
    padding = 0;

    // Draw interface left menu background
    DrawRectangle(0, 0, UI_MENU_WIDTH, size.y, UI_COLOR_BACKGROUND);
    DrawRectangle(UI_MENU_WIDTH - UI_MENU_BORDER, 0, UI_MENU_BORDER, size.y, UI_COLOR_PRIMARY);

    // Draw material title
    DrawText(UI_TEXT_MATERIAL_TITLE, UI_MENU_WIDTH/2 - matTitleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);
    DrawRectangle(UI_MENU_WIDTH/2 - matTitleLength/2, padding + UI_MENU_PADDING*2.4f, matTitleLength, 2, UI_COLOR_PRIMARY);

    // Draw albedo RGB sliders
    padding += UI_MENU_PADDING*2.5f;
    DrawText(textureTitles[0], UI_MENU_WIDTH/2 - titlesLength[0]/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    DrawText("R", UI_MENU_WIDTH/10 - UI_TEXT_SIZE_H3/2, padding + UI_MENU_BORDER, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    matPBR.albedo.color.r = (int)GuiSlider((Rectangle){ UI_MENU_BORDER*2 + UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, 
                                         padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.albedo.color.r, 0, 255);
    padding += UI_MENU_PADDING*2;
    DrawText("G", UI_MENU_WIDTH/10 - UI_TEXT_SIZE_H3/2, padding + UI_MENU_BORDER, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    matPBR.albedo.color.g = (int)GuiSlider((Rectangle){ UI_MENU_BORDER*2 + UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, 
                                         padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.albedo.color.g, 0, 255);
    padding += UI_MENU_PADDING*2;
    DrawText("B", UI_MENU_WIDTH/10 - UI_TEXT_SIZE_H3/2, padding + UI_MENU_BORDER, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    matPBR.albedo.color.b = (int)GuiSlider((Rectangle){ UI_MENU_BORDER*2 + UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, 
                                         padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.albedo.color.b, 0, 255);

    // Draw metalness slider
    padding += UI_MENU_PADDING*2;
    DrawText(textureTitles[2], UI_MENU_WIDTH/2 - titlesLength[2]/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    matPBR.metalness.color.r = (int)GuiSlider((Rectangle){ UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.metalness.color.r, 0, 255);

    // Draw roughness slider
    padding += UI_MENU_PADDING*2;
    DrawText(textureTitles[3], UI_MENU_WIDTH/2 - titlesLength[3]/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    matPBR.roughness.color.r = (int)GuiSlider((Rectangle){ UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.roughness.color.r, 0, 255);

    // Draw height parallax slider
    padding += UI_MENU_PADDING*2;
    DrawText(textureTitles[6], UI_MENU_WIDTH/2 - titlesLength[6]/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    matPBR.height.color.r = (int)GuiSlider((Rectangle){ UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.75f/2, padding, UI_MENU_WIDTH*0.75f, UI_SLIDER_HEIGHT }, matPBR.height.color.r, 0, 255);

    // Draw render settings title 
    padding += UI_MENU_PADDING*2.5f;
    DrawText(UI_TEXT_RENDER_TITLE, UI_MENU_WIDTH/2 - renderTitleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);
    DrawRectangle(UI_MENU_WIDTH/2 - renderTitleLength/2, padding + UI_MENU_PADDING*2.4f, renderTitleLength, 2, UI_COLOR_PRIMARY);

    // Draw render scale combo box
    padding += UI_MENU_PADDING*2.5f;
    DrawText(UI_TEXT_RENDER_SCALE, UI_MENU_WIDTH/2 - renderScaleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    renderScale = GuiComboBox((Rectangle){ UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.3f - UI_MENU_WIDTH*0.6f/8, padding, UI_MENU_WIDTH*0.6f, UI_SLIDER_HEIGHT*1.5f }, MAX_RENDER_SCALES, (char **)renderScalesTitles, renderScale);

    // Draw render mode combo box
    padding += UI_MENU_PADDING*2.0f;
    DrawText(UI_TEXT_RENDER_MODE, UI_MENU_WIDTH/2 - renderModeLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    padding += UI_MENU_PADDING*2.25f;
    renderMode = GuiComboBox((Rectangle){ UI_MENU_WIDTH/2 - UI_MENU_WIDTH*0.3f - UI_MENU_WIDTH*0.6f/8, padding, UI_MENU_WIDTH*0.6f, UI_SLIDER_HEIGHT*1.5f }, MAX_RENDER_MODES, (char **)renderModesTitles, renderMode);

    // Draw post-processing effects title 
    padding += UI_MENU_PADDING*3;
    DrawText(UI_TEXT_EFFECTS_TITLE, UI_MENU_WIDTH/2 - effectsTitleLength/2, padding + UI_MENU_PADDING, UI_TEXT_SIZE_H2, UI_COLOR_PRIMARY);
    DrawRectangle(UI_MENU_WIDTH/2 - renderTitleLength/2, padding + UI_MENU_PADDING*2.4f, renderTitleLength, 2, UI_COLOR_PRIMARY);

    // Draw FXAA effect enabled state checkbox
    padding += UI_MENU_PADDING*3.75f;
    enabledFxaa = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_EFFECTS_FXAA, enabledFxaa);

    // Draw bloom effect enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    enabledBloom = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_EFFECTS_BLOOM, enabledBloom);

    // Draw vignette effect enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    enabledVignette = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_EFFECTS_VIGNETTE, enabledVignette);

    // Draw wireframe effect enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    drawWire = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_EFFECTS_WIRE, drawWire);

    // Draw draw logo enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    drawLogo = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_DRAW_LOGO, drawLogo);

    // Draw draw lights enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    drawLights = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_DRAW_LIGHTS, drawLights);

    // Draw draw grid enabled state checkbox
    padding += UI_MENU_PADDING*2.0f;
    drawGrid = GuiCheckBox((Rectangle){ UI_MENU_PADDING*1.85f, padding, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_DRAW_GRID, drawGrid);

    // Draw viewport interface help button
    if (GuiButton((Rectangle){ UI_MENU_WIDTH + UI_MENU_PADDING, GetScreenHeight() - UI_MENU_PADDING - UI_BUTTON_HEIGHT, 
                   UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT }, UI_TEXT_BUTTON_HELP))
    {
        drawHelp = true;
        if (selectedLight != -1) selectedLight = -1;
    }

    // Draw viewport interface screenshot button
    padding = UI_MENU_WIDTH + UI_MENU_PADDING + UI_BUTTON_WIDTH + UI_MENU_PADDING;
    if (GuiButton((Rectangle){ padding, GetScreenHeight() - UI_MENU_PADDING - UI_BUTTON_HEIGHT, 
                   UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT }, UI_TEXT_BUTTON_SS)) TakeScreenshot();

    // Draw viewport interface camera type combo box
    padding += UI_BUTTON_WIDTH + UI_MENU_PADDING;
    cameraType = GuiComboBox((Rectangle){ padding, GetScreenHeight() - UI_MENU_PADDING - UI_BUTTON_HEIGHT,
                             UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT }, MAX_CAMERA_TYPES, (char **)cameraTypesTitles, cameraType);

    // Draw viewport interface reset scene button
    padding += UI_MENU_PADDING*2 + UI_BUTTON_WIDTH + UI_MENU_PADDING;
    if (GuiButton((Rectangle){ padding, GetScreenHeight() - UI_MENU_PADDING - UI_BUTTON_HEIGHT, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT }, UI_TEXT_BUTTON_RESET)) resetScene = true;

    // Draw viewport interface display/hide help message
    DrawText(UI_TEXT_DISPLAY, GetScreenWidth() - UI_MENU_WIDTH - displayLength - 10, GetScreenHeight() - UI_TEXT_SIZE_H3 - 5, UI_TEXT_SIZE_H3, UI_COLOR_BACKGROUND);
}

// Draw specific light settings interface
void DrawLightInterface(Light *light)
{
    Vector2 screenPos = GetWorldToScreen(light->position, camera);
    Vector2 padding = { screenPos.x + UI_MENU_PADDING/2, screenPos.y + UI_MENU_PADDING/2 };

    // Draw interface background
    DrawRectangle(padding.x, padding.y, UI_LIGHT_WIDTH, UI_LIGHT_HEIGHT, UI_COLOR_PRIMARY);
    DrawRectangle(padding.x + 3, padding.y + 3, UI_LIGHT_WIDTH - 6, UI_LIGHT_HEIGHT - 6, UI_COLOR_BACKGROUND);
    padding.x += UI_MENU_PADDING;
    padding.y += UI_MENU_PADDING;
    
    // Draw light enabled state checkbox
    light->enabled = GuiCheckBox((Rectangle){ padding.x, padding.y, UI_CHECKBOX_SIZE, UI_CHECKBOX_SIZE }, UI_TEXT_LIGHT_ENABLED, light->enabled);
    padding.y += UI_MENU_PADDING*2;

    // Draw light color R channel slider
    light->color.r = (int)GuiSlider((Rectangle){ padding.x + UI_MENU_PADDING*1.5f, padding.y, UI_LIGHT_WIDTH*0.75f, UI_SLIDER_HEIGHT }, light->color.r, 0, 255);
    DrawText(UI_TEXT_LIGHT_R, padding.x, padding.y + UI_TEXT_SIZE_H3/2, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    padding.y += UI_MENU_PADDING*2;

    // Draw light color G channel slider
    light->color.g = (int)GuiSlider((Rectangle){ padding.x + UI_MENU_PADDING*1.5f, padding.y, UI_LIGHT_WIDTH*0.75f, UI_SLIDER_HEIGHT }, light->color.g, 0, 255);
    DrawText(UI_TEXT_LIGHT_G, padding.x, padding.y + UI_TEXT_SIZE_H3/2, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    padding.y += UI_MENU_PADDING*2;

    // Draw light color B channel slider
    light->color.b = (int)GuiSlider((Rectangle){ padding.x + UI_MENU_PADDING*1.5f, padding.y, UI_LIGHT_WIDTH*0.75f, UI_SLIDER_HEIGHT }, light->color.b, 0, 255);
    DrawText(UI_TEXT_LIGHT_B, padding.x, padding.y + UI_TEXT_SIZE_H3/2, UI_TEXT_SIZE_H3, UI_COLOR_SECONDARY);
    padding.y += UI_MENU_PADDING*2;

    // Send lights values to environment PBR shader
    UpdateLightValues(environment, *light);
}

// Draw interface PBR texture or alternative text
void DrawTextureMap(int id, Texture2D texture, Vector2 position)
{
    Rectangle rect = { position.x - UI_TEXTURES_SIZE/2, position.y - UI_TEXTURES_SIZE/2, UI_TEXTURES_SIZE, UI_TEXTURES_SIZE };
    DrawRectangle(rect.x - UI_MENU_BORDER, rect.y - UI_MENU_BORDER, rect.width + UI_MENU_BORDER*2, rect.height + UI_MENU_BORDER*2, UI_COLOR_PRIMARY);
    DrawText(textureTitles[id], position.x - titlesLength[id]/2, position.y - UI_TEXT_SIZE_H3/2 - rect.height*0.6f, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);

    // Draw PBR texture or display help message
    if (texture.id != 0)
    {
        DrawTexturePro(texture, (Rectangle){ 0, 0, texture.width, texture.height }, rect, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        // Draw semi-transparent rectangle if mouse is over the texture rectangle
        if (CheckCollisionPointRec(GetMousePosition(), rect))
        {
            DrawRectangleRec(rect, Fade(UI_COLOR_SECONDARY, 0.5f));
            DrawText(UI_TEXT_DELETE, rect.x + rect.width/2 - deleteLength/2, rect.y + rect.height/2 - UI_TEXT_SIZE_H3/2, UI_TEXT_SIZE_H3, UI_COLOR_BACKGROUND);
        }
    }
    else
    {
        DrawRectangleRec(rect, UI_COLOR_SECONDARY);
        DrawText(UI_TEXT_DRAG_HERE, position.x - dragLength/2, position.y, UI_TEXT_SIZE_H3, UI_COLOR_PRIMARY);
    }
}
