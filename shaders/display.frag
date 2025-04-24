#version 460 core

in vec2 texture_coords;
out vec4 FragColor;

uniform sampler2D rendered_texture;

void main() {
    // Apply the tone mapping and gamma correction
    vec3 hdr_color = texture(rendered_texture, texture_coords).rgb;
    
    // Exposure tone mapping
    vec3 mapped = vec3(1.0f) - exp(-hdr_color * 1.0f);
    
    // Gamma correction
    const float gamma = 1.0f;
    mapped = pow(mapped, vec3(1.0 / gamma));
    
    FragColor = vec4(hdr_color, 1.0f);
}