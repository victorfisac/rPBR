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

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"             // Required for image loading
#include "external/stb_image_write.h"       // Required for image saving
#include "external/glad.h"                  // Required for OpenGL API

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
#define         PATH_MODEL                  "resources/models/cerberus.obj"
#define         PATH_PBR_VS                 "resources/shaders/pbr.vs"
#define         PATH_PBR_FS                 "resources/shaders/pbr.fs"
#define         PATH_CUBE_VS                "resources/shaders/cubemap.vs"
#define         PATH_CUBE_FS                "resources/shaders/cubemap.fs"
#define         PATH_SKYBOX_VS              "resources/shaders/skybox.vs"
#define         PATH_SKYBOX_FS              "resources/shaders/skybox.fs"
#define         PATH_IRRADIANCE_FS          "resources/shaders/irradiance.fs"
#define         PATH_PREFILTER_FS           "resources/shaders/prefilter.fs"
#define         PATH_BRDF_VS                "resources/shaders/brdf.vs"
#define         PATH_BRDF_FS                "resources/shaders/brdf.fs"
#define         PATH_HDR                    "resources/textures/hdr/hdr_apartament.hdr"
#define         PATH_TEXTURES_ALBEDO        "resources/textures/cerberus/cerberus_albedo.png"
#define         PATH_TEXTURES_NORMALS       "resources/textures/cerberus/cerberus_normals.png"
#define         PATH_TEXTURES_METALLIC      "resources/textures/cerberus/cerberus_metallic.png"
#define         PATH_TEXTURES_ROUGHNESS     "resources/textures/cerberus/cerberus_roughness.png"
#define         PATH_TEXTURES_AO            "resources/textures/cerberus/cerberus_ao.png"

#define         MAX_LIGHTS                  4               // Max lights supported by shader
#define         MAX_ROWS                    1               // Rows to render models
#define         MAX_COLUMNS                 1               // Columns to render models
#define         MODEL_SCALE                 1.0f            // Model scale transformation for rendering
#define         MODEL_OFFSET                0.45f           // Distance between models for rendering
#define         ROTATION_SPEED              0.25f           // Models rotation speed
#define         CUBEMAP_SIZE                1024            // Cubemap texture size
#define         IRRADIANCE_SIZE             32              // Irradiance map from cubemap texture size
#define         PREFILTERED_SIZE            128             // Prefiltered HDR environment map texture size
#define         BRDF_SIZE                   512             // BRDF LUT texture map size

