#version 330 core

// Input from the vertex buffer
in vec3 vertexPosition;
in vec3 vertexNormal;

// Uniforms from your application
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Outline-specific uniforms
uniform float outlineThickness;  // The thickness of the outline

// Output to the fragment shader (optional)
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragPosition;
out vec4 fragColor;

void main()
{
    // Calculate world-space position of vertex
    vec4 worldPosition = model * vec4(vertexPosition, 1.0);
    
    // Scale the position for the outline effect
    // The outline is created by expanding the vertices along their normal direction
    vec4 outlinePosition = worldPosition + vec4(normalize(vertexNormal) * outlineThickness, 0.0);

    // Send position and normal to the fragment shader
    fragPosition = outlinePosition.xyz;
    fragNormal = mat3(transpose(inverse(model))) * vertexNormal;

    // Final screen-space position (for outline, use the expanded position)
    gl_Position = projection * view * outlinePosition;
}
