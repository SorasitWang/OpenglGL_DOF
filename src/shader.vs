
#version 330 core
layout (location = 0) in vec3 aPos; // the position variable has attribute position 0
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform bool inverse_normals;

out vec2 TexCoords;
out vec3 FragPos;  
out vec3 Normal;

void main()
{

    FragPos = vec3(model * vec4(aPos, 1.0));   
    TexCoords = aTexCoord;
    
    vec3 n = inverse_normals ? -aNormal : aNormal;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * n);
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}