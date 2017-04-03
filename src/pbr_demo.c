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
#include <stdlib.h>                 // Required for: exit()
#include <stdio.h>                  // Required for: printf()
#include <string.h>                 // Required for: strcpy()

#include "raylib.h"                 // Required for raylib framework
#include "pbrmath.h"                // Required for matrix and vectors math

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"     // Required for image loading
#include "external/glad.h"          // Required for OpenGL API

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         MAX_LIGHTS          4               // Max lights supported by shader
#define         MAX_ROWS            7               // Rows to render models
#define         MAX_COLUMNS         7               // Columns to render models
#define         MODEL_SCALE         0.35f           // Model scale transformation for rendering
#define         MODEL_OFFSET        0.45f           // Distance between models for rendering

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
unsigned int LoadHighDynamicRange(const char *filename);        // Loads a high dynamic range (HDR) format file as linear float and converts it to a texture returning its id
void UnloadHighDynamicRange(unsigned int id);                   // Unloads a high dynamic range (HDR) created texture
void RenderCube();                                              // RenderCube() Renders a 1x1 3D cube in NDC

//----------------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //------------------------------------------------------------------------------
    int screenWidth = 1280;
    int screenHeight = 720;

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
    Shader cubeShader = LoadShader("resources/shaders/cubemap.vs", "resources/shaders/cubemap.fs");
    Shader skyShader = LoadShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader irradianceShader = LoadShader("resources/shaders/irradiance.vs", "resources/shaders/irradiance.fs");

    // Set up materials and lighting
    Material material = LoadDefaultMaterial();
    material.shader = pbrShader;
    dwarf.material = material;

    // Get PBR shader locations
    int shaderViewLoc = GetShaderLocation(dwarf.material.shader, "viewPos");
    int shaderModelLoc = GetShaderLocation(dwarf.material.shader, "mMatrix");
    int shaderAlbedoLoc = GetShaderLocation(dwarf.material.shader, "albedo");
    int shaderMetallicLoc = GetShaderLocation(dwarf.material.shader, "metallic");    
    int shaderRoughnessLoc = GetShaderLocation(dwarf.material.shader, "roughness");
    int shaderAoLoc = GetShaderLocation(dwarf.material.shader, "ao");
    int shaderLightPosLoc[MAX_LIGHTS] = { -1 };
    int shaderLightColorLoc[MAX_LIGHTS] = { -1 };

    for (unsigned int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightPosName[16] = "lightPos[x]\0";
        lightPosName[9] = '0' + i;
        shaderLightPosLoc[i] = GetShaderLocation(dwarf.material.shader, lightPosName);
    }

    for (unsigned int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightColorName[16] = "lightColor[x]\0";
        lightColorName[11] = '0' + i;
        shaderLightColorLoc[i] = GetShaderLocation(dwarf.material.shader, lightColorName);
    }

    // Get cubemap shader locations
    int equirectangularMapLoc = GetShaderLocation(cubeShader, "equirectangularMap");
    int cubeProjectionLoc = GetShaderLocation(cubeShader, "projection");
    int cubeViewLoc = GetShaderLocation(cubeShader, "view");

    // Get skybox shader locations
    int skyMapLoc = GetShaderLocation(skyShader, "environmentMap");
    int skyProjectionLoc = GetShaderLocation(skyShader, "projection");
    int skyViewLoc = GetShaderLocation(skyShader, "view");

    // Get irradiance shader locations
    int irradianceMapLoc = GetShaderLocation(irradianceShader, "environmentMap");
    int irradianceProjectionLoc = GetShaderLocation(irradianceShader, "projection");
    int irradianceViewLoc = GetShaderLocation(irradianceShader, "view");

    // Set up PBR shader constant values
    glUseProgram(dwarf.material.shader.id);
    glUniform1i(GetShaderLocation(dwarf.material.shader, "irradianceMap"), 0);
    glUniform1i(GetShaderLocation(dwarf.material.shader, "reflectionMap"), 1);
    glUniform1i(GetShaderLocation(dwarf.material.shader, "blurredMap"), 2);
    float shaderAlbedo[3] = { 1.0f, 1.0f, 1.0f };
    SetShaderValue(dwarf.material.shader, shaderAlbedoLoc, shaderAlbedo, 3);
    float shaderAo[1] = { 1.0f };
    SetShaderValue(dwarf.material.shader, shaderAoLoc, shaderAo, 1);
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    for (unsigned int i = 0; i < MAX_LIGHTS; i++) SetShaderValue(dwarf.material.shader, shaderLightColorLoc[i], lightColor, 3);

    // Set up cubemap shader constant values
    glUseProgram(cubeShader.id);
    glUniform1i(equirectangularMapLoc, 0);

    // Set up irradiance shader constant values
    glUseProgram(irradianceShader.id);
    glUniform1i(irradianceMapLoc, 0);

    // Set up skybox shader constant values
    glUseProgram(skyShader.id);
    glUniform1i(skyMapLoc, 0);

    // Set our game to run at 60 frames-per-second
    SetTargetFPS(60);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    // Load HDR environment
    unsigned int skyTex = LoadHighDynamicRange("resources/textures/skybox_apartament.hdr");

    // Set up framebuffer for skybox
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // Set up cubemap to render and attach to framebuffer
    // NOTE: faces are stored with 16 bit floating point values
    unsigned int cubeMap;
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create projection (transposed) and different views for each face
    Matrix captureProjection = MatrixPerspective(90.0f, 1.0f, 0.01, 1000.0);
    MatrixTranspose(&captureProjection);
    Matrix captureViews[6] = {
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f })
    };

    // Convert HDR equirectangular environment map to cubemap equivalent
    glUseProgram(cubeShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyTex);
    SetShaderValueMatrix(cubeShader, cubeProjectionLoc, captureProjection);

    glViewport(0, 0, 1024, 1024);     // Note: don't forget to configure the viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(cubeShader, cubeViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create an irradiance cubemap, and re-scale capture FBO to irradiance scale
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // Solve diffuse integral by convolution to create an irradiance cubemap
    glUseProgram(irradianceShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    SetShaderValueMatrix(irradianceShader, irradianceProjectionLoc, captureProjection);

    glViewport(0, 0, 32, 32);   // Note: don't forget to configure the viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(irradianceShader, irradianceViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Load blurred HDR environment
    unsigned int skyTexBlur = LoadHighDynamicRange("resources/textures/skybox_apartament_blur.hdr");

    // Set up framebuffer for skybox
    unsigned int captureFBOBlur, captureRBOBlur;
    glGenFramebuffers(1, &captureFBOBlur);
    glGenRenderbuffers(1, &captureRBOBlur);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBOBlur);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBOBlur);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBOBlur);

    // Set up cubemap to render and attach to framebuffer
    // NOTE: faces are stored with 16 bit floating point values
    unsigned int cubeMapBlur;
    glGenTextures(1, &cubeMapBlur);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBlur);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convert HDR equirectangular environment map to cubemap equivalent
    glUseProgram(cubeShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyTexBlur);
    SetShaderValueMatrix(cubeShader, cubeProjectionLoc, captureProjection);

    glViewport(0, 0, 1024, 1024);     // Note: don't forget to configure the viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBOBlur);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(cubeShader, cubeViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMapBlur, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Then before rendering, configure the viewport to the actual screen dimensions
    Matrix defaultProjection = MatrixPerspective(camera.fovy, (double)screenWidth/(double)screenHeight, 0.01, 1000.0);
    MatrixTranspose(&defaultProjection);
    SetShaderValueMatrix(cubeShader, cubeProjectionLoc, defaultProjection);
    SetShaderValueMatrix(skyShader, skyProjectionLoc, defaultProjection);
    SetShaderValueMatrix(irradianceShader, irradianceProjectionLoc, defaultProjection);

    // Reset viewport dimensions to default
    glViewport(0, 0, screenWidth, screenHeight);
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
        for (unsigned int i = 0; i < MAX_LIGHTS; i++)
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

            ClearBackground(DARKGRAY);

            Begin3dMode(camera);

                // Draw ground grid
                DrawGrid(10, 1.0f);

                // Draw models grid with parametric metalness and roughness values
                for (unsigned int rows = 0; rows < MAX_ROWS; rows++)
                {
                    float shaderMetallic[1] = { (float)rows/(float)MAX_ROWS };
                    SetShaderValue(dwarf.material.shader, shaderMetallicLoc, shaderMetallic, 1);

                    for (unsigned int col = 0; col < MAX_COLUMNS; col++)
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

                        // Enable irradiance map
                        glUseProgram(dwarf.material.shader.id);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

                        // Enable reflection map
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
                        
                        // Enable blurred reflection map
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBlur);

                        DrawModelEx(dwarf, (Vector3){ rows*MODEL_OFFSET, 0.0f, col*MODEL_OFFSET }, rotationAxis, rotationAngle, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE }, WHITE);
                        
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                        // Enable reflection map
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                    }
                }

                // Draw light gizmos
                for (unsigned int i = 0; i < MAX_LIGHTS; i++)
                {
                    DrawSphere(lightPosition[i], 0.025f, YELLOW);
                    DrawSphereWires(lightPosition[i], 0.025f, 16, 16, ORANGE);
                }

                // Calculate view matrix for custom shaders
                Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);

                // Render skybox (render as last to prevent overdraw)
                SetShaderValueMatrix(skyShader, skyViewLoc, view);
                glUseProgram(skyShader.id);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
                RenderCube();

            End3dMode();

            DrawFPS(10, 10);

        EndDrawing();
        //--------------------------------------------------------------------------
    }

    // De-Initialization
    //------------------------------------------------------------------------------
    // Unload external and allocated resources
    UnloadShader(dwarf.material.shader);
    UnloadShader(cubeShader);
    UnloadShader(skyShader);
    UnloadShader(irradianceShader);
    UnloadModel(dwarf);
    UnloadHighDynamicRange(skyTex);
    UnloadHighDynamicRange(skyTexBlur);
    UnloadHighDynamicRange(cubeMap);
    UnloadHighDynamicRange(cubeMapBlur);
    UnloadHighDynamicRange(irradianceMap);

    // Close window and OpenGL context
    CloseWindow();
    //------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------------
// Loads a high dynamic range (HDR) format file as linear float and converts it to a texture returning its id
unsigned int LoadHighDynamicRange(const char *filename)
{
    unsigned int hdrId;

    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, nrComponents = 0;
    float *data = stbi_loadf(filename, &width, &height, &nrComponents, 0);

    if (data)
    {
        glGenTextures(1, &hdrId);
        glBindTexture(GL_TEXTURE_2D, hdrId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data); 

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    return hdrId;
}

// Unloads a high dynamic range (HDR) created texture
void UnloadHighDynamicRange(unsigned int id)
{
    if (id != 0) glDeleteTextures(1, &id);
}

// RenderCube() Renders a 1x1 3D cube in NDC.
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void RenderCube()
{
    // Initialize (if necessary)
    if (cubeVAO == 0)
    {
        GLfloat vertices[] = {
            // Back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,   // Bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,    // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,    // bottom-right         
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,    // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,   // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,   // top-left
            // Front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,   // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,    // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,    // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,    // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,   // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,   // bottom-left
            // Left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,   // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,   // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,   // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,   // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,   // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,   // top-right
            // Right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,    // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,    // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,    // top-right         
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,    // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,    // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,    // bottom-left     
            // Bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,   // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,    // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,    // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,    // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,   // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,   // top-right
            // Top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,   // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,    // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,    // top-right     
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,    // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,   // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f    // bottom-left        
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        // Fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}