/*******************************************************************************************
*
*   rPBR [shader] - Background skybox fragment shader
*
*   Copyright (c) 2017 Victor Fisac
*
**********************************************************************************************/

#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPos;

// Input uniform values
uniform samplerCube environmentMap;
uniform int skyMode;
uniform vec2 resolution;

// Output fragment color
out vec4 finalColor;

float scurve(float x);
vec3 blur(samplerCube cubemap, vec3 pos, float radius);

float scurve(float x)
{
    float result = x*2.0 - 1.0;
    result = -result*abs(result)*0.5 + x + 0.5;
    return result;
}

vec3 blur(samplerCube cubemap, vec3 pos, float radius)
{
    vec3 blurred = vec3(0.0);

    if (radius >= 1.0)
    {
        float width = 1.0/resolution.x;
        float height = 1.0/resolution.y;
        float iRadius = 1.0/radius;

        vec4 axis = vec4(0.0);
        float divisor = 0.0;
        float weight = 0.0;
        vec4 color = vec4(0.0);

        // X axis pass
        for (float x = -radius; x <= radius; x++)
        {
            axis = texture(cubemap, pos + vec3(x*width, 0.0, 0.0));
            weight = scurve(1.0 - (abs(x)*iRadius)); 
            color += axis*weight;
            divisor += weight;
        }

        // Y axis pass
        for (float y = -radius; y <= radius; y++)
        {
            axis = texture(cubemap, pos + vec3(0.0, y*height, 0.0));
            weight = scurve(1.0 - (abs(y)*iRadius));
            color += axis*weight;
            divisor += weight;
        }

        // Z axis pass
        for (float z = -radius; z <= radius; z++)
        {
            axis = texture(cubemap, pos + vec3(0.0, 0.0, z*height));
            weight = scurve(1.0 - (abs(z)*iRadius)); 
            color += axis*weight;
            divisor += weight;
        }

        blurred = vec3(color)/divisor;
    }
    else blurred = texture(cubemap, pos).rgb;

    return blurred;
}

void main()
{
    // Fetch color from texture map
    vec3 color = vec3(0.0);
    if (skyMode == 1) color = blur(environmentMap, fragPos, 15.0);
    else color = texture(environmentMap, fragPos).rgb;

    // Apply gamma correction
    color = color/(color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    // Calculate final fragment color
    finalColor = vec4(color, 1.0);
}
