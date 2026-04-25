#version 330

const int MAX_INDEXED_COLORS = 10;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform ivec4 palette[MAX_INDEXED_COLORS];

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    if (texelColor.a < 0.01) {
        discard;
    }

    // Use round() to avoid floating point truncation issues
    // Use clamp() to prevent out-of-bounds array access
    int index = clamp(int(round(texelColor.r * 255.0)), 0, MAX_INDEXED_COLORS - 1);
    ivec4 color = palette[index];

    // Explicitly cast to vec4 before dividing by a float, and multiply by vertex color
    finalColor = vec4(color) / 255.0 * fragColor;
}
