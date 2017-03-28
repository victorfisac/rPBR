#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPos;

// Material parameters
uniform samplerCube environmentMap;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec3 color = texture(environmentMap, fragPos).rgb;

    color = color/(color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    finalColor = vec4(color, 1.0);
}