//----------------------------------------------------------------------------------
// Structs and enums
//----------------------------------------------------------------------------------
typedef enum {
    DEFAULT,
    ALBEDO,
    NORMALS,
    METALLIC,
    ROUGHNESS,
    AMBIENT_OCCLUSION,
    LIGHTING,
    FRESNEL,
    IRRADIANCE,
    REFLECTION
} RenderMode;

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
unsigned int LoadDynamicTexture(const char *filename);          // Loads a high dynamic range (HDR) format file as linear float and converts it to a texture returning its id
void UnloadDynamicTexture(unsigned int id);                     // Unloads a dynamic created texture
void CaptureScreenshot(int width, int height);                  // Take screenshot from screen and save it
unsigned char *ReadScreenPixels(int width, int height);         // Read screen pixel data (color buffer)
void RenderCube(void);                                          // Renders a 1x1 3D cube in NDC
void RenderQuad(void);                                          // Renders a 1x1 XY quad in NDC

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
    Vector3 lightPosition[MAX_LIGHTS] = { (Vector3){ -1.0f, 1.0f, -1.0f }, (Vector3){ 1.0, 1.0f, -1.0f }, (Vector3){ 1.0f, 1.0f, 1.0f }, (Vector3){ -1.0f, 1.0f, 1.0f } };
    Camera camera = {{ 2.75f, 2.55f, 2.75f }, { 1.0f, 1.05f, 1.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f };
    SetCameraMode(camera, CAMERA_FREE);
    int selectedLight = 0;
    RenderMode mode = DEFAULT;
    bool drawGrid = true;
    bool drawLights = true;
    bool drawSkybox = true;

    // Load external resources
    Model model = LoadModel(PATH_MODEL);
    Shader pbrShader = LoadShader(PATH_PBR_VS, PATH_PBR_FS);
    Shader cubeShader = LoadShader(PATH_CUBE_VS, PATH_CUBE_FS);
    Shader skyShader = LoadShader(PATH_SKYBOX_VS, PATH_SKYBOX_FS);
    Shader irradianceShader = LoadShader(PATH_SKYBOX_VS, PATH_IRRADIANCE_FS);
    Shader prefilterShader = LoadShader(PATH_SKYBOX_VS, PATH_PREFILTER_FS);
    Shader brdfShader = LoadShader(PATH_BRDF_VS, PATH_BRDF_FS);
    Texture2D albedoTex = LoadTexture(PATH_TEXTURES_ALBEDO);
    Texture2D normalsTex = LoadTexture(PATH_TEXTURES_NORMALS);
    Texture2D metallicTex = LoadTexture(PATH_TEXTURES_METALLIC);
    Texture2D roughnessTex = LoadTexture(PATH_TEXTURES_ROUGHNESS);
    Texture2D aoTex = LoadTexture(PATH_TEXTURES_AO);

    // Set up materials and lighting
    Material material = LoadDefaultMaterial();
    material.shader = pbrShader;
    model.material = material;

    // Get PBR shader locations
    int shaderModeLoc = GetShaderLocation(model.material.shader, "renderMode");
    int shaderViewLoc = GetShaderLocation(model.material.shader, "viewPos");
    int shaderModelLoc = GetShaderLocation(model.material.shader, "mMatrix");
    int shaderAlbedoLoc = GetShaderLocation(model.material.shader, "albedo.color");
    int shaderNormalsLoc = GetShaderLocation(model.material.shader, "normals.color");
    int shaderMetallicLoc = GetShaderLocation(model.material.shader, "metallic.color");    
    int shaderRoughnessLoc = GetShaderLocation(model.material.shader, "roughness.color");
    int shaderAoLoc = GetShaderLocation(model.material.shader, "ao.color");
    int shaderLightPosLoc[MAX_LIGHTS] = { -1 };
    int shaderLightColorLoc[MAX_LIGHTS] = { -1 };

    for (unsigned int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightPosName[16] = "lightPos[x]\0";
        lightPosName[9] = '0' + i;
        shaderLightPosLoc[i] = GetShaderLocation(model.material.shader, lightPosName);
    }

    for (unsigned int i = 0; i < MAX_LIGHTS; i++)
    {
        char lightColorName[16] = "lightColor[x]\0";
        lightColorName[11] = '0' + i;
        shaderLightColorLoc[i] = GetShaderLocation(model.material.shader, lightColorName);
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

    // Get prefilter shader locations
    int prefilterMapLoc = GetShaderLocation(prefilterShader, "environmentMap");
    int prefilterProjectionLoc = GetShaderLocation(prefilterShader, "projection");
    int prefilterViewLoc = GetShaderLocation(prefilterShader, "view");
    int prefilterRoughnessLoc = GetShaderLocation(prefilterShader, "roughness");

    // Set up PBR shader constant values
    glUseProgram(model.material.shader.id);
    glUniform1i(GetShaderLocation(model.material.shader, "albedo.useSampler"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "normals.useSampler"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "metallic.useSampler"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "roughness.useSampler"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "ao.useSampler"), 1);

    glUniform1i(GetShaderLocation(model.material.shader, "irradianceMap"), 0);
    glUniform1i(GetShaderLocation(model.material.shader, "prefilterMap"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "brdfLUT"), 2);
    glUniform1i(GetShaderLocation(model.material.shader, "albedo.sampler"), 3);
    glUniform1i(GetShaderLocation(model.material.shader, "normals.sampler"), 4);
    glUniform1i(GetShaderLocation(model.material.shader, "metallic.sampler"), 5);
    glUniform1i(GetShaderLocation(model.material.shader, "roughness.sampler"), 6);
    glUniform1i(GetShaderLocation(model.material.shader, "ao.sampler"), 7);
    float shaderAlbedo[3] = { 1.0f, 1.0f, 1.0f };
    SetShaderValue(model.material.shader, shaderAlbedoLoc, shaderAlbedo, 3);
    float shaderNormals[3] = { 0.5f, 0.5f, 1.0f };
    SetShaderValue(model.material.shader, shaderNormalsLoc, shaderNormals, 3);
    float shaderAo[3] = { 1.0f , 1.0f, 1.0f };
    SetShaderValue(model.material.shader, shaderAoLoc, shaderAo, 3);
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    for (unsigned int i = 0; i < MAX_LIGHTS; i++) SetShaderValue(model.material.shader, shaderLightColorLoc[i], lightColor, 3);

    // Set up cubemap shader constant values
    glUseProgram(cubeShader.id);
    glUniform1i(equirectangularMapLoc, 0);

    // Set up irradiance shader constant values
    glUseProgram(irradianceShader.id);
    glUniform1i(irradianceMapLoc, 0);
    
    // Set up prefilter shader constant values
    glUseProgram(prefilterShader.id);
    glUniform1i(prefilterMapLoc, 0);

    // Set up skybox shader constant values
    glUseProgram(skyShader.id);
    glUniform1i(skyMapLoc, 0);

    // Set our game to run at 60 frames-per-second
    SetTargetFPS(60);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Load HDR environment
    unsigned int skyTex = LoadDynamicTexture(PATH_HDR);

    // Set up framebuffer for skybox
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBEMAP_SIZE, CUBEMAP_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // Set up cubemap to render and attach to framebuffer
    // NOTE: faces are stored with 16 bit floating point values
    unsigned int cubeMap;
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, CUBEMAP_SIZE, CUBEMAP_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
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

    glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);     // Note: don't forget to configure the viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(cubeShader, cubeViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create an irradiance cubemap, and re-scale capture FBO to irradiance scale
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, IRRADIANCE_SIZE, IRRADIANCE_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

    // Solve diffuse integral by convolution to create an irradiance cubemap
    glUseProgram(irradianceShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    SetShaderValueMatrix(irradianceShader, irradianceProjectionLoc, captureProjection);

    glViewport(0, 0, IRRADIANCE_SIZE, IRRADIANCE_SIZE);   // Note: don't forget to configure the viewport to the capture dimensions
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

    // Create a prefiltered HDR environment map
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, PREFILTERED_SIZE, PREFILTERED_SIZE, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate mipmaps for the prefiltered HDR texture
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    // Prefilter HDR and store data into mipmap levels
    glUseProgram(prefilterShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    SetShaderValueMatrix(prefilterShader, prefilterProjectionLoc, captureProjection);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; mip++)
    {
        // Resize framebuffer according to mip-level size.
        unsigned int mipWidth  = PREFILTERED_SIZE*pow(0.5, mip);
        unsigned int mipHeight = PREFILTERED_SIZE*pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip/(float)(maxMipLevels - 1);
        glUniform1f(prefilterRoughnessLoc, roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            SetShaderValueMatrix(prefilterShader, prefilterViewLoc, captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);   
    
    // Generate BRDF convolution texture
    unsigned int brdfLut;
    glGenTextures(1, &brdfLut);
    glBindTexture(GL_TEXTURE_2D, brdfLut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, BRDF_SIZE, BRDF_SIZE, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Render BRDF LUT into a quad using default FBO
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, BRDF_SIZE, BRDF_SIZE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLut, 0);

    glViewport(0, 0, BRDF_SIZE, BRDF_SIZE);
    glUseProgram(brdfShader.id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Then before rendering, configure the viewport to the actual screen dimensions
    Matrix defaultProjection = MatrixPerspective(camera.fovy, (double)screenWidth/(double)screenHeight, 0.01, 1000.0);
    MatrixTranspose(&defaultProjection);
    SetShaderValueMatrix(cubeShader, cubeProjectionLoc, defaultProjection);
    SetShaderValueMatrix(skyShader, skyProjectionLoc, defaultProjection);
    SetShaderValueMatrix(irradianceShader, irradianceProjectionLoc, defaultProjection);
    SetShaderValueMatrix(prefilterShader, prefilterProjectionLoc, defaultProjection);

    // Reset viewport dimensions to default
    glViewport(0, 0, screenWidth, screenHeight);
    //------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //--------------------------------------------------------------------------
        // Update current rotation angle
        rotationAngle += ROTATION_SPEED;

        // Check selected light inputs
        if (IsKeyPressed(KEY_F1)) selectedLight = 0;
        else if (IsKeyPressed(KEY_F2)) selectedLight = 1;
        else if (IsKeyPressed(KEY_F3)) selectedLight = 2;
        else if (IsKeyPressed(KEY_F4)) selectedLight = 3;

        // Check for light position movement inputs
        if (IsKeyDown(KEY_UP)) lightPosition[selectedLight].z += 0.1f;
        else if (IsKeyDown(KEY_DOWN)) lightPosition[selectedLight].z -= 0.1f;
        if (IsKeyDown(KEY_RIGHT)) lightPosition[selectedLight].x += 0.1f;
        else if (IsKeyDown(KEY_LEFT)) lightPosition[selectedLight].x -= 0.1f;
        if (IsKeyDown(KEY_W)) lightPosition[selectedLight].y += 0.1f;
        else if (IsKeyDown(KEY_S)) lightPosition[selectedLight].y -= 0.1f;
        
        // Check for render mode inputs
        if (IsKeyPressed('1')) mode = DEFAULT;
        else if (IsKeyPressed('2')) mode = ALBEDO;
        else if (IsKeyPressed(KEY_THREE)) mode = NORMALS;
        else if (IsKeyPressed(KEY_FOUR)) mode = METALLIC;
        else if (IsKeyPressed(KEY_FIVE)) mode = ROUGHNESS;
        else if (IsKeyPressed(KEY_SIX)) mode = AMBIENT_OCCLUSION;
        else if (IsKeyPressed(KEY_SEVEN)) mode = LIGHTING;
        else if (IsKeyPressed(KEY_EIGHT)) mode = FRESNEL;
        else if (IsKeyPressed(KEY_NINE)) mode = IRRADIANCE;
        else if (IsKeyPressed(KEY_ZERO)) mode = REFLECTION;
        
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

        // Check for capture screenshot input
        if (IsKeyPressed(KEY_P)) CaptureScreenshot(screenWidth, screenHeight);

        // Send current mode to shader
        int shaderMode[1] = { (int)mode };
        SetShaderValuei(model.material.shader, shaderModeLoc, shaderMode, 1);

        // Update current light position
        for (unsigned int i = 0; i < MAX_LIGHTS; i++)
        {
            float lightPos[3] = { lightPosition[i].x,  lightPosition[i].y, lightPosition[i].z };
            SetShaderValue(model.material.shader, shaderLightPosLoc[i], lightPos, 3);
        }

        // Update camera values and send them to shader
        UpdateCamera(&camera);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(model.material.shader, shaderViewLoc, cameraPos, 3);
        //--------------------------------------------------------------------------

        // Draw
        //--------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(DARKGRAY);

            Begin3dMode(camera);

                // Draw ground grid
                if (drawGrid) DrawGrid(10, 1.0f);

                // Draw models grid with parametric metalness and roughness values
                for (unsigned int rows = 0; rows < MAX_ROWS; rows++)
                {
                    float shaderMetallic[3] = { (float)rows/(float)MAX_ROWS , 0.0f, 0.0f };
                    SetShaderValue(model.material.shader, shaderMetallicLoc, shaderMetallic, 3);

                    for (unsigned int col = 0; col < MAX_COLUMNS; col++)
                    {
                        float rough = (float)col/(float)MAX_COLUMNS;
                        ClampFloat(&rough, 0.05f, 1.0f);

                        float shaderRoughness[3] = { rough, 0.0f, 0.0f };
                        SetShaderValue(model.material.shader, shaderRoughnessLoc, shaderRoughness, 3);

                        Matrix matScale = MatrixScale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
                        Matrix matRotation = MatrixRotate(rotationAxis, rotationAngle*DEG2RAD);
                        Matrix matTranslation = MatrixTranslate(rows*MODEL_OFFSET, 0.0f, col*MODEL_OFFSET);
                        Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
                        SetShaderValueMatrix(model.material.shader, shaderModelLoc, transform);

                        // Enable and bind irradiance map
                        glUseProgram(model.material.shader.id);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

                        // Enable and bind prefiltered reflection map
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

                        // Enable and bind BRDF LUT map
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, brdfLut);

                        // Enable and bind albedo map
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, albedoTex.id);

                        // Enable and bind normals map
                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, normalsTex.id);

                        // Enable and bind metallic map
                        glActiveTexture(GL_TEXTURE5);
                        glBindTexture(GL_TEXTURE_2D, metallicTex.id);

                        // Enable and bind roughness map
                        glActiveTexture(GL_TEXTURE6);
                        glBindTexture(GL_TEXTURE_2D, roughnessTex.id);

                        // Enable and bind ambient occlusion map
                        glActiveTexture(GL_TEXTURE7);
                        glBindTexture(GL_TEXTURE_2D, aoTex.id);

                        // Draw model using PBR shader and textures maps
                        DrawModelEx(model, (Vector3){ rows*MODEL_OFFSET, 0.0f, col*MODEL_OFFSET }, rotationAxis, rotationAngle, (Vector3){ MODEL_SCALE, MODEL_SCALE, MODEL_SCALE }, WHITE);

                        // Disable and unbind irradiance map
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                        // Disable and unbind prefiltered reflection map
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                        // Disable and unbind BRDF LUT map
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Disable and unbind albedo map
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Disable and unbind normals map
                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Disable and unbind metallic map
                        glActiveTexture(GL_TEXTURE5);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Disable and unbind roughness map
                        glActiveTexture(GL_TEXTURE6);
                        glBindTexture(GL_TEXTURE_2D, 0);

                        // Disable and unbind ambient occlusion map
                        glActiveTexture(GL_TEXTURE7);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                }

                // Draw light gizmos
                for (unsigned int i = 0; ((i < MAX_LIGHTS) && drawLights); i++)
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
                if (drawSkybox) RenderCube();

            End3dMode();

            DrawFPS(10, 10);

        EndDrawing();
        //--------------------------------------------------------------------------
    }

    // De-Initialization
    //------------------------------------------------------------------------------
    // Unload external and allocated resources
    UnloadModel(model);
    UnloadTexture(albedoTex);
    UnloadTexture(normalsTex);
    UnloadTexture(metallicTex);
    UnloadTexture(roughnessTex);
    UnloadTexture(aoTex);
    UnloadShader(pbrShader);
    UnloadShader(cubeShader);
    UnloadShader(skyShader);
    UnloadShader(irradianceShader);
    UnloadShader(prefilterShader);
    UnloadShader(brdfShader);
    UnloadDynamicTexture(skyTex);
    UnloadDynamicTexture(cubeMap);
    UnloadDynamicTexture(irradianceMap);
    UnloadDynamicTexture(prefilterMap);
    UnloadDynamicTexture(brdfLut);

    // Close window and OpenGL context
    CloseWindow();
    //------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------------
// Loads a high dynamic range (HDR) format file as linear float and converts it to a texture returning its id
unsigned int LoadDynamicTexture(const char *filename)
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

// Unloads a dynamic created texture
void UnloadDynamicTexture(unsigned int id)
{
    if (id != 0) glDeleteTextures(1, &id);
}

// Take screenshot from screen and save it
void CaptureScreenshot(int width, int height)
{
    // Read screen pixels and save image as PNG
    unsigned char *imgData = ReadScreenPixels(width, height);
    stbi_write_png("screenshot.png", width, height, 4, imgData, width*4);
    free(imgData);
}

// Read screen pixel data (color buffer)
unsigned char *ReadScreenPixels(int width, int height)
{
    unsigned char *screenData = (unsigned char *)calloc(width*height*4, sizeof(unsigned char));

    // NOTE: glReadPixels returns image flipped vertically -> (0,0) is the bottom left corner of the framebuffer
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

    // Flip image vertically!
    unsigned char *imgData = (unsigned char *)malloc(width*height*sizeof(unsigned char)*4);

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < (width*4); x++)
        {
            // Flip line
            imgData[((height - 1) - y)*width*4 + x] = screenData[(y*width*4) + x];

            // Set alpha component value to 255 (no trasparent image retrieval)
            // NOTE: Alpha value has already been applied to RGB in framebuffer, we don't need it!
            if (((x + 1)%4) == 0) imgData[((height - 1) - y)*width*4 + x] = 255;
        }
    }

    free(screenData);

    return imgData;     // NOTE: image data should be freed
}

// Renders a 1x1 3D cube in NDC
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void RenderCube(void)
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

        // Set up cube VAO
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

    // Render cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// Renders a 1x1 XY quad in NDC
GLuint quadVAO = 0;
GLuint quadVBO;
void RenderQuad(void)
{
    if (quadVAO == 0) 
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        // Set up plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);

        // Fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        // Link vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    }

    // Render quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
