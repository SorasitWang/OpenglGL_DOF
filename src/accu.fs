#version 330 core

in VS_OUT {
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D accuTexture;
uniform int n;

out vec4 FragColor;
void main()
{           
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 accuColor = texture( accuTexture, fs_in.TexCoords).rgb;
    FragColor = vec4(accuColor + color,1.0f);
}