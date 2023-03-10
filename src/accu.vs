#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec2 TexCoords;
} vs_out;

layout (std140) uniform Matrices
{
    mat4 vp;

};
uniform mat4 model;
void main()
{

    vs_out.TexCoords = aTexCoords;
    gl_Position = vp * model * vec4(aPos, 1.0);
}