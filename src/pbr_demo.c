/*******************************************************************************************
*
*   pbraylib - Physically Based Rendering for raylib
*
*   Copyright (c) 2017 Victor Fisac
*
********************************************************************************************/

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include "raylib.h"
#include "pbrmath.h"

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         MAX_ROWS            7
#define         MAX_COLUMNS         7
#define         MODEL_SCALE         0.35f
#define         MODEL_OFFSET        0.35f

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 800;
    int screenHeight = 450;

    // Enable Multi Sampling Anti Aliasing 4x (if available)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "pbraylib - Physically Based Rendering");

    // Define the camera to look into our 3d world, its mode and model drawing position
    float rotationAngle = 0.0f;
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
    Vector3 lightPosition = { 1.125f, 1.0f, 1.125f };
    Camera camera = {{ 3.75f, 2.25f, 3.75f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f };
    SetCameraMode(camera, CAMERA_FREE);

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
    int shaderLightPosLoc[1] = { -1 };
    int shaderLightColorLoc[1] = { -1 };
    for (int i = 0; i < 1; i++) shaderLightPosLoc[i] = GetShaderLocation(dwarf.material.shader, "lightPos");
    for (int i = 0; i < 1; i++) shaderLightColorLoc[i] = GetShaderLocation(dwarf.material.shader, "lightColor");
    
    // Set up shader constant values
    float shaderAlbedo[3] = { 0.5f, 0.0f, 0.0f };
    SetShaderValue(dwarf.material.shader, shaderAlbedoLoc, shaderAlbedo, 3);
    float shaderAo[1] = { 1.0f };
    SetShaderValue(dwarf.material.shader, shaderAoLoc, shaderAo, 1);
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    for (int i = 0; i < 1; i++) SetShaderValue(dwarf.material.shader, shaderLightColorLoc[i], lightColor, 3);

    // Set our game to run at 60 frames-per-second
    SetTargetFPS(60);                           
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        // Update current rotation angle
        rotationAngle += 1.0f;
        
        // Check for light position movement inputs
        if (IsKeyDown(KEY_UP)) lightPosition.z += 0.1f;
        else if (IsKeyDown(KEY_DOWN)) lightPosition.z -= 0.1f;
        if (IsKeyDown(KEY_RIGHT)) lightPosition.x += 0.1f;
        else if (IsKeyDown(KEY_LEFT)) lightPosition.x -= 0.1f;
        if (IsKeyDown('W')) lightPosition.y += 0.1f;
        else if (IsKeyDown('S')) lightPosition.y -= 0.1f;
        
        // Update current light position
        float lightPos[3] = { lightPosition.x,  lightPosition.y, lightPosition.z };
        for (int i = 0; i < 1; i++) SetShaderValue(dwarf.material.shader, shaderLightPosLoc[i], lightPos, 3);
        
        // Update camera values and send them to shader
        UpdateCamera(&camera);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(dwarf.material.shader, shaderViewLoc, cameraPos, 3);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
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
                        if (rough < 0.05f) rough = 0.05f;
                        else if (rough > 1.0f) rough = 1.0f;
                        
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
                
                DrawSphere(lightPosition, 0.05f, YELLOW);
                
                DrawGrid(10, 1.0f);

            End3dMode();

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload external resources
    UnloadShader(pbrShader);
    UnloadModel(dwarf);

    // Close window and OpenGL context
    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}
