/***********************************************************************************
*
*   pbraylib - Physically Based Rendering for raylib
*
*   Copyright (c) 2017 Victor Fisac
*
***********************************************************************************/

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include <stdlib.h>                         // Required for: exit(), free()
#include <stdio.h>                          // Required for: printf()
#include <string.h>                         // Required for: strcpy()
#include <math.h>                           // Required for: pow()

#include "raylib.h"                         // Required for raylib framework
#include "pbrmath.h"                        // Required for matrix and vectors math
#include "pbrcore.h"                        // Required for lighting, environment and drawing functions
#include "pbrutils.h"                       // Required for utilities

#include "external/glad.h"                  // Required for OpenGL API

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         PATH_MODEL                  "resources/models/cerberus.obj"
#define         PATH_HDR                    "resources/textures/hdr/hdr_pinetree.hdr"
#define         PATH_TEXTURES_ALBEDO        "resources/textures/cerberus/cerberus_albedo.png"
#define         PATH_TEXTURES_NORMALS       "resources/textures/cerberus/cerberus_normals.png"
#define         PATH_TEXTURES_METALLIC      "resources/textures/cerberus/cerberus_metallic.png"
#define         PATH_TEXTURES_ROUGHNESS     "resources/textures/cerberus/cerberus_roughness.png"
#define         PATH_TEXTURES_AO            "resources/textures/cerberus/cerberus_ao.png"
#define         PATH_TEXTURES_HEIGHT        "resources/textures/cerberus/cerberus_height.png"

#define         MAX_LIGHTS                  4               // Max lights supported by shader
#define         MAX_ROWS                    1               // Rows to render models
#define         MAX_COLUMNS                 1               // Columns to render models
#define         MODEL_SCALE                 1.5f            // Model scale transformation for rendering
#define         MODEL_OFFSET                0.45f           // Distance between models for rendering
#define         ROTATION_SPEED              0.0f            // Models rotation speed

#define         CUBEMAP_SIZE                1024            // Cubemap texture size
#define         IRRADIANCE_SIZE             32              // Irradiance map from cubemap texture size
#define         PREFILTERED_SIZE            256             // Prefiltered HDR environment map texture size
#define         BRDF_SIZE                   512             // BRDF LUT texture map size

//----------------------------------------------------------------------------------
// Structs and enums
//----------------------------------------------------------------------------------
typedef enum { DEFAULT, ALBEDO, NORMALS, METALLIC, ROUGHNESS, AMBIENT_OCCLUSION, LIGHTING, FRESNEL, IRRADIANCE, REFLECTION } RenderMode;

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //------------------------------------------------------------------------------
    int screenWidth = 1280;
    int screenHeight = 720;
    int selectedLight = 0;
    RenderMode mode = DEFAULT;
    bool drawGrid = true;
    bool drawLights = true;
    bool drawSkybox = true;
    bool useAlbedoMap = true;
    bool useNormalMap = true;
    bool useMetallicMap = true;
    bool useRoughnessMap = true;
    bool useOcclusionMap = true;
    bool useParallaxMap = false;

    // Define the camera to look into our 3d world, its mode and model drawing position
    float rotationAngle = 0.0f;
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    Camera camera = {{ 2.75f, 2.55f, 2.75f }, { 1.0f, 1.05f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f };
    SetCameraMode(camera, CAMERA_FREE);

    // Enable Multi Sampling Anti Aliasing 4x (if available)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "rPBR - Physically Based Rendering");

    // Define environment attributes
    Environment environment = LoadEnvironment(PATH_HDR, CUBEMAP_SIZE, IRRADIANCE_SIZE, PREFILTERED_SIZE, BRDF_SIZE);

    // Load external resources
    Model model = LoadModel(PATH_MODEL);
    MaterialPBR matPBR = SetupMaterialPBR(environment, useAlbedoMap, useNormalMap, useMetallicMap, useRoughnessMap, useOcclusionMap, useParallaxMap);
    if (matPBR.useAlbedoMap) matPBR.albedoTex = LoadTexture(PATH_TEXTURES_ALBEDO);
    if (matPBR.useNormalMap) matPBR.normalsTex = LoadTexture(PATH_TEXTURES_NORMALS);
    if (matPBR.useMetallicMap) matPBR.metallicTex = LoadTexture(PATH_TEXTURES_METALLIC);
    if (matPBR.useRoughnessMap) matPBR.roughnessTex = LoadTexture(PATH_TEXTURES_ROUGHNESS);
    if (matPBR.useOcclusionMap) matPBR.aoTex = LoadTexture(PATH_TEXTURES_AO);
    if (matPBR.useParallaxMap) matPBR.heightTex = LoadTexture(PATH_TEXTURES_HEIGHT);

    // Set up materials and lighting
    Material material = { 0 };
    material.shader = matPBR.env.pbrShader;
    model.material = material;

    // Get PBR shader locations
    int shaderModeLoc = GetShaderLocation(model.material.shader, "renderMode");

    // Define lights attributes
    int lightsCount = 0;
    Light lights[MAX_LIGHTS] = { 0 };
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ -1.0f, 1.0f, -1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 255, 0, 0, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ 1.0f, 1.0f, -1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 0, 255, 0, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ -1.0f, 1.0f, 1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 0, 0, 255, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 3.0f, 2.0f, 3.0f }, (Vector3){ 0, 0, 0 }, (Color){ 255, 0, 255, 255 }, model.material.shader, &lightsCount);

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

        // Check for capture screenshot input
        if (IsKeyPressed(KEY_P)) CaptureScreenshot(screenWidth, screenHeight);

        // Check for scene reset input
        if (IsKeyPressed(KEY_R))
        {
            rotationAngle = 0.0f;
            camera.position = (Vector3){ 2.75f, 3.55f, 2.75f };
            camera.target = (Vector3){ 1.0f, 2.05f, 1.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.fovy = 45.0f;
            SetCameraMode(camera, CAMERA_FREE);
        }

        // Check selected light inputs
        if (IsKeyPressed(KEY_F1)) selectedLight = 0;
        else if (IsKeyPressed(KEY_F2)) selectedLight = 1;
        else if (IsKeyPressed(KEY_F3)) selectedLight = 2;
        else if (IsKeyPressed(KEY_F4)) selectedLight = 3;

        // Check for light position movement inputs
        if (IsKeyDown(KEY_UP)) lights[selectedLight].position.z += 0.1f;
        else if (IsKeyDown(KEY_DOWN)) lights[selectedLight].position.z -= 0.1f;
        if (IsKeyDown(KEY_RIGHT)) lights[selectedLight].position.x += 0.1f;
        else if (IsKeyDown(KEY_LEFT)) lights[selectedLight].position.x -= 0.1f;
        if (IsKeyDown(KEY_W)) lights[selectedLight].position.y += 0.1f;
        else if (IsKeyDown(KEY_S)) lights[selectedLight].position.y -= 0.1f;

        // Check for render mode inputs
        if (IsKeyPressed(KEY_ONE)) mode = DEFAULT;
        else if (IsKeyPressed(KEY_TWO)) mode = ALBEDO;
        else if (IsKeyPressed(KEY_THREE)) mode = NORMALS;
        else if (IsKeyPressed(KEY_FOUR)) mode = METALLIC;
        else if (IsKeyPressed(KEY_FIVE)) mode = ROUGHNESS;
        else if (IsKeyPressed(KEY_SIX)) mode = AMBIENT_OCCLUSION;
        else if (IsKeyPressed(KEY_SEVEN)) mode = LIGHTING;
        else if (IsKeyPressed(KEY_EIGHT)) mode = FRESNEL;
        else if (IsKeyPressed(KEY_NINE)) mode = IRRADIANCE;
        else if (IsKeyPressed(KEY_ZERO)) mode = REFLECTION;

        // Send current mode to shader
        int shaderMode[1] = { (int)mode };
        SetShaderValuei(model.material.shader, shaderModeLoc, shaderMode, 1);

        // Update current light position
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(environment.pbrShader, lights[i]);

        // Update camera values and send them to shader
        UpdateCamera(&camera);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(environment.pbrShader, environment.pbrViewLoc, cameraPos, 3);
        //--------------------------------------------------------------------------

        // Draw
        //--------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(DARKGRAY);

            Begin3dMode(camera);

                // Draw ground grid
                if (drawGrid) DrawGrid(10, 1.0f);

                // Draw loaded model using physically based rendering
                DrawModelPBR(model, matPBR, (Vector3){ 0, 0, 0 }, rotationAxis, rotationAngle, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE });

                // Draw light gizmos
                if (drawLights) for (unsigned int i = 0; (i < MAX_LIGHTS); i++) DrawLight(lights[i]);

                // Render skybox (render as last to prevent overdraw)
                if (drawSkybox) DrawSkybox(environment, camera);

            End3dMode();

            DrawFPS(10, 10);

        EndDrawing();
        //--------------------------------------------------------------------------
    }

    // De-Initialization
    //------------------------------------------------------------------------------
    // Unload loaded model mesh and binded textures
    UnloadModel(model);

    // Unload materialPBR assigned textures
    UnloadMaterialPBR(matPBR);

    // Unload environment loaded shaders and dynamic textures
    UnloadEnvironment(environment);

    // Close window and OpenGL context
    CloseWindow();
    //------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
// ...