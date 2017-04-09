#version 330
#define     MAX_LIGHTS      4

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
in mat4 fragModelMatrix;

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
uniform samplerCube reflectionMap;
uniform samplerCube blurredMap;

// Other parameters
uniform int renderMode;
uniform vec3 viewPos;
const float PI = 3.14159265359;

// Output fragment color
out vec4 finalColor;

vec3 blend(vec3 bg, vec3 fg);
vec3 ComputeMaterialProperty(MaterialProperty property);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

vec3 blend(vec3 bg, vec3 fg)
{
    vec3 result = vec3(0.0);
    
    result.x = ((bg.x < 0.5) ? (2.0*bg.x*fg.x) : (1.0 - 2.0*(1.0 - bg.x)*(1.0 - fg.x)));
    result.y = ((bg.y < 0.5) ? (2.0*bg.y*fg.y) : (1.0 - 2.0*(1.0 - bg.y)*(1.0 - fg.y)));
    result.z = ((bg.z < 0.5) ? (2.0*bg.z*fg.z) : (1.0 - 2.0*(1.0 - bg.z)*(1.0 - fg.z)));
    
    return result;
}

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
    // Calculate TBN and RM matrices
    mat3 TBN = transpose(mat3(fragTangent, fragBinormal, fragNormal));
    mat3 RM = mat3(fragModelMatrix[0].xyz, fragModelMatrix[1].xyz, fragModelMatrix[2].xyz);

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
        view = normalize(TBN*normalize(viewPos - fragPos));

        // Convert tangent space normal to world space due to cubemap reflection calculations
        refl = normalize(reflect(normalize(fragPos - viewPos), normalize(RM*(normal*TBN))));
    }

    // Fetch material values from texture sampler or color attributes
    vec3 color = ComputeMaterialProperty(albedo);
    vec3 metal = ComputeMaterialProperty(metallic);
    vec3 rough = ComputeMaterialProperty(roughness);
    vec3 occlusion = ComputeMaterialProperty(ao);

    // Calculate diffuse color from metalness value
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, color, metal.r);

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

        // Cook-torrance brdf
        float NDF = DistributionGGX(normal, high, rough.r);
        float G = GeometrySmith(normal, view, light, roughness.color.r);
        vec3 F = fresnelSchlickRoughness(max(dot(high, view), 0.0), f0, rough.r);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metal.r;

        vec3 nominator = NDF*G*F;
        float denominator = 4*max(dot(normal, view), 0.0)*max(dot(normal, light), 0.0) + 0.001;
        vec3 brdf = nominator/denominator;

        // Add to outgoing radiance Lo
        float NdotL = max(dot(normal, light), 0.0);
        Lo += (kD*color/PI + brdf)*radiance*NdotL;
        lightDot += radiance*NdotL + brdf;
    }

    // Calculate specular fresnel and energy conservation
    vec3 kS = fresnelSchlickRoughness(max(dot(normal, view), 0.0), f0, rough.r);
    vec3 kD = 1.0 - kS;

    // Calculate indirect diffuse
    vec3 irradiance = texture(irradianceMap, fragNormal).rgb;

    // Calculate indirect specular and reflection
    vec3 fullReflection = texture(reflectionMap, refl).rgb;
    vec3 blurReflection = texture(blurredMap, refl).rgb;
    vec3 reflection = mix(blurReflection, fullReflection, rough.r)*metal.r;

    // Calculate final lighting
    vec3 diffuse = color*irradiance;
    vec3 ambient = kD*diffuse + kS*reflection;

    // Calculate final fragment color
    vec3 fragmentColor = vec3(0.0);
    if (renderMode == 0) fragmentColor = (ambient + Lo)*occlusion;          // Default
    else if (renderMode == 1) fragmentColor = color;                        // Albedo
    else if (renderMode == 2) fragmentColor = normalize(RM*(normal*TBN));   // Normals
    else if (renderMode == 3) fragmentColor = metal;                        // Metallic
    else if (renderMode == 4) fragmentColor = rough;                        // Roughness
    else if (renderMode == 5) fragmentColor = occlusion;                    // Ambient Occlusion
    else if (renderMode == 6) fragmentColor = lightDot;                     // Lighting
    else if (renderMode == 7) fragmentColor = kS;                           // Fresnel
    else if (renderMode == 8) fragmentColor = irradiance;                   // Irradiance
    else if (renderMode == 9) fragmentColor = reflection;                   // Reflection

    // Apply gamma correction
    fragmentColor = fragmentColor/(fragmentColor + vec3(1.0));
    fragmentColor = pow(fragmentColor, vec3(1.0/2.2));

    // Calculate final fragment color
    finalColor = vec4(fragmentColor, 1.0);
}
