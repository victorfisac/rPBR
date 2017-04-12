#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;

// Constant values
const float samples = 16.0;             // pixels per axis; higher = bigger glow, worse performance
const float quality = 0.35;   	        // lower = smaller glow, better quality

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    const int range = (int(samples) - 1)/2;
    vec2 sizeFactor = vec2(1)/resolution*quality;
    vec4 sum = vec4(0);
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++) sum += texture(texture0, fragTexCoord + vec2(x, y)*sizeFactor);
    }

    vec4 bloomColor = ((sum/(samples*samples)) + source)*colDiffuse;
    float amount = clamp(source.r + source.g + source.b, 0.0, 1.0);

    // Calculate final fragment color
    finalColor = mix(source, bloomColor, amount);
}