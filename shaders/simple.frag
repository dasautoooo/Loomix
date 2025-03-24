#version 330 core
out vec4 FragColor;
in vec3 vWorldPos;

void main() {
    FragColor = vec4(vWorldPos * 0.2 + 0.5, 1.0);
}
