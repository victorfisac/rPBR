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
#include "raylib.h"
#include "pbrmath.h"        // Required for matrix and vectors math
#include <string.h>         // Required for: strcpy()

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         MAX_LIGHTS          4               // Max lights supported by shader
#define         MAX_ROWS            7               // Rows to render models
#define         MAX_COLUMNS         7               // Columns to render models
#define         MODEL_SCALE         0.35f           // Model scale transformation for rendering
#define         MODEL_OFFSET        0.35f           // Distance between models for rendering

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //------------------------------------------------------------------------------
    int screenWidth = 1366;
    int screenHeight = 768;

    // Enable Multi Sampling Anti Aliasing 4x (if available)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "pbraylib - Physically Based Rendering");

    // Define the camera to look into our 3d world, its mode and model drawing position
    float rotationAngle = 0.0f;
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    Vector3 lightPosition[MAX_LIGHTS] = { (Vector3){ 1.125f, 1.0f, 1.125f }, (Vector3){ 2.125f, 1.0f, 1.125f }, (Vector3){ 1.125f, 1.0f, 2.125f }, (Vector3){ 2.125f, 1.0f, 2.125f } };
    Camera camera = {{ 3.75f, 2.25f, 3.75f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f };
    SetCameraMode(camera, CAMERA_FREE);
    int selectedLight = 0;

    // Load external resources
    Model dwarf = LoadModel("resources/models/dwarf.obj");
    Shader pbrShader = LoadShader("resources/shaders/pbr.vs", "resources/shaders/pbr.fs");

    // Set up materials and lighting
    Material material = LoadDefaultMaterial();
    material.shader = pbrShader;
    dwarf.material = material;

    // Get shader locations
    int shaderViewLoc = GetShaderLocation(dwarf.material.shader, "viewPos");
    int shaderModelLoc = GetShaderLocation(dwarf.material.shader, "mMatrix");
    int shaderAlbedoLoc = GetShaderLocation(dwarf.material.shader, "albedo");
    int shaderMetallicLoc = GetShaderLocation(dwarf.material.shader, "metallic");    
    int shaderRoughnessLoc = GetShaderLocation(dwarf.material.shader, "roughness");
    int shaderAoLoc = GetShaderLocation(dwarf.material.shader, "ao");
    int shaderLightPosLoc[MAX_LIGHTS] = { -1 };
    int shaderLightColorLoc[MAX_LIGHTS] = { -1 };

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightPosName[32] = "lightPos[x]\0";
        lightPosName[9] = '0' + i;
        shaderLightPosLoc[i] = GetShaderLocation(dwarf.material.shader, lightPosName);
    }

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightColorName[32] = "lightColor[x]\0";
        lightColorName[11] = '0' + i;
        shaderLightColorLoc[i] = GetShaderLocation(dwarf.material.shader, lightColorName);
    }

    // Set up shader constant values
    float shaderAlbedo[3] = { 0.5f, 0.0f, 0.0f };
    SetShaderValue(dwarf.material.shader, shaderAlbedoLoc, shaderAlbedo, 3);
    float shaderAo[1] = { 1.0f };
    SetShaderValue(dwarf.material.shader, shaderAoLoc, shaderAo, 1);
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < MAX_LIGHTS; i++) SetShaderValue(dwarf.material.shader, shaderLightColorLoc[i], lightColor, 3);

    // Set our game to run at 60 frames-per-second
    SetTargetFPS(60);                           
    //------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //--------------------------------------------------------------------------
        // Update current rotation angle
        rotationAngle += 1.0f;

        // Check selected light inputs
        if (IsKeyPressed('1')) selectedLight = 0;
        else if (IsKeyPressed('2')) selectedLight = 1;
        else if (IsKeyPressed('3')) selectedLight = 2;
        else if (IsKeyPressed('4')) selectedLight = 3;

        // Check for light position movement inputs
        if (IsKeyDown(KEY_UP)) lightPosition[selectedLight].z += 0.1f;
        else if (IsKeyDown(KEY_DOWN)) lightPosition[selectedLight].z -= 0.1f;
        if (IsKeyDown(KEY_RIGHT)) lightPosition[selectedLight].x += 0.1f;
        else if (IsKeyDown(KEY_LEFT)) lightPosition[selectedLight].x -= 0.1f;
        if (IsKeyDown('W')) lightPosition[selectedLight].y += 0.1f;
        else if (IsKeyDown('S')) lightPosition[selectedLight].y -= 0.1f;

        // Update current light position
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            float lightPos[3] = { lightPosition[i].x,  lightPosition[i].y, lightPosition[i].z };
            SetShaderValue(dwarf.material.shader, shaderLightPosLoc[i], lightPos, 3);
        }

        // Update camera values and send them to shader
        UpdateCamera(&camera);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(dwarf.material.shader, shaderViewLoc, cameraPos, 3);
        //--------------------------------------------------------------------------

        // Draw
        //--------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            Begin3dMode(camera);

                // Draw models grid with parametric metalness and roughness values
                for (int rows = 0; rows < MAX_ROWS; rows++)
                {
                    float shaderMetallic[1] = { (float)rows/(float)MAX_ROWS };
                    SetShaderValue(dwarf.material.shader, shaderMetallicLoc, shaderMetallic, 1);

                    for (int col = 0; col < MAX_COLUMNS; col++)
                    {
                        float rough = (float)col/(float)MAX_COLUMNS;
                        ClampFloat(&rough, 0.05f, 1.0f);

                        float shaderRoughness[1] = { rough };
                        SetShaderValue(dwarf.material.shader, shaderRoughnessLoc, shaderRoughness, 1);

                        Matrix matScale = MatrixScale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
                        Matrix matRotation = MatrixRotate(rotationAxis, rotationAngle*DEG2RAD);
                        Matrix matTranslation = MatrixTranslate(rows*MODEL_OFFSET, 0.0f, col*MODEL_OFFSET);
                        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
                        SetShaderValueMatrix(dwarf.material.shader, shaderModelLoc, transform);

                        DrawModelEx(dwarf, (Vector3){ rows*MODEL_OFFSET, 0.0f, col*MODEL_OFFSET }, rotationAxis, rotationAngle, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE }, RED);
                    }
                }

                for (int i = 0; i < MAX_LIGHTS; i++)
                {
                    DrawSphere(lightPosition[i], 0.025f, YELLOW);
                    DrawSphereWires(lightPosition[i], 0.025f, 16, 16, ORANGE);
                }

                DrawGrid(10, 1.0f);

            End3dMode();

            DrawFPS(10, 10);

        EndDrawing();
        //--------------------------------------------------------------------------
    }

    // De-Initialization
    //------------------------------------------------------------------------------
    // Unload external resources
    UnloadShader(pbrShader);
    UnloadModel(dwarf);

    // Close window and OpenGL context
    CloseWindow();
    //------------------------------------------------------------------------------

    return 0;
}
