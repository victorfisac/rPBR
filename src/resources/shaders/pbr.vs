#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Input uniform values
uniform mat4 mvpMatrix;
uniform mat4 mMatrix;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec3 fragPos;
out vec3 fragNormal;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;

    // Calculate fragment position based on model transformations
    fragPos = vec3(mMatrix*vec4(vertexPosition, 1.0f));

    // Calculate fragment normal based on model transformations
    mat3 normalMatrix = transpose(inverse(mat3(mMatrix)));
    fragNormal = normalize(normalMatrix*vertexNormal);

    // Calculate final vertex position
    gl_Position = mvpMatrix*vec4(vertexPosition, 1.0);
}