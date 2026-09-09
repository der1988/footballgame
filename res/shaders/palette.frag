#version 330

const int MAX_INDEXED_COLORS = 12;

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
    int index = int(round(texelColor.r * 255.0));

    // A red channel past the last index is art that carries its own colours - the ball in the
    // keeper's hands, say - and is drawn as it was authored.
    if (index >= MAX_INDEXED_COLORS) {
        finalColor = texelColor * fragColor;
        return;
    }

    ivec4 color = palette[index];

    // Explicitly cast to vec4 before dividing by a float, and multiply by vertex color
    finalColor = vec4(color) / 255.0 * fragColor;
}
