#include "./helper.h"
#include "../shader_m.h"

unsigned int Helper::textureColorbuffer, Helper::rbo, Helper::framebuffer,Helper::depthFBO,Helper::depthMap;
float Helper::screenH, Helper::screenW;
float* Helper::geoData;
Helper::Helper() {

}
Helper::Helper(float SCR_WIDTH, float SCR_HEIGHT) {
	// create a color attachment texture
	this->screenW = SCR_WIDTH;
	this->screenH = SCR_HEIGHT;
	//this->geoShader.setSource("./header/helper/geoShader.vs", "./header/helper/geoShader.fs");
	this->screenShader.setSource("./header/helper/screenShader.vs", "./header/helper/screenShader.fs");
	this->depthShader.setSource("./header/helper/depthShader.vs", "./header/helper/depthShader.fs");
	this->geoData = new float[SCR_WIDTH * SCR_HEIGHT];
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glGenFramebuffers(1, &depthFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		this->screenW, this->screenH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

glm::vec3 Helper::calWordPos(glm::vec2 mousePos) {
	// convert origin
	mousePos.y = screenH - mousePos.y;
	cout << mousePos.x << " " << mousePos.y << endl;
	// find value from texturBuffer 


	int start = (mousePos.x + mousePos.y * this->screenW) * 3;

	cout << start << endl;

	return glm::vec3(this->geoData[start], this->geoData[start + 1], this->geoData[start + 2]);

}