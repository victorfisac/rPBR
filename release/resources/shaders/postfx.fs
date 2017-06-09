/*******************************************************************************************
*
*   rPBR [shader] - Post-processing effects fragment shader
*
*   Copyright (c) 2017 Victor Fisac
*
**********************************************************************************************/

#version 330

#define     FXAA_REDUCE_MIN     (1.0/128.0)
#define     FXAA_REDUCE_MUL     (1.0/8.0)
#define     FXAA_SPAN_MAX       8.0

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;

// Input uniform values
uniform sampler2D texture0;
uniform vec2 resolution;
uniform int enabledFxaa;
uniform int enabledBloom;
uniform int enabledVignette;

// Constant values
const float samples = 32.0;             // pixels per axis; higher = bigger glow, worse performance
const float quality = 0.25;   	        // lower = smaller glow, better quality
const float radius = 0.75;              // Radius of our vignette, where 0.5 results in a circle fitting the screen
const float softness = 0.9;             // Softness of our vignette, between 0.0 and 1.0
const float oppacity = 0.7;             // Opacity to apply vignette to source image

// Output fragment color
out vec4 finalColor;

void main()
{
    finalColor = texture(texture0, fragTexCoord);

    // FXAA
    //------------------------------------------------------------------------------
    if (enabledFxaa == 1)
    {
        // Calculate inverse of resolution vector
        vec2 inverse_resolution = vec2(1.0/resolution.x,1.0/resolution.y);

        // Calculate antialiasing algorithm
        vec3 rgbNW = texture2D(texture0, fragTexCoord.xy + (vec2(-1.0,-1.0))*inverse_resolution).xyz;
        vec3 rgbNE = texture2D(texture0, fragTexCoord.xy + (vec2(1.0,-1.0))*inverse_resolution).xyz;
        vec3 rgbSW = texture2D(texture0, fragTexCoord.xy + (vec2(-1.0,1.0))*inverse_resolution).xyz;
        vec3 rgbSE = texture2D(texture0, fragTexCoord.xy + (vec2(1.0,1.0))*inverse_resolution).xyz;

        vec3 rgbM  = texture2D(texture0,  fragTexCoord.xy).xyz;
        vec3 luma = vec3(0.299, 0.587, 0.114);

        float lumaNW = dot(rgbNW, luma);
        float lumaNE = dot(rgbNE, luma);
        float lumaSW = dot(rgbSW, luma);
        float lumaSE = dot(rgbSE, luma);
        float lumaM  = dot(rgbM,  luma);
        float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
        float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE))); 

        vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)), ((lumaNW + lumaSW) - (lumaNE + lumaSE)));
        float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE)*(0.25*FXAA_REDUCE_MUL),FXAA_REDUCE_MIN);
        float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
        dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),dir*rcpDirMin))*inverse_resolution;
        vec3 rgbA = 0.5*(texture2D(texture0, fragTexCoord + dir*(1.0/3.0 - 0.5)).xyz + texture2D(texture0, fragTexCoord + dir*(2.0/3.0 - 0.5)).xyz);
        vec3 rgbB = rgbA*0.5 + 0.25*(texture2D(texture0, fragTexCoord + dir*-0.5).xyz + texture2D(texture0, fragTexCoord + dir*0.5).xyz);
        float lumaB = dot(rgbB, luma);

        // Calculate final fragment color
        finalColor = vec4((((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB), 1.0);
    }

    // Bloom
    //------------------------------------------------------------------------------
    if (enabledBloom == 1)
    {
        const int range = (int(samples) - 1)/2;
        vec2 sizeFactor = vec2(1)/resolution*quality;
        vec4 sum = vec4(0);
        for (int x = -range; x <= range; x++)
        {
            for (int y = -range; y <= range; y++) sum += texture(texture0, fragTexCoord + vec2(x, y)*sizeFactor);
        }

        vec4 bloomColor = ((sum/(samples*samples)) + finalColor);
        float amount = clamp(finalColor.r + finalColor.g + finalColor.b, 0.0, 1.0);

        // Calculate final fragment color
        finalColor = mix(finalColor, bloomColor, amount);
    }

    // Vignette
    //------------------------------------------------------------------------------
    if (enabledVignette == 1)
    {
        // Determine center
        vec2 position = fragTexCoord.xy - vec2(0.5);

        // Determine the vector length from center
        float len = length(position);

        // Our vignette effect, using smoothstep
        float vignette = smoothstep(radius, radius - softness, len);

        // Apply our vignette
        vec3 final = finalColor.rgb*vignette;

        // Calculate final fragment color
        finalColor = vec4(mix(finalColor.rgb, final, oppacity), 1.0);
    }
}
