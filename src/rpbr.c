/***********************************************************************************
*
*   rPBR - Physically based rendering viewer for raylib
*
*   FEATURES:
*       - Load OBJ models and texture images in real-time by drag and drop.
*       - Use right mouse button to rotate lighting.
*       - Use middle mouse button to rotate and pan camera.
*       - Use interface to adjust lighting, material and screen parameters (space - display/hide interface).
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
#include "pbrui.h"                          // Required for interface drawing

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         WINDOW_TITLE                "rPBR - Physically based rendering 3D model viewer"
#define         WINDOW_WIDTH                1440
#define         WINDOW_HEIGHT               810

#define         PATH_ICON                   "resources/textures/rpbr_icon.png"
#define         PATH_TEXTURES_HDR           "resources/textures/hdr/pinetree.hdr"
#define         PATH_MODEL                  "resources/models/dwarf.obj"
#define         PATH_TEXTURES_ALBEDO        "resources/textures/dwarf/dwarf_albedo.png"
#define         PATH_TEXTURES_NORMALS       "resources/textures/dwarf/dwarf_normals.png"
#define         PATH_TEXTURES_METALNESS     "resources/textures/dwarf/dwarf_metalness.png"
#define         PATH_TEXTURES_ROUGHNESS     "resources/textures/dwarf/dwarf_roughness.png"
// #define      PATH_TEXTURES_AO            "resources/textures/dwarf/dwarf_ao.png"
// #define      PATH_TEXTURES_EMISSION      "resources/textures/dwarf/dwarf_emission.png"
// #define      PATH_TEXTURES_HEIGHT        "resources/textures/dwarf/dwarf_height.png"
#define         PATH_SHADERS_POSTFX_VS      "resources/shaders/postfx.vs"
#define         PATH_SHADERS_POSTFX_FS      "resources/shaders/postfx.fs"

#define         MAX_RENDER_SCALES           3                   // Max number of available render scales (0.5X, 1X, 2X)
#define         MAX_LIGHTS                  4                   // Max lights supported by shader
#define         MAX_SCROLL                  850                 // Max mouse wheel for interface scrolling
#define         SCROLL_SPEED                50                  // Interface scrolling speed

#define         CAMERA_FOV                  60.0f               // Camera global field of view
#define         MODEL_SCALE                 1.75f               // Model scale transformation for rendering
#define         MODEL_OFFSET                0.45f               // Distance between models for rendering
#define         ROTATION_SPEED              0.0f                // Models rotation speed

#define         LIGHT_SPEED                 0.1f                // Light rotation input speed
#define         LIGHT_DISTANCE              3.5f                // Light distance from center of world
#define         LIGHT_HEIGHT                1.0f                // Light height from center of world
#define         LIGHT_RADIUS                0.05f               // Light gizmo drawing radius
#define         LIGHT_OFFSET                0.03f               // Light gizmo drawing radius when mouse is over

#define         CUBEMAP_SIZE                1024                // Cubemap texture size
#define         IRRADIANCE_SIZE             32                  // Irradiance map from cubemap texture size
#define         PREFILTERED_SIZE            256                 // Prefiltered HDR environment map texture size
#define         BRDF_SIZE                   512                 // BRDF LUT texture map size

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { DEFAULT, ALBEDO, NORMALS, METALNESS, ROUGHNESS, AMBIENT_OCCLUSION, EMISSION, LIGHTING, FRESNEL, IRRADIANCE, REFLECTION } RenderMode;
typedef enum { RENDER_SCALE_0_5X, RENDER_SCALE_1X, RENDER_SCALE_2X } RenderScale;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
float renderScales[MAX_RENDER_SCALES] = { 0.5f, 1.0f, 2.0f };   // Availables render scales

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
void DrawLight(Light light, bool over);                         // Draw a light gizmo based on light attributes

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //------------------------------------------------------------------------------
    // Enable Multi Sampling Anti Aliasing 4x (if available)
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    InitInterface();

    // Change default window icon
    Image icon = LoadImage(PATH_ICON);
    SetWindowIcon(icon);
    SetWindowMinSize(960, 540);

    // Define render settings states
    RenderMode renderMode = DEFAULT;
    RenderScale renderScale = RENDER_SCALE_2X;
    CameraMode cameraMode = CAMERA_FREE;
    BackgroundMode backMode = BACKGROUND_SKY;
    bool drawGrid = false;
    bool drawWires = false;
    bool drawLights = true;
    bool drawSkybox = true;
    bool drawFPS = true;
    bool drawUI = true;
    bool canMoveCamera = true;
    bool overUI = false;
    int scrolling = 0;
    Texture2D textures[7] = { 0 };

    // Define post-processing effects enabled states
    bool enabledFxaa = true;
    bool enabledBloom = true;
    bool enabledVignette = true;

    // Initialize lighting rotation
    int mousePosX = 0;
    int lastMousePosX = 0;
    float lightAngle = 0.0f;

    // Define the camera to look into our 3d world, its mode and model drawing position
    float rotationAngle = 0.0f;
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    Camera camera = {{ 3.5f, 3.0f, 3.5f }, { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, CAMERA_FOV };
    SetCameraMode(camera, cameraMode);

    // Define environment attributes
    Environment environment = LoadEnvironment(PATH_TEXTURES_HDR, CUBEMAP_SIZE, IRRADIANCE_SIZE, PREFILTERED_SIZE, BRDF_SIZE);

    // Load external resources
    Model model = LoadModel(PATH_MODEL);
    MaterialPBR matPBR = SetupMaterialPBR(environment, (Color){ 255 }, 255, 255);
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
        CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 0, LIGHT_HEIGHT*2.0f, -LIGHT_DISTANCE }, (Vector3){ 0.0f, 0.0f, 0.0f }, (Color){ 255, 0, 255, 255 }, environment)
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
        // Update current rotation angle
        rotationAngle += ROTATION_SPEED;
        overUI = CheckCollisionPointRec(GetMousePosition(), (Rectangle){ GetScreenWidth() - UI_MENU_WIDTH, 0, UI_MENU_WIDTH, GetScreenHeight() });

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
                UnloadMaterialPBR(matPBR);
                matPBR = SetupMaterialPBR(environment, (Color){ 255 }, 255, 255);
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

            ClearDroppedFiles();
        }

        // Check for display UI switch states
        if (IsKeyPressed(KEY_SPACE)) drawUI = !drawUI;

        // Check for switch camera mode input
        if (IsKeyPressed(KEY_C))
        {
            rotationAngle = 0.0f;
            cameraMode = ((cameraMode == CAMERA_FREE) ? CAMERA_ORBITAL : CAMERA_FREE);
            camera.position = (Vector3){ 3.5f, ((cameraMode == CAMERA_FREE) ? 3.0f : 2.5f), 3.5f };
            camera.target = (Vector3){ 0.0f, ((cameraMode == CAMERA_FREE) ? 0.5f : 1.0f), 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.fovy = CAMERA_FOV;
            SetCameraMode(camera, cameraMode);
        }

        // Check for scene camera reset input
        if (IsKeyPressed(KEY_R))
        {
            // Reset rotation and camera values
            rotationAngle = 0.0f;
            cameraMode = CAMERA_FREE;
            camera.position = (Vector3){ 3.5f, 3.0f, 3.5f };
            camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.fovy = CAMERA_FOV;
            SetCameraMode(camera, cameraMode);
        }

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

        // Avoid conflict between camera zoom and interface scroll
        if (GetMouseWheelMove() != 0) canMoveCamera = !overUI;

        // Check for camera movement inputs
        if (canMoveCamera) UpdateCamera(&camera);

        // Fix camera move state if camera mode is orbital and mouse middle button is not down
        if (!canMoveCamera && ((!IsMouseButtonDown(MOUSE_MIDDLE_BUTTON) && (cameraMode == CAMERA_ORBITAL)))) canMoveCamera = true;

        /*// DEBUG INPUTS
        // Check for render mode inputs
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
        else if (IsKeyPressed(KEY_F11)) renderMode = REFLECTION;

        // Check for render scale inputs
        if (IsKeyPressed(KEY_Y) && (renderScale < RENDER_SCALE_2X))
        {
            renderScale++;
            UnloadRenderTexture(fxTarget);
            fxTarget = LoadRenderTexture(GetScreenWidth()*renderScales[renderScale], GetScreenHeight()*renderScales[renderScale]);
        }
        else if (IsKeyPressed(KEY_H) && (renderScale > RENDER_SCALE_0_5X))
        {
            renderScale--;
            UnloadRenderTexture(fxTarget);
            fxTarget = LoadRenderTexture(GetScreenWidth()*renderScales[renderScale], GetScreenHeight()*renderScales[renderScale]);
        }*/

        // Update lights values
        for (int i = 0; i < totalLights; i++)
        {
            // Check for light enabled input
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Ray ray = GetMouseRay(GetMousePosition(), camera);
                if (CheckCollisionRaySphere(ray, lights[i].position, LIGHT_RADIUS)) lights[i].enabled = !lights[i].enabled;
            }

            // Send lights values to environment PBR shader
            UpdateLightsValues(environment, lights[i]);
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
                    DrawModelPBR(model, matPBR, (Vector3){ 0, 0, 0 }, rotationAxis, rotationAngle, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE });
                    if (drawWires) DrawModelWires(model, (Vector3){ 0, 0, 0 }, MODEL_SCALE, DARKGRAY);

                    // Draw light gizmos
                    if (drawLights) for (unsigned int i = 0; (i < totalLights); i++)
                    {
                        Ray ray = GetMouseRay(GetMousePosition(), camera);
                        DrawLight(lights[i], CheckCollisionRaySphere(ray, lights[i].position, LIGHT_RADIUS));
                    }

                    // Render skybox (render as last to prevent overdraw)
                    if (drawSkybox) DrawSkybox(environment, backMode, camera);

                End3dMode();

            EndTextureMode();

            BeginShaderMode(fxShader);

                DrawTexturePro(fxTarget.texture, (Rectangle){ 0, 0, fxTarget.texture.width, -fxTarget.texture.height }, (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() }, (Vector2){ 0, 0 }, 0.0f, WHITE);

            EndShaderMode();

            if (drawUI) DrawInterface(GetScreenWidth(), GetScreenHeight(), scrolling, textures, 7);

            if (drawFPS) DrawFPS(10, 10);

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
// Draw a light gizmo based on light attributes
void DrawLight(Light light, bool over)
{
    switch (light.type)
    {
        case LIGHT_DIRECTIONAL:
        {
            DrawSphere(light.position, (over ? (LIGHT_RADIUS + LIGHT_OFFSET) : LIGHT_RADIUS), (light.enabled ? light.color : GRAY));
            DrawLine3D(light.position, light.target, (light.enabled ? light.color : DARKGRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 1, 0, 0 }, 90, (light.enabled ? light.color : GRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 0, 1, 0 }, 90, (light.enabled ? light.color : GRAY));
            DrawCircle3D(light.target, LIGHT_RADIUS, (Vector3){ 0, 0, 1 }, 90, (light.enabled ? light.color : GRAY));
        } break;
        case LIGHT_POINT: DrawSphere(light.position, (over ? (LIGHT_RADIUS + LIGHT_OFFSET) : LIGHT_RADIUS), (light.enabled ? light.color : GRAY));
        default: break;
    }
}