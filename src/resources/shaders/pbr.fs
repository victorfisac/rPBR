#version 330
#define     MAX_LIGHTS              4
#define     MAX_REFLECTION_LOD      4.0

struct MaterialProperty {
    vec3 color;
    int useSampler;
    sampler2D sampler;
};

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBinormal;

// Material parameters
uniform MaterialProperty albedo;
uniform MaterialProperty normals;
uniform MaterialProperty metallic;
uniform MaterialProperty roughness;
uniform MaterialProperty ao;

// Lighting parameters
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];

// Environment parameters
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// Other parameters
uniform int renderMode;
uniform vec3 viewPos;
const float PI = 3.14159265359;

// Output fragment color
out vec4 finalColor;

vec3 ComputeMaterialProperty(MaterialProperty property);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

vec3 ComputeMaterialProperty(MaterialProperty property)
{
    if (property.useSampler == 1) return texture(property.sampler, fragTexCoord).rgb;
    else return property.color;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2*(a2 - 1.0) + 1.0);
    denom = PI*denom*denom;

    return nom/denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = r*r/8.0;

    float nom = NdotV;
    float denom = NdotV*(1.0 - k) + k;

    return nom/denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1*ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0)*pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)*pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Fetch material values from texture sampler or color attributes
    vec3 color = pow(ComputeMaterialProperty(albedo), vec3(2.2));
    vec3 metal = ComputeMaterialProperty(metallic);
    vec3 rough = ComputeMaterialProperty(roughness);
    vec3 occlusion = ComputeMaterialProperty(ao);

    // Calculate TBN and RM matrices
    mat3 TBN = transpose(mat3(fragTangent, fragBinormal, fragNormal));

    // Calculate lighting required attributes
    vec3 normal = normalize(fragNormal);
    vec3 view = normalize(viewPos - fragPos);
    vec3 refl = reflect(-view, normal);

    // Check if normal mapping is enabled
    if (normals.useSampler == 1)
    {
        // Fetch normal map color and transform lighting values to tangent space
        normal = ComputeMaterialProperty(normals);
        normal = normalize(normal*2.0 - 1.0);
        normal = normalize(normal*TBN);
        view = normalize(TBN*normalize(viewPos - fragPos));

        // Convert tangent space normal to world space due to cubemap reflection calculations
        refl = normalize(reflect(normalize(fragPos - viewPos), normal));
    }

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, color, metal.r);

    // Calculate lighting for all lights
    vec3 Lo = vec3(0.0);
    vec3 lightDot = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        // Calculate per-light radiance
        vec3 light = normalize(lightPos[i] - fragPos);
        if (normals.useSampler == 1) light = normalize(TBN*normalize(lightPos[i] - fragPos));
        vec3 high = normalize(view + light);
        float distance = length(lightPos[i] - fragPos);
        if (normals.useSampler == 1) distance = length(normalize(TBN*normalize(lightPos[i] - fragPos)));
        float attenuation = 1.0/(distance*distance);
        vec3 radiance = lightColor[i]*attenuation;

        // Cook-torrance BRDF
        float NDF = DistributionGGX(normal, high, rough.r);
        float G = GeometrySmith(normal, view, light, rough.r);
        vec3 F = fresnelSchlick(max(dot(high, view), 0.0), F0);
        vec3 nominator = NDF*G*F;
        float denominator = 4*max(dot(normal, view), 0.0)*max(dot(normal, light), 0.0) + 0.001;
        vec3 brdf = nominator/denominator;

        // Store to kS the fresnel value and calculate energy conservation
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;

        // Multiply kD by the inverse metalness such that only non-metals have diffuse lighting
        kD *= 1.0 - metal.r;

        // Scale light by dot product between normal and light direction
        float NdotL = max(dot(normal, light), 0.0);

        // Add to outgoing radiance Lo
        // Note: BRDF is already multiplied by the Fresnel so it doesn't need to be multiplied again
        Lo += (kD*color/PI + brdf)*radiance*NdotL;
        lightDot += radiance*NdotL + brdf;
    }

    // Calculate ambient lighting using IBL
    vec3 F = fresnelSchlickRoughness(max(dot(normal, view), 0.0), F0, rough.r);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metal.r;

    // Calculate indirect diffuse
    vec3 irradiance = texture(irradianceMap, fragNormal).rgb;
    vec3 diffuse = color*irradiance;

    // Sample both the prefilter map and the BRDF lut and combine them together as per the Split-Sum approximation
    vec3 prefilterColor = textureLod(prefilterMap, refl, rough.r*MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(normal, view), 0.0), rough.r)).rg;
    vec3 reflection = prefilterColor*(F*brdf.x + brdf.y);

    // Calculate final lighting
    vec3 ambient = (kD*diffuse + reflection)*occlusion;

    // Calculate fragment color based on render mode
    vec3 fragmentColor = ambient + Lo;                                      // Physically Based Rendering
    if (renderMode == 1) fragmentColor = color;                             // Albedo
    else if (renderMode == 2) fragmentColor = normal;                       // Normals
    else if (renderMode == 3) fragmentColor = metal;                        // Metallic
    else if (renderMode == 4) fragmentColor = rough;                        // Roughness
    else if (renderMode == 5) fragmentColor = occlusion;                    // Ambient Occlusion
    else if (renderMode == 6) fragmentColor = lightDot;                     // Lighting
    else if (renderMode == 7) fragmentColor = kS;                           // Fresnel
    else if (renderMode == 8) fragmentColor = irradiance;                   // Irradiance
    else if (renderMode == 9) fragmentColor = reflection;                   // Reflection

    // Apply HDR tonemapping
    fragmentColor = fragmentColor/(fragmentColor + vec3(1.0));

    // Apply gamma correction
    fragmentColor = pow(fragmentColor, vec3(1.0/2.2));

    // Calculate final fragment color
    finalColor = vec4(fragmentColor, 1.0);
}
