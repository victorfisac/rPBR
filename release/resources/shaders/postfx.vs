/*******************************************************************************************
*
*   rPBR [shader] - Post-processing effects vertex shader
*
*   Copyright (c) 2017 Victor Fisac
*
**********************************************************************************************/

#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Input uniform values
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;

    // Calculate final vertex position
    gl_Position = mvpMatrix*vec4(vertexPosition, 1.0);
}
