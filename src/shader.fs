
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
}; 
 
struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  


struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;
    float outerCurOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}; 

struct SimpleLight {
    vec3 Position;
    vec3 Color;
};

uniform SimpleLight lights[16];

in vec3 FragPos;  
in vec3 Normal;
in vec2 TexCoords;

 
# define NR_POINT_LIGHTS 4  
//uniform PointLight pointLights[NR_POINT_LIGHTS];
//uniform Material material;
//uniform Light light; 
//uniform DirLight dirLight;
//uniform vec3 lightPos;  
uniform vec3 viewPos;
uniform sampler2D texture1;
//uniform sampler2D texture2;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);  

void main()
{           

    vec3 color = texture(texture1,TexCoords).rgb;
    vec3 normal = normalize(Normal);
    // ambient
    vec3 ambient = 0 * color;
    // lighting
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < 16; i++)
    {
        // diffuse
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = lights[i].Color * diff * color;      
        vec3 result = diffuse;        
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(FragPos - lights[i].Position);
        result *= 1.0 / (distance*distance);
        lighting += result;
                
    }
    FragColor = 0*vec4(ambient + lighting, 1.0f);
    
    // check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}