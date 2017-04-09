#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPos;

// Environment parameters
uniform sampler2D equirectangularMap;

// Other parameters
const vec2 invAtan = vec2(0.1591, 0.3183);

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

    // Fetch color from texture map
    vec3 color = texture(equirectangularMap, uv).rgb;

    // Calculate final fragment color
    finalColor = vec4(color, 1.0);
}
