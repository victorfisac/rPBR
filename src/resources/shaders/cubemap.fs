#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPos;

// Material parameters
uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591f, 0.3183f);

// Output fragment color
out vec4 finalColor;

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    // Normalize local position 
    vec2 uv = SampleSphericalMap(normalize(fragPos));
    
    vec3 color = texture(equirectangularMap, uv).rgb;
    finalColor = vec4(color, 1.0);
}
