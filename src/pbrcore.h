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
#include <math.h>                           // Required for: pow()

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"             // Required for image loading
#include "external/glad.h"                  // Required for OpenGL API

//----------------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------
// Structs and enums
//----------------------------------------------------------------------------------
typedef enum {
    BACKGROUND_SKY,
    BACKGROUND_BLURSKY,
    BACKGROUND_AMBIENT
} BackgroundMode;

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

typedef struct Environment {
    unsigned int cubemapId;
    unsigned int irradianceId;
    unsigned int prefilterId;
    unsigned int brdfId;
    Shader pbrShader;
    Shader cubeShader;
    Shader skyShader;
    Shader irradianceShader;
    Shader prefilterShader;
    Shader brdfShader;
    int pbrViewLoc;
    int skyViewLoc;
    int skyResolutionLoc;
} Environment;

typedef struct MaterialPBR {
    Texture2D albedoTex;
    Texture2D normalsTex;
    Texture2D metalnessTex;
    Texture2D roughnessTex;
    Texture2D aoTex;
    Texture2D emissionTex;
    Texture2D heightTex;
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetalnessMap;
    bool useRoughnessMap;
    bool useOcclusionMap;
    bool useEmissionMap;
    bool useParallaxMap;
    Color albedoColor;
    Color normalsColor;
    Color metalColor;
    Color roughnessColor;
    Color aoColor;
    Color emissionColor;
    Color heightColor;
    Environment env;
} MaterialPBR;

typedef enum TypePBR {
    PBR_ALBEDO,
    PBR_NORMALS,
    PBR_METALNESS,
    PBR_ROUGHNESS,
    PBR_AO,
    PBR_EMISSION,
    PBR_HEIGHT
} TypePBR;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightsCount = 0;                     // Current amount of created lights 

//----------------------------------------------------------------------------------
// Functions Declaration
//----------------------------------------------------------------------------------
MaterialPBR SetupMaterialPBR(Environment env, Color albedo, int metalness, int roughness);                                      // Set up PBR environment shader constant values
void SetMaterialTexturePBR(MaterialPBR *mat, TypePBR type, Texture2D texture);                                                  // Set texture to PBR material
Light CreateLight(int type, Vector3 pos, Vector3 targ, Color color, Environment env);                                           // Defines a light and get locations from environment PBR shader
Environment LoadEnvironment(const char *filename, int cubemapSize, int irradianceSize, int prefilterSize, int brdfSize);        // Load an environment cubemap, irradiance, prefilter and PBR scene

int GetLightsCount(void);                                                                                                       // Get the current amount of created lights
void UpdateLightsValues(Environment env, Light light);                                                                          // Send to environment PBR shader lights values
void UpdateEnvironmentValues(Environment env, Camera camera, Vector2 res);                                                      // Send to environment PBR shader camera view and resolution values

void DrawModelPBR(Model model, MaterialPBR mat, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale);    // Draw a model using physically based rendering
void DrawSkybox(Environment environment, int mode, Camera camera);                                                              // Draw a cube skybox using environment cube map
void RenderCube(void);                                                                                                          // Renders a 1x1 3D cube in NDC
void RenderQuad(void);                                                                                                          // Renders a 1x1 XY quad in NDC

void UnloadMaterialPBR(MaterialPBR mat);                                                                                        // Unload material PBR textures
void UnloadEnvironment(Environment env);                                                                                        // Unload environment loaded shaders and dynamic textures

//----------------------------------------------------------------------------------
// Functions Definition
//----------------------------------------------------------------------------------
// Set up PBR environment shader constant values
MaterialPBR SetupMaterialPBR(Environment env, Color albedo, int metalness, int roughness)
{
    MaterialPBR mat;

    mat.albedoColor = albedo;
    mat.normalsColor = (Color){ 128, 128, 255, 255 };
    mat.metalColor = (Color){ metalness, 0, 0, 0 };
    mat.roughnessColor = (Color){ roughness, 0, 0, 0 };
    mat.aoColor = (Color){ 255, 255, 255, 255 };
    mat.heightColor = (Color){ 0, 0, 0, 0 };
    mat.emissionColor = (Color){ 0, 0, 0, 0 };

    mat.env = env;

    // Set up PBR shader material texture units
    glUseProgram(mat.env.pbrShader.id);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "albedo.sampler"), 3);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "normals.sampler"), 4);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "metalness.sampler"), 5);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "roughness.sampler"), 6);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "ao.sampler"), 7);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "emission.sampler"), 8);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "height.sampler"), 9);

    return mat;
}

// Set texture to PBR material
void SetMaterialTexturePBR(MaterialPBR *mat, TypePBR type, Texture2D texture)
{
    // Apply changes to material and send sampler use state to material shader
    glUseProgram(mat->env.pbrShader.id);

    switch (type)
    {
        case PBR_ALBEDO:
        {
            mat->albedoTex = texture;
            mat->useAlbedoMap = true;
        } break;
        case PBR_NORMALS:
        {
            mat->normalsTex = texture;
            mat->useNormalMap = true;
        } break;
        case PBR_METALNESS:
        {
            mat->metalnessTex = texture;
            mat->useMetalnessMap = true;
        } break;
        case PBR_ROUGHNESS:
        {
            mat->roughnessTex = texture;
            mat->useRoughnessMap = true;
        } break;
        case PBR_AO:
        {
            mat->aoTex = texture;
            mat->useOcclusionMap = true;
        } break;
        case PBR_EMISSION:
        {
            mat->emissionTex = texture;
            mat->useEmissionMap = true;
        } break;
        case PBR_HEIGHT:
        {
            mat->heightTex = texture;
            mat->useParallaxMap = true;
        } break;
        default: break;
    }
}

// Defines a light and get locations from environment shader
Light CreateLight(int type, Vector3 pos, Vector3 targ, Color color, Environment env)
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
    enabledName[7] = '0' + lightsCount;
    typeName[7] = '0' + lightsCount;
    posName[7] = '0' + lightsCount;
    targetName[7] = '0' + lightsCount;
    colorName[7] = '0' + lightsCount;

    light.enabledLoc = GetShaderLocation(env.pbrShader, enabledName);
    light.typeLoc = GetShaderLocation(env.pbrShader, typeName);
    light.posLoc = GetShaderLocation(env.pbrShader, posName);
    light.targetLoc = GetShaderLocation(env.pbrShader, targetName);
    light.colorLoc = GetShaderLocation(env.pbrShader, colorName);

    UpdateLightsValues(env, light);
    lightsCount++;

    return light;
}

// Load an environment cubemap, irradiance, prefilter and PBR scene
Environment LoadEnvironment(const char *filename, int cubemapSize, int irradianceSize, int prefilterSize, int brdfSize)
{
    Environment env = { 0 };

    // Load environment required shaders
    env.pbrShader = LoadShader(PATH_PBR_VS, PATH_PBR_FS);
    env.cubeShader = LoadShader(PATH_CUBE_VS, PATH_CUBE_FS);
    env.skyShader = LoadShader(PATH_SKYBOX_VS, PATH_SKYBOX_FS);
    env.irradianceShader = LoadShader(PATH_SKYBOX_VS, PATH_IRRADIANCE_FS);
    env.prefilterShader = LoadShader(PATH_SKYBOX_VS, PATH_PREFILTER_FS);
    env.brdfShader = LoadShader(PATH_BRDF_VS, PATH_BRDF_FS);

    // Get cubemap shader locations
    int equirectangularMapLoc = GetShaderLocation(env.cubeShader, "equirectangularMap");
    int cubeProjectionLoc = GetShaderLocation(env.cubeShader, "projection");
    int cubeViewLoc = GetShaderLocation(env.cubeShader, "view");

    // Get skybox shader locations
    int skyMapLoc = GetShaderLocation(env.skyShader, "environmentMap");
    int skyProjectionLoc = GetShaderLocation(env.skyShader, "projection");
    env.skyViewLoc = GetShaderLocation(env.skyShader, "view");
    env.skyResolutionLoc = GetShaderLocation(env.skyShader, "resolution");

    // Get irradiance shader locations
    int irradianceMapLoc = GetShaderLocation(env.irradianceShader, "environmentMap");
    int irradianceProjectionLoc = GetShaderLocation(env.irradianceShader, "projection");
    int irradianceViewLoc = GetShaderLocation(env.irradianceShader, "view");

    // Get prefilter shader locations
    int prefilterMapLoc = GetShaderLocation(env.prefilterShader, "environmentMap");
    int prefilterProjectionLoc = GetShaderLocation(env.prefilterShader, "projection");
    int prefilterViewLoc = GetShaderLocation(env.prefilterShader, "view");
    int prefilterRoughnessLoc = GetShaderLocation(env.prefilterShader, "roughness");

    // Set up environment shader texture units
    glUseProgram(env.pbrShader.id);
    glUniform1i(GetShaderLocation(env.pbrShader, "irradianceMap"), 0);
    glUniform1i(GetShaderLocation(env.pbrShader, "prefilterMap"), 1);
    glUniform1i(GetShaderLocation(env.pbrShader, "brdfLUT"), 2);

    // Set up cubemap shader constant values
    glUseProgram(env.cubeShader.id);
    glUniform1i(equirectangularMapLoc, 0);

    // Set up irradiance shader constant values
    glUseProgram(env.irradianceShader.id);
    glUniform1i(irradianceMapLoc, 0);

    // Set up prefilter shader constant values
    glUseProgram(env.prefilterShader.id);
    glUniform1i(prefilterMapLoc, 0);

    // Set up skybox shader constant values
    glUseProgram(env.skyShader.id);
    glUniform1i(skyMapLoc, 0);

    // Set up depth face culling and cube map seamless
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glLineWidth(2);

    // Load HDR environment texture
    unsigned int skyTex = 0;
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, nrComponents = 0;
    float *data = stbi_loadf(filename, &width, &height, &nrComponents, 0);

    if (data)
    {
        glGenTextures(1, &skyTex);
        glBindTexture(GL_TEXTURE_2D, skyTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    // Set up framebuffer for skybox
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // Set up cubemap to render and attach to framebuffer
    // NOTE: faces are stored with 16 bit floating point values
    glGenTextures(1, &env.cubemapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemapId);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, cubemapSize, cubemapSize, 0, GL_RGB, GL_FLOAT, NULL);
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
    glUseProgram(env.cubeShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyTex);
    SetShaderValueMatrix(env.cubeShader, cubeProjectionLoc, captureProjection);

    // Note: don't forget to configure the viewport to the capture dimensions
    glViewport(0, 0, cubemapSize, cubemapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(env.cubeShader, cubeViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.cubemapId, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create an irradiance cubemap, and re-scale capture FBO to irradiance scale
    glGenTextures(1, &env.irradianceId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.irradianceId);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceSize, irradianceSize, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceSize, irradianceSize);

    // Solve diffuse integral by convolution to create an irradiance cubemap
    glUseProgram(env.irradianceShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemapId);
    SetShaderValueMatrix(env.irradianceShader, irradianceProjectionLoc, captureProjection);

    // Note: don't forget to configure the viewport to the capture dimensions
    glViewport(0, 0, irradianceSize, irradianceSize);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(env.irradianceShader, irradianceViewLoc, captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.irradianceId, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderCube();
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create a prefiltered HDR environment map
    glGenTextures(1, &env.prefilterId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.prefilterId);
    for (unsigned int i = 0; i < 6; i++) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilterSize, prefilterSize, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate mipmaps for the prefiltered HDR texture
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Prefilter HDR and store data into mipmap levels
    glUseProgram(env.prefilterShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env.cubemapId);
    SetShaderValueMatrix(env.prefilterShader, prefilterProjectionLoc, captureProjection);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;

    for (unsigned int mip = 0; mip < maxMipLevels; mip++)
    {
        // Resize framebuffer according to mip-level size.
        unsigned int mipWidth  = prefilterSize*pow(0.5, mip);
        unsigned int mipHeight = prefilterSize*pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip/(float)(maxMipLevels - 1);
        glUniform1f(prefilterRoughnessLoc, roughness);

        for (unsigned int i = 0; i < 6; ++i)
        {
            SetShaderValueMatrix(env.prefilterShader, prefilterViewLoc, captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env.prefilterId, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Generate BRDF convolution texture
    glGenTextures(1, &env.brdfId);
    glBindTexture(GL_TEXTURE_2D, env.brdfId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdfSize, brdfSize, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Render BRDF LUT into a quad using default FBO
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdfSize, brdfSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, env.brdfId, 0);

    glViewport(0, 0, brdfSize, brdfSize);
    glUseProgram(env.brdfShader.id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad();

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Then before rendering, configure the viewport to the actual screen dimensions
    Matrix defaultProjection = MatrixPerspective(60.0, (double)GetScreenWidth()/(double)GetScreenHeight(), 0.01, 1000.0);
    MatrixTranspose(&defaultProjection);
    SetShaderValueMatrix(env.cubeShader, cubeProjectionLoc, defaultProjection);
    SetShaderValueMatrix(env.skyShader, skyProjectionLoc, defaultProjection);
    SetShaderValueMatrix(env.irradianceShader, irradianceProjectionLoc, defaultProjection);
    SetShaderValueMatrix(env.prefilterShader, prefilterProjectionLoc, defaultProjection);
    env.pbrViewLoc = GetShaderLocation(env.pbrShader, "viewPos");

    // Reset viewport dimensions to default
    glViewport(0, 0, GetScreenWidth(), GetScreenHeight());

    return env;
}

// Get the current amount of created lights
int GetLightsCount(void)
{
    return lightsCount;
}

// Send to environment PBR shader lights values
void UpdateLightsValues(Environment env, Light light)
{
    // Send to shader light enabled state and type
    glUniform1i(light.enabledLoc, light.enabled);
    glUniform1i(light.typeLoc, light.type);

    // Send to shader light position values
    float position[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(env.pbrShader, light.posLoc, position, 3);

    // Send to shader light target position values
    float target[3] = { light.target.x, light.target.y, light.target.z };
    SetShaderValue(env.pbrShader, light.targetLoc, target, 3);

    // Send to shader light color values
    float diff[4] = { (float)light.color.r/(float)255, (float)light.color.g/(float)255, (float)light.color.b/(float)255, (float)light.color.a/(float)255 };
    SetShaderValue(env.pbrShader, light.colorLoc, diff, 4);
}

// Send to environment PBR shader camera view and resolution values
void UpdateEnvironmentValues(Environment env, Camera camera, Vector2 res)
{
    // Send to shader camera view position
    float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
    SetShaderValue(env.pbrShader, env.pbrViewLoc, cameraPos, 3);

    // Send to shader screen resolution
    float resolution[2] = { res.x, res.y };
    SetShaderValue(env.skyShader, env.skyResolutionLoc, resolution, 2);
}

// Draw a model using physically based rendering
void DrawModelPBR(Model model, MaterialPBR mat, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale)
{
    // Switch to PBR shader
    glUseProgram(mat.env.pbrShader.id);

    // Set up material uniforms and other constant values
    float shaderAlbedo[3] = { (float)mat.albedoColor.r/(float)255, (float)mat.albedoColor.g/(float)255, (float)mat.albedoColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "albedo.color"), shaderAlbedo, 3);
    float shaderNormals[3] = { (float)mat.normalsColor.r/(float)255, (float)mat.normalsColor.g/(float)255, (float)mat.normalsColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "normals.color"), shaderNormals, 3);
    float shaderMetalness[3] = { (float)mat.metalColor.r/(float)255, (float)mat.metalColor.g/(float)255, (float)mat.metalColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "metalness.color"), shaderMetalness, 3);
    float shaderRoughness[3] = { 1.0f - (float)mat.roughnessColor.r/(float)255, 1.0f - (float)mat.roughnessColor.g/(float)255, 1.0f - (float)mat.roughnessColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "roughness.color"), shaderRoughness, 3);
    float shaderAo[3] = { (float)mat.aoColor.r/(float)255, (float)mat.aoColor.g/(float)255, (float)mat.aoColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "ao.color"), shaderAo, 3);
    float shaderEmission[3] = { (float)mat.emissionColor.r/(float)255, (float)mat.emissionColor.g/(float)255, (float)mat.emissionColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "emission.color"), shaderEmission, 3);
    float shaderHeight[3] = { (float)mat.heightColor.r/(float)255, (float)mat.heightColor.g/(float)255, (float)mat.heightColor.b/(float)255 };
    SetShaderValue(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "height.color"), shaderHeight, 3);

    // Send sampler use state to PBR shader
    glUseProgram(mat.env.pbrShader.id);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "albedo.useSampler"), mat.useAlbedoMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "normals.useSampler"), mat.useNormalMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "metalness.useSampler"), mat.useMetalnessMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "roughness.useSampler"), mat.useRoughnessMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "ao.useSampler"), mat.useOcclusionMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "emission.useSampler"), mat.useEmissionMap);
    glUniform1i(GetShaderLocation(mat.env.pbrShader, "height.useSampler"), mat.useParallaxMap);

    // Calculate and send to shader model matrix
    Matrix matScale = MatrixScale(scale.x, scale.y, scale.z);
    Matrix matRotation = MatrixRotate(rotationAxis, rotationAngle*DEG2RAD);
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);
    Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
    SetShaderValueMatrix(mat.env.pbrShader, GetShaderLocation(mat.env.pbrShader, "mMatrix"), transform);

    // Enable and bind irradiance map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mat.env.irradianceId);

    // Enable and bind prefiltered reflection map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mat.env.prefilterId);

    // Enable and bind BRDF LUT map
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mat.env.brdfId);

    if (mat.useAlbedoMap)
    {
        // Disable and bind albedo map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useNormalMap)
    {
        // Disable and bind normals map
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useMetalnessMap)
    {
        // Disable and bind metalness map
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useRoughnessMap)
    {
        // Disable and bind roughness map
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useOcclusionMap)
    {
        // Disable and bind ambient occlusion map
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useParallaxMap)
    {
        // Disable and bind parallax height map
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useAlbedoMap)
    {
        // Enable and bind albedo map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mat.albedoTex.id);
    }

    if (mat.useNormalMap)
    {
        // Enable and bind normals map
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, mat.normalsTex.id);
    }

    if (mat.useMetalnessMap)
    {
        // Enable and bind metalness map
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, mat.metalnessTex.id);
    }

    if (mat.useRoughnessMap)
    {
        // Enable and bind roughness map
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, mat.roughnessTex.id);
    }

    if (mat.useOcclusionMap)
    {
        // Enable and bind ambient occlusion map
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, mat.aoTex.id);
    }

    if (mat.useEmissionMap)
    {
        // Enable and bind emission map
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, mat.emissionTex.id);
    }

    if (mat.useParallaxMap)
    {
        // Enable and bind parallax height map
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, mat.heightTex.id);
    }

    // Draw model using PBR shader and textures maps
    DrawModelEx(model, position, rotationAxis, rotationAngle, scale, WHITE);

    // Disable and unbind irradiance map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Disable and unbind prefiltered reflection map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Disable and unbind BRDF LUT map
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (mat.useAlbedoMap)
    {
        // Disable and bind albedo map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useNormalMap)
    {
        // Disable and bind normals map
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useMetalnessMap)
    {
        // Disable and bind metalness map
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useRoughnessMap)
    {
        // Disable and bind roughness map
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useOcclusionMap)
    {
        // Disable and bind ambient occlusion map
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useEmissionMap)
    {
        // Disable and bind emission map
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (mat.useParallaxMap)
    {
        // Disable and bind parallax height map
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Draw a cube skybox using environment cube map
void DrawSkybox(Environment environment, int mode, Camera camera)
{
    // Calculate view matrix for custom shaders
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);

    // Send to shader view matrix and bind cubemap texture
    glUseProgram(environment.skyShader.id);
    glUniform1i(GetShaderLocation(environment.skyShader, "skyMode"), mode);
    SetShaderValueMatrix(environment.skyShader, environment.skyViewLoc, view);
    glActiveTexture(GL_TEXTURE0);

    switch (mode)
    {
        case BACKGROUND_SKY: glBindTexture(GL_TEXTURE_CUBE_MAP, environment.cubemapId); break;
        case BACKGROUND_BLURSKY: glBindTexture(GL_TEXTURE_CUBE_MAP, environment.cubemapId); break;
        case BACKGROUND_AMBIENT: glBindTexture(GL_TEXTURE_CUBE_MAP, environment.irradianceId); break;
        default: break;
    }

    // Render skybox cube
    RenderCube();
}

// Renders a 1x1 3D cube in NDC
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void RenderCube(void)
{
    // Initialize if it is not yet
    if (cubeVAO == 0)
    {
        GLfloat vertices[] = {
            -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f , 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
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
    // Initialize if it is not yet
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

// Unload material PBR textures
void UnloadMaterialPBR(MaterialPBR mat)
{
    if (mat.useAlbedoMap) UnloadTexture(mat.albedoTex);
    if (mat.useNormalMap) UnloadTexture(mat.normalsTex);
    if (mat.useMetalnessMap) UnloadTexture(mat.metalnessTex);
    if (mat.useRoughnessMap) UnloadTexture(mat.roughnessTex);
    if (mat.useOcclusionMap) UnloadTexture(mat.aoTex);
    if (mat.useEmissionMap) UnloadTexture(mat.emissionTex);
    if (mat.useParallaxMap) UnloadTexture(mat.heightTex);
}

// Unload environment loaded shaders and dynamic textures
void UnloadEnvironment(Environment env)
{
    // Unload used environment shaders
    UnloadShader(env.pbrShader);
    UnloadShader(env.cubeShader);
    UnloadShader(env.skyShader);
    UnloadShader(env.irradianceShader);
    UnloadShader(env.prefilterShader);
    UnloadShader(env.brdfShader);

    // Unload dynamic textures created in environment initialization
    glDeleteTextures(1, &env.cubemapId);
    glDeleteTextures(1, &env.irradianceId);
    glDeleteTextures(1, &env.prefilterId);
    glDeleteTextures(1, &env.brdfId);
}
