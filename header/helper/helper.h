#ifndef HELPER_H
#define HELPER_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>



#include <string>
#include <vector>
#include "../shader_m.h"
using namespace std;
class Helper {

public:
	static unsigned int textureColorbuffer, rbo, framebuffer, depthFBO, depthMap;
	static float screenW, screenH;
	static float* geoData;
	Shader geoShader, screenShader,depthShader;
	Helper();
	Helper(float SCR_WIDTH, float SCR_HEIGHT);
	glm::vec3 calWordPos(glm::vec2 mousePos);
	void addObject(glm::vec3 min, glm::vec3 max);
};

#endif