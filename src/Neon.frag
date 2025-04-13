#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 neonColor;   // Neon glow color
uniform float glowIntensity;  // How strong the glow is
uniform float quality;  // Blur quality (lower = sharper, higher = smoother)

// Output fragment color
out vec4 finalColor;

// Constants
const vec2 size = vec2(1200, 800);  // Framebuffer size
const int BLUR_RADIUS = 3;          // Constant blur radius
const int NUM_PASSES = 3;           // How many times we blur the glow
const float blurSpread = 1.5;       // Multiplier for each blur step
const float edgeThreshold = 0.4;    // How much brightness is needed for a glow

void main()
{
    vec4 sum = vec4(0.0);
    vec2 texelSize = vec2(1.0) / size * quality;
    
    // Get the base color
    vec4 source = texture(texture0, fragTexCoord);

    // Edge detection for neon glow (keeps only bright parts)
    float brightness = dot(source.rgb, vec3(0.299, 0.587, 0.114)); // Luminance
    vec4 glow = brightness > edgeThreshold ? source : vec4(0.0);  

    // Blur pass to spread the glow outward
    for (int pass = 0; pass < NUM_PASSES; pass++)
    {
        float radius = BLUR_RADIUS * (1.0 + pass * blurSpread);

        for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
        {
            sum += texture(texture0, fragTexCoord + vec2(i, 0) * texelSize * radius) * 0.1;
            sum += texture(texture0, fragTexCoord + vec2(0, i) * texelSize * radius) * 0.1;
        }
    }

    // Apply neon color and intensity
    vec4 neonGlow = sum * neonColor * glowIntensity;

    // Final composition: keep source color, add the glow
    finalColor = source + neonGlow * colDiffuse;
}
