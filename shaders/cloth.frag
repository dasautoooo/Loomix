#version 330 core
in vec3 FragNormal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 uClothColor;
uniform vec3 uLightPos;

void main()
{
    // Simple diffuse shading
    vec3 normal = normalize(FragNormal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 color = uClothColor * diff;  // basic lambert shading
    FragColor = vec4(color, 1.0);
}