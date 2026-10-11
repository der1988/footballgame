#version 330

// PALETTE SHADER
//
// Fragment shader of the players. For every pixel of a player sprite it picks the colour from
// the palette of the player, then lets a ball hidden behind him show through.
//
// 1. THE PALETTE
//
// All the players share the same frames, whatever kit they wear. The frames hold palette indices
// instead of colours: the red channel of a texel is a number from 0 to 11.
//
//    0 hair          3 kit light     6 shoes         9 kit dark
//    1 socks light   4 socks dark    7 shadow       10 card
//    2 skin dark     5 shorts        8 skin light   11 gloves
//
// Each player sets the palette uniform to his own 12 colours before he is drawn.
//
//   - A transparent texel is not part of the player: the pixel is discarded.
//   - The red channel is rounded back to the index it was saved as.
//   - An index of 12 or more is a texel that carries its own colours, like the ball in the hands
//     of the keeper. It is drawn as it is in the spritesheet.
//   - Otherwise the colour is the palette entry at that index.
//   - Either way it is multiplied by fragColor, the tint of the sprite.
//
// 2. THE HIDDEN BALL
//
// Sprites are drawn back to front, so a player further down the screen hides the ball. The
// shader cannot read the screen to find it, so it works the ball out again from the spritesheet,
// which holds the ball as well as the players.
//
//   - hiddenBallDst is the rectangle of the world the ball frame was drawn to, and
//     hiddenBallSrcOrigin the top left texel of that frame.
//   - fragPosition, the world position of this pixel, less the top left corner of hiddenBallDst
//     is how far into the ball frame this pixel is.
//   - The ball is drawn one texel per pixel, not rotated and not flipped, so the texel at
//     hiddenBallSrcOrigin plus that offset is the pixel of the ball drawn here. Outside the
//     frame there is no ball.
//   - If that texel is not transparent, this pixel covers the ball: it is mixed with the colour
//     of the ball by hiddenBallAlpha.
//   - If a texel next to it is, this pixel touches the ball from outside: it is mixed with
//     hiddenBallOutlineColor.
//
// The part of the ball the player does not cover is still on the screen from its own draw.
//
// The shader does not know who is in front of the ball. A player drawn before the ball sets an
// empty hiddenBallDst, so every pixel is outside the frame; one drawn after it sets the real
// rectangle. Uniforms keep their value between draws, so the empty one has to be set.

const int MAX_INDEXED_COLORS = 12;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec2 fragPosition;

// Input uniform values
uniform sampler2D texture0;
uniform ivec4 palette[MAX_INDEXED_COLORS];

// The hidden ball, see the top of the file. texture0 holds the ball too.
// Where the ball frame was drawn: top left corner (xy) and size (zw), in world pixels. No ball
// when the size is 0.
uniform vec4 hiddenBallDst;
// Top left texel of the ball frame in texture0
uniform vec2 hiddenBallSrcOrigin;
// Share of the ball in a pixel that covers it, 0 to 1
uniform float hiddenBallAlpha;
// Its alpha is the share of the outline in a pixel next to the ball
uniform vec4 hiddenBallOutlineColor;

// Output fragment color
out vec4 finalColor;

// The texel of the ball frame this far from its top left corner, transparent outside the frame
vec4 hiddenBallTexel(vec2 offset)
{
    if (any(lessThan(offset, vec2(0.0))) || any(greaterThanEqual(offset, hiddenBallDst.zw))) {
        return vec4(0.0);
    }
    return texelFetch(texture0, ivec2(hiddenBallSrcOrigin + offset), 0);
}

vec4 withHiddenBall(vec4 color)
{
    vec2 offset = floor(fragPosition) - hiddenBallDst.xy;
    vec4 ball = hiddenBallTexel(offset);
    if (ball.a > 0.0) {
        color.rgb = mix(color.rgb, ball.rgb, hiddenBallAlpha);
        return color;
    }

    // The outline is every pixel next to the ball that is not the ball
    float ballNextTo = hiddenBallTexel(offset + vec2(-1.0, 0.0)).a +
                       hiddenBallTexel(offset + vec2(1.0, 0.0)).a +
                       hiddenBallTexel(offset + vec2(0.0, -1.0)).a +
                       hiddenBallTexel(offset + vec2(0.0, 1.0)).a;
    if (ballNextTo > 0.0) {
        color.rgb = mix(color.rgb, hiddenBallOutlineColor.rgb, hiddenBallOutlineColor.a);
    }
    return color;
}

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
        finalColor = withHiddenBall(texelColor * fragColor);
        return;
    }

    ivec4 color = palette[index];

    // Explicitly cast to vec4 before dividing by a float, and multiply by vertex color
    finalColor = withHiddenBall(vec4(color) / 255.0 * fragColor);
}
