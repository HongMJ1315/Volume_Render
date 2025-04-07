#version 400 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform vec3 minDrawPos;

void main() {

    if(!(!(minDrawPos.x <= FragPos.x ) &&
         minDrawPos.y <= FragPos.y  &&
         minDrawPos.z <= FragPos.z )) {
        discard;
    }
    // 環境光
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射光照
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 鏡面反射光照
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 0.3);
}
