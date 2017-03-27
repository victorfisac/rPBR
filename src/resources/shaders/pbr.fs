#version 330
#define     MAX_LIGHTS      4

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec3 fragPos;
in vec3 fragNormal;

// Material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// Lighting parameters
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];

// Other parameters
uniform vec3 viewPos;
const float PI = 3.14159265359;

// Output fragment color
out vec4 finalColor;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

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

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 view = normalize(viewPos - fragPos);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

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
        float NDF = DistributionGGX(normal, high, roughness);
        float G = GeometrySmith(normal, view, light, roughness);
        vec3 F = fresnelSchlick(max(dot(high, view), 0.0), f0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 nominator = NDF*G*F;
        float denominator = 4*max(dot(normal, view), 0.0)*max(dot(normal, light), 0.0) + 0.001;
        vec3 brdf = nominator/denominator;

        // Add to outgoing radiance Lo
        float NdotL = max(dot(normal, light), 0.0);
        Lo += (kD*albedo/PI + brdf)*radiance*NdotL;
    }

    vec3 ambient = vec3(0.03)*albedo*ao;
    vec3 color = ambient + Lo;

    color = color/(color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    finalColor = vec4(color, 1.0);
}
