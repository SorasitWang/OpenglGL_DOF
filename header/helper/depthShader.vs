
#version 330 core
layout (location = 0) in vec3 aPos; // the position variable has attribute position 0
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
layout (std140) uniform Matrices
{
    //mat4 projection;
    //mat4 view;
    mat4 vp;
};

out vec4 pos;

void main()
{
    //pos = projection * view * model * vec4(aPos, 1.0);
    pos =vp * model * vec4(aPos, 1.0);
    gl_Position = pos;

}