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
    // Calculate lighting required attributes
    vec3 normal = normalize(fragNormal);
    // vec3 normal = ComputeMaterialProperty(normals);
    // normal = normalize(normal*2.0 - 1.0);
    vec3 view = normalize(viewPos - fragPos);
    vec3 refl = reflect(-view, normal);
    
    // Fetch material values from texture sampler or color attributes
    vec3 color = ComputeMaterialProperty(albedo);
    vec3 metal = ComputeMaterialProperty(metallic);
    vec3 rough = ComputeMaterialProperty(roughness);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, color, metal.r);

    // Reflectance equation
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        // Calculate per-light radiance
        vec3 light = normalize(lightPos[i] - fragPos);
        vec3 high = normalize(view + light);
        float distance = length(lightPos[i] - fragPos);
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
    }

    // Calculate specular fresnel and energy conservation
    vec3 kS = fresnelSchlickRoughness(max(dot(normal, view), 0.0), f0, rough.r);
    vec3 kD = 1.0 - kS;

    // Calculate indirect diffuse
    vec3 irradiance = texture(irradianceMap, normal).rgb;

    // Calculate indirect specular and reflection
    vec3 fullReflection = texture(reflectionMap, refl).rgb;
    vec3 blurReflection = texture(blurredMap, refl).rgb;
    vec3 reflection = mix(blurReflection, fullReflection, rough.r)*metal.r;

    // Calculate final lighting
    vec3 diffuse = color*irradiance;
    vec3 ambient = kD*diffuse + kS*reflection;

    // Calculate final fragment color
    vec3 fragmentColor = (ambient + Lo)*ComputeMaterialProperty(ao);

    // Apply gamma correction
    fragmentColor = fragmentColor/(fragmentColor + vec3(1.0));
    fragmentColor = pow(fragmentColor, vec3(1.0/2.2));

    finalColor = vec4(fragmentColor, 1.0);
}
