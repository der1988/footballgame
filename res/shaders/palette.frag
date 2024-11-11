#version 330

const int MAX_INDEXED_COLORS = 255;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec3 palette[MAX_INDEXED_COLORS];

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    int index = int(texelColor.r*255.0);
    vec3 color = palette[index];

    finalColor = vec4(color/255.0, texelColor.a);
}

