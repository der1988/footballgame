#version 300 es
precision highp float;
precision highp int;

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;
// Position in the world, before the camera is applied
out vec2 fragPosition;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragPosition = vertexPosition.xy;

    // Calculate final vertex position
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
