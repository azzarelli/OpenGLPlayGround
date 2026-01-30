#version 330 core

in vec2 UV;
out vec3 color;

// Values that stay constant for the whole mesh
uniform sampler2D myTextureSampler;

void main(){
  color = texture(myTextureSampler, UV).rgb; // Gives RGBA so we just select RGB for now
}