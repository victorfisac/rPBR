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

// Environment parameters
uniform samplerCube irradianceMap;
uniform samplerCube reflectionMap;
uniform samplerCube blurredMap;
uniform sampler2D brdfMap;

// Other parameters
uniform vec3 viewPos;
const float PI = 3.14159265359;

// Output fragment color
out vec4 finalColor;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

float GGX(float NdotV, float a);
float G_Smith(float a, float NdotV, float NdotL);
float radicalInverse_VdC(uint bits);
vec2 Hammersley(uint i, uint n);
vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N);
vec3 PrefilterEnvMap(float roughness, vec3 R);
vec2 IntegrateBRDF(float Roughness, float NoV, vec3 N);
vec3 ApproximateSpecularIBL(vec3 specularColor, float roughness, vec3 N, vec3 V);

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

float GGX(float NdotV, float a)
{
    float k = (a*a)/2;
    return NdotV/(NdotV*(1.0f - k) + k);
}

float G_Smith(float a, float NdotV, float NdotL)
{
    return GGX(NdotL, a)*GGX(NdotV, a);
}

float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint n)
{ 
    return vec2(i/n, radicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
    float a = pow(Roughness + 1, 2);
    float Phi = 2*PI*Xi.x;
    float CosTheta = sqrt((1 - Xi.y)/(1 + (a*a - 1)*Xi.y));
    float SinTheta = sqrt( 1 - CosTheta*CosTheta);

    vec3 H = vec3(0.0);
    H.x = SinTheta*cos(Phi);
    H.y = SinTheta*sin(Phi);
    H.z = CosTheta;

    vec3 UpVector = ((abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0));
    vec3 TangentX = normalize(cross(UpVector, N));
    vec3 TangentY = cross(N, TangentX);

    // Tangent to world space
    return TangentX*H.x + TangentY*H.y + N*H.z;
}

vec3 PrefilterEnvMap(float roughness, vec3 R)
{
    vec3 N = R;
    vec3 V = R;
    vec3 prefColor = vec3(0.0);
    uint numSamples = uint(1024);
    float totalWeight = 0;

    for (uint i = uint(0); i < numSamples; i++)
    {
        vec2 Xi = Hammersley(i, numSamples);
        vec3 H = ImportanceSampleGGX(Xi, roughness, N);
        vec3 L = 2*dot(V, H)*H - V;
        float NoL = clamp(dot(N, L), 0.0, 1.0);

        if (NoL > 0)
        {
            prefColor += texture(reflectionMap, L).rgb*NoL;
            totalWeight += NoL;
        }
    }

    return prefColor/totalWeight;
}

vec2 IntegrateBRDF(float Roughness, float NoV, vec3 N)
{
    vec3 V = vec3(sqrt(1.0f - NoV*NoV), 0.0, NoV);
    float A = 0;
    float B = 0;
    uint numSamples = uint(1024);

    for (uint i = uint(0); i < numSamples; i++)
    {
        vec2 Xi = Hammersley(i, numSamples);
        vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
        vec3 L = 2*dot(V, H)*H - V;
        float NoL = clamp(L.z, 0.0, 1.0);
        float NoH = clamp(H.z, 0.0, 1.0);
        float VoH = clamp(dot(V, H), 0.0, 1.0);

        if(NoL > 0)
        {
            float G = G_Smith(Roughness, NoV, NoL);
            float G_Vis = G*VoH/(NoH*NoV);
            float Fc = pow(1 - VoH, 5);
            A += (1 - Fc)*G_Vis;
            B += Fc*G_Vis;
        }
    }
    return vec2(A, B)/numSamples;
}


vec3 ApproximateSpecularIBL(vec3 specularColor, float roughness, vec3 N, vec3 V)
{
    float NoV = max(dot(N, V), 0.0);
    vec3 R = 2*dot(V, N)*N - V;
    vec3 prefilteredColor = PrefilterEnvMap(roughness, R);
    vec2 envBRDF = IntegrateBRDF(roughness, NoV, N);
    
    return prefilteredColor*(specularColor*envBRDF.x + envBRDF.y);
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
        vec3 F = fresnelSchlickRoughness(max(dot(high, view), 0.0), f0, roughness);

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

    vec3 kS = fresnelSchlickRoughness(max(dot(normal, view), 0.0), f0, roughness);
    vec3 kD = 1.0 - kS;
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 fullReflection = texture(reflectionMap, normal).rgb;
    vec3 blurReflection = texture(blurredMap, normal).rgb;
    vec3 reflection = mix(blurReflection, fullReflection, roughness)*metallic;
    vec3 diffuse = albedo*irradiance;
    vec3 ambient = kD*diffuse + kS*reflection;
    vec3 color = (ambient + Lo)*ao;

    color = color/(color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    finalColor = vec4(color, 1.0);
}
