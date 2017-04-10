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
#include "pbr3d.h"                          // Required for 3D drawing functions

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

typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT
} LightType;

typedef struct {
    bool enabled;
    LightType type;
    Vector3 position;
    Vector3 target;
    Color color;
    int enabledLoc;
    int typeLoc;
    int posLoc;
    int targetLoc;
    int colorLoc;
} Light;

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------
unsigned int LoadDynamicTexture(const char *filename);                  // Loads a high dynamic range (HDR) format file as linear float and converts it to a texture returning its id
void UnloadDynamicTexture(unsigned int id);                             // Unloads a dynamic created texture
void CaptureScreenshot(int width, int height);                          // Take screenshot from screen and save it

Light CreateLight(int type, Vector3 pos, Vector3 targ, 
                  Color color, Shader shader, int *lightCount);         // Defines a light type, position, target, color and get locations from shader
void UpdateLightValues(Shader shader, Light light);                     // Send to shader light values
void DrawLight(Light light);                                            // Draw a light gizmo based on light attributes

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

    // Load external resources
    Model model = LoadModel(PATH_MODEL);
    Shader pbrShader = LoadShader(PATH_PBR_VS, PATH_PBR_FS);
    Shader cubeShader = LoadShader(PATH_CUBE_VS, PATH_CUBE_FS);
    Shader skyShader = LoadShader(PATH_SKYBOX_VS, PATH_SKYBOX_FS);
    Shader irradianceShader = LoadShader(PATH_SKYBOX_VS, PATH_IRRADIANCE_FS);
    Shader prefilterShader = LoadShader(PATH_SKYBOX_VS, PATH_PREFILTER_FS);
    Shader brdfShader = LoadShader(PATH_BRDF_VS, PATH_BRDF_FS);
    Texture2D albedoTex = ((useAlbedoMap) ? LoadTexture(PATH_TEXTURES_ALBEDO) : (Texture2D){ 0 });;
    Texture2D normalsTex = ((useNormalMap) ? LoadTexture(PATH_TEXTURES_NORMALS) : (Texture2D){ 0 });
    Texture2D metallicTex = ((useMetallicMap) ? LoadTexture(PATH_TEXTURES_METALLIC) : (Texture2D){ 0 });
    Texture2D roughnessTex = ((useRoughnessMap) ? LoadTexture(PATH_TEXTURES_ROUGHNESS) : (Texture2D){ 0 });
    Texture2D aoTex = ((useOcclusionMap) ? LoadTexture(PATH_TEXTURES_AO) : (Texture2D){ 0 });
    Texture2D heightTex = ((useParallaxMap) ? LoadTexture(PATH_TEXTURES_HEIGHT) : (Texture2D){ 0 });

    // Set up materials and lighting
    Material material = { 0 };
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
    int shaderHeightLoc = GetShaderLocation(model.material.shader, "height.color");

    // Define lights attributes
    int lightsCount = 0;
    Light lights[MAX_LIGHTS] = { 0 };
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ -1.0f, 1.0f, -1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 255, 0, 0, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ 1.0f, 1.0f, -1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 0, 255, 0, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_POINT, (Vector3){ -1.0f, 1.0f, 1.0f }, (Vector3){ 0, 0, 0 }, (Color){ 0, 0, 255, 255 }, model.material.shader, &lightsCount);
    lights[lightsCount] = CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 3.0f, 2.0f, 3.0f }, (Vector3){ 0, 0, 0 }, (Color){ 255, 0, 255, 255 }, model.material.shader, &lightsCount);

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
    glUniform1i(GetShaderLocation(model.material.shader, "albedo.useSampler"), useAlbedoMap);
    glUniform1i(GetShaderLocation(model.material.shader, "normals.useSampler"), useNormalMap);
    glUniform1i(GetShaderLocation(model.material.shader, "metallic.useSampler"), useMetallicMap);
    glUniform1i(GetShaderLocation(model.material.shader, "roughness.useSampler"), useRoughnessMap);
    glUniform1i(GetShaderLocation(model.material.shader, "ao.useSampler"), useOcclusionMap);
    glUniform1i(GetShaderLocation(model.material.shader, "height.useSampler"), useParallaxMap);

    // Set up PBR shader texture units
    glUniform1i(GetShaderLocation(model.material.shader, "irradianceMap"), 0);
    glUniform1i(GetShaderLocation(model.material.shader, "prefilterMap"), 1);
    glUniform1i(GetShaderLocation(model.material.shader, "brdfLUT"), 2);
    glUniform1i(GetShaderLocation(model.material.shader, "albedo.sampler"), 3);
    glUniform1i(GetShaderLocation(model.material.shader, "normals.sampler"), 4);
    glUniform1i(GetShaderLocation(model.material.shader, "metallic.sampler"), 5);
    glUniform1i(GetShaderLocation(model.material.shader, "roughness.sampler"), 6);
    glUniform1i(GetShaderLocation(model.material.shader, "ao.sampler"), 7);
    glUniform1i(GetShaderLocation(model.material.shader, "height.sampler"), 8);

    // Set up material uniforms and other constant values
    float shaderAlbedo[3] = { 1.0f, 1.0f, 1.0f };
    SetShaderValue(model.material.shader, shaderAlbedoLoc, shaderAlbedo, 3);
    float shaderNormals[3] = { 0.5f, 0.5f, 1.0f };
    SetShaderValue(model.material.shader, shaderNormalsLoc, shaderNormals, 3);
    float shaderAo[3] = { 1.0f , 1.0f, 1.0f };
    SetShaderValue(model.material.shader, shaderAoLoc, shaderAo, 3);
    if (useParallaxMap)
    {
        float shaderHeight[3] = { 0.1f , 0.0f, 0.0f };
        SetShaderValue(model.material.shader, shaderHeightLoc, shaderHeight, 3);
    }

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
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(model.material.shader, lights[i]);

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

                        if (useAlbedoMap)
                        {
                            // Enable and bind albedo map
                            glActiveTexture(GL_TEXTURE3);
                            glBindTexture(GL_TEXTURE_2D, albedoTex.id);
                        }

                        if (useNormalMap)
                        {
                            // Enable and bind normals map
                            glActiveTexture(GL_TEXTURE4);
                            glBindTexture(GL_TEXTURE_2D, normalsTex.id);
                        }

                        if (useMetallicMap)
                        {
                            // Enable and bind metallic map
                            glActiveTexture(GL_TEXTURE5);
                            glBindTexture(GL_TEXTURE_2D, metallicTex.id);
                        }

                        if (useRoughnessMap)
                        {
                            // Enable and bind roughness map
                            glActiveTexture(GL_TEXTURE6);
                            glBindTexture(GL_TEXTURE_2D, roughnessTex.id);
                        }

                        if (useOcclusionMap)
                        {
                            // Enable and bind ambient occlusion map
                            glActiveTexture(GL_TEXTURE7);
                            glBindTexture(GL_TEXTURE_2D, aoTex.id);
                        }

                        if (useParallaxMap)
                        {
                            // Enable and bind parallax height map
                            glActiveTexture(GL_TEXTURE8);
                            glBindTexture(GL_TEXTURE_2D, heightTex.id);
                        }

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

                        if (useAlbedoMap)
                        {
                            // Disable and bind albedo map
                            glActiveTexture(GL_TEXTURE3);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }

                        if (useNormalMap)
                        {
                            // Disable and bind normals map
                            glActiveTexture(GL_TEXTURE4);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }

                        if (useMetallicMap)
                        {
                            // Disable and bind metallic map
                            glActiveTexture(GL_TEXTURE5);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }

                        if (useRoughnessMap)
                        {
                            // Disable and bind roughness map
                            glActiveTexture(GL_TEXTURE6);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }

                        if (useOcclusionMap)
                        {
                            // Disable and bind ambient occlusion map
                            glActiveTexture(GL_TEXTURE7);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }

                        if (useParallaxMap)
                        {
                            // Disable and bind parallax height map
                            glActiveTexture(GL_TEXTURE8);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }
                    }
                }

                // Draw light gizmos
                for (unsigned int i = 0; ((i < MAX_LIGHTS) && drawLights); i++) DrawLight(lights[i]);

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
    // Unload loaded model mesh and binded textures
    UnloadModel(model);
    if (useAlbedoMap) UnloadTexture(albedoTex);
    if (useNormalMap) UnloadTexture(normalsTex);
    if (useMetallicMap) UnloadTexture(metallicTex);
    if (useRoughnessMap) UnloadTexture(roughnessTex);
    if (useOcclusionMap) UnloadTexture(aoTex);
    if (useParallaxMap) UnloadTexture(heightTex);

    // Unload all loaded shaders
    UnloadShader(pbrShader);
    UnloadShader(cubeShader);
    UnloadShader(skyShader);
    UnloadShader(irradianceShader);
    UnloadShader(prefilterShader);
    UnloadShader(brdfShader);

    // Unload dynamic textures created in program initialization
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
    stbi_write_png("screenshot.png", width, height, 4, imgData, width*4);
    free(imgData);
}

// Defines a light type, position, target, color and get locations from shader
Light CreateLight(int type, Vector3 pos, Vector3 targ, Color color, Shader shader, int *lightCount)
{
    Light light = { 0 };

    light.enabled = true;
    light.type = type;
    light.position = pos;
    light.target = targ;
    light.color = color;

    char enabledName[32] = "lights[x].enabled\0";
    char typeName[32] = "lights[x].type\0";
    char posName[32] = "lights[x].position\0";
    char targetName[32] = "lights[x].target\0";
    char colorName[32] = "lights[x].color\0";
    enabledName[7] = '0' + *lightCount;
    typeName[7] = '0' + *lightCount;
    posName[7] = '0' + *lightCount;
    targetName[7] = '0' + *lightCount;
    colorName[7] = '0' + *lightCount;

    light.enabledLoc = GetShaderLocation(shader, enabledName);
    light.typeLoc = GetShaderLocation(shader, typeName);
    light.posLoc = GetShaderLocation(shader, posName);
    light.targetLoc = GetShaderLocation(shader, targetName);
    light.colorLoc = GetShaderLocation(shader, colorName);

    UpdateLightValues(shader, light);
    *lightCount += 1;

    return light;
}

// Send to shader light values
void UpdateLightValues(Shader shader, Light light)
{
    glUniform1i(light.enabledLoc, light.enabled);
    glUniform1i(light.typeLoc, light.type);
    float position[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(shader, light.posLoc, position, 3);
    float target[3] = { light.target.x, light.target.y, light.target.z };
    SetShaderValue(shader, light.targetLoc, target, 3);
    float diff[4] = { (float)light.color.r/(float)255, (float)light.color.g/(float)255, (float)light.color.b/(float)255, (float)light.color.a/(float)255 };
    SetShaderValue(shader, light.colorLoc, diff, 4);
}

// Draw a light gizmo based on light attributes
void DrawLight(Light light)
{
    switch (light.type)
    {
        case LIGHT_DIRECTIONAL:
        {
            DrawSphere(light.position, 0.015f, (light.enabled ? light.color : GRAY));
            DrawSphere(light.target, 0.015f, (light.enabled ? light.color : GRAY));
            DrawLine3D(light.position, light.target, (light.enabled ? light.color : DARKGRAY));
        } break;
        case LIGHT_POINT: DrawSphere(light.position, 0.025f, (light.enabled ? light.color : GRAY));
        default: break;
    }
}
