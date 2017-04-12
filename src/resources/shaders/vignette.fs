#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Constant values
const float radius = 0.75;         // Radius of our vignette, where 0.5 results in a circle fitting the screen
const float softness = 0.9;        // Softness of our vignette, between 0.0 and 1.0
const float oppacity = 0.5;        // Opacity to apply vignette to source image

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    vec3 source = texture(texture0, fragTexCoord).rgb;

    // Determine center
    vec2 position = fragTexCoord.xy - vec2(0.5);

    // Determine the vector length from center
    float len = length(position);

    // Our vignette effect, using smoothstep
    float vignette = smoothstep(radius, radius - softness, len);

    // Apply our vignette
    vec3 final = source.rgb*vignette;

    // Calculate final fragment color
    finalColor = vec4(mix(source, final, oppacity), 1.0);
}