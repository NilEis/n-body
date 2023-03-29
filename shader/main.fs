#version 330 core

out vec4 FragColor;

void main() {
    FragColor = vec4((gl_FragCoord.xyz + vec3(10.0)) / 20.0, 1.0);
}