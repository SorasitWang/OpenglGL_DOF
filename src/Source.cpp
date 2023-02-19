#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "../header/stb_image.h"
#endif
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>


#include "../header/shader_m.h"
#include "../header/camera.h"
#include "../header/model.h"
#include "../header/helper/helper.h"
#include <cmath>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
unsigned int loadTexture(const char* path, bool gammaCorrection);
void renderQuad();
void renderCube();
void uniformDist(const float r, int x,int y,vector<float> prevValue, vector<float>& fb2,vector<float>& fb3);
void draw(Shader shader, Shader shaderLight, unsigned int woodTexture, unsigned int containerTexture
    , vector<glm::vec3> lightPositions, vector<glm::vec3> lightColors);
float normalizeDepth(float depth);
std::vector<float> filterCreation(const int size,const float sigma);
# define M_PI           3.14159265358979323846  
// settings
const unsigned int SCR_WIDTH = 300;
const unsigned int SCR_HEIGHT = 300;
const glm::vec2 SCR_CENTER(SCR_WIDTH/2, SCR_HEIGHT/2);
const float zNear = 0.1f, zFar = 15.0f;
const float zMul = zNear * 1, zDiff = 1 - zNear;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// camera
Camera camera(glm::vec3(.0f, 1.5f, 4.5f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const float aperture = 4.0f;
float focusLength = 6.0;
float focus = 1.0f;
float invFocusLength = 1 / focusLength;
float screenPos = 4.0f;
vector<float> allR;
Helper helper;
using namespace std;
int main()
{
    camera.ProcessMouseMovement(10.0f,1.0f);
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader("./src/7.bloom.vs", "./src/7.bloom.fs");
    Shader shaderLight("./src/7.bloom.vs", "./src/7.light_box.fs");
    Shader shaderBlur("./src/7.blur.vs", "./src/7.blur.fs");
    Shader shaderBloomFinal("./src/7.bloom_final.vs", "./src/7.bloom_final.fs");
    helper = Helper(SCR_WIDTH, SCR_HEIGHT);

    unsigned int uniformBlockIndexDepth = glGetUniformBlockIndex(helper.depthShader.ID, "Matrices");
    glUniformBlockBinding(helper.geoShader.ID, uniformBlockIndexDepth, 0);
    unsigned int uniformBlockIndexShader = glGetUniformBlockIndex(shader.ID, "Matrices");
    glUniformBlockBinding(shader.ID, uniformBlockIndexShader, 0);
    unsigned int uniformBlockIndexLight = glGetUniformBlockIndex(shaderLight.ID, "Matrices");
    glUniformBlockBinding(shaderLight.ID, uniformBlockIndexLight, 0);

    unsigned int uboMatrices;
    glGenBuffers(1, &uboMatrices);

    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    // Mat4
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, sizeof(glm::mat4));
    // load textures
    // -------------
    unsigned int woodTexture = loadTexture("./res/texture/wood.png", true); // note that we're loading the texture as an SRGB texture
    unsigned int containerTexture = loadTexture("./res/texture/wood.png", true); // note that we're loading the texture as an SRGB texture

    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }


    //unsigned int dofFBO;
    //glGenFramebuffers(1, &dofFBO);
    //glBindFramebuffer(GL_FRAMEBUFFER, dofFBO);
    //unsigned int dofBuffer;
    //glGenTextures(1, &dofBuffer);
   
    //glBindTexture(GL_TEXTURE_2D, dofBuffer);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //// attach texture to framebuffer
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, dofBuffer, 0);
    //
    //unsigned int dofDepth;
    //glGenRenderbuffers(1, &dofDepth);
    //glBindRenderbuffer(GL_RENDERBUFFER, dofDepth);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dofDepth);


    // lighting info
    // -------------
    // positions
    std::vector<glm::vec3> lightPositions;
    lightPositions.push_back(glm::vec3(0.0f, 0.5f, 1.5f));
    lightPositions.push_back(glm::vec3(-4.0f, 0.5f, -3.0f));
    lightPositions.push_back(glm::vec3(3.0f, 0.5f, 1.0f));
    lightPositions.push_back(glm::vec3(-.8f, 2.4f, -1.0f));
    // colors
    std::vector<glm::vec3> lightColors;
    lightColors.push_back(glm::vec3(5.0f, 5.0f, 5.0f));
    lightColors.push_back(glm::vec3(10.0f, 0.0f, 0.0f));
    lightColors.push_back(glm::vec3(0.0f, 0.0f, 15.0f));
    lightColors.push_back(glm::vec3(0.0f, 5.0f, 0.0f));


    // shader configuration
    // --------------------
    const int kernelSize = 11;
    if (kernelSize % 2 == 0) {
        cout << "Kernel size should be odd number!!"<<endl;
    }
    shader.use();
    shader.setInt("diffuseTexture", 0);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    shaderBlur.setInt("kernelSize", (kernelSize + 1) / 2);
    std::vector<float> kernel = filterCreation(kernelSize,2.0f);
    int half = (kernelSize - 1) / 2;
        for (int i = half; i < kernelSize; ++i) {
            cout << kernel[i] << "\t";
            shaderBlur.setFloat("weight[" + std::to_string(i-half) + "]", kernel[i]);
        }
    shaderBloomFinal.use();
    shaderBloomFinal.setInt("scene", 0);
    shaderBloomFinal.setInt("bloomBlur", 1);

    vector<float> dofFBO1(SCR_WIDTH*SCR_HEIGHT*3,0);
    vector<float> dofFBO2(SCR_WIDTH* SCR_HEIGHT * 3,0);
    vector<float> dofFBO3(SCR_WIDTH* SCR_HEIGHT * 3, 0);
    vector<float> dofDepth(SCR_WIDTH* SCR_HEIGHT);

    unsigned int testText;
    glGenTextures(1, &testText);
    glBindTexture(GL_TEXTURE_2D, testText);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, dofFBO3.data());
    //glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
    //float dofDepth[SCR_HEIGHT * SCR_WIDTH];
    // render loop
    // -----------
    int pingpong = 0;
    while (!glfwWindowShouldClose(window))
    {
        dofFBO1 = vector<float>(SCR_WIDTH * SCR_HEIGHT * 3, 0);
        dofFBO2= vector<float>(SCR_WIDTH * SCR_HEIGHT * 3, 0);
        dofFBO3 = vector<float>(SCR_WIDTH * SCR_HEIGHT * 3, 0);
        dofDepth= vector<float>(SCR_WIDTH * SCR_HEIGHT);
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        //cout << 1 << endl;
        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //// 1. render scene into floating point framebuffer
        //// -----------------------------------------------
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, zNear, zFar);
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        //glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glm::mat4 view = camera.GetViewMatrix();
       
        glm::mat4 vp = projection * view;

        glBindFramebuffer(GL_FRAMEBUFFER, helper.depthFBO);
        glBindTexture(GL_TEXTURE_2D, helper.depthMap);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, helper.geoData);
        helper.depthShader.use();
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(vp));
        draw(helper.depthShader, shaderLight, woodTexture, containerTexture, lightPositions, lightColors);

      
        //cout << helper.geoData[50]<< endl;

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        //int n = 10;
        //float aperture = 0.01;

        //glm::vec3 object(0);
        //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, zNear, zFar);
        //glm::vec3 right = glm::normalize(glm::cross(object - camera.Position, camera.Up));
        //glm::vec3 p_up = glm::normalize(glm::cross(object - camera.Position, right));
        //glm::mat4 mvp = projection * camera.GetViewMatrix();
        //for (int i = 0; i < n; i++) {
        //    glm::vec3 bokeh = right * cosf(i * 2 * M_PI / n) + p_up * sinf(i * 2 * M_PI / n);
        //    glm::mat4 modelview = glm::lookAt(camera.Position + aperture * bokeh, object, p_up);
        //    glm::mat4 mvp = projection * modelview;

        //    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        //    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(mvp));
        //    draw(shader, shaderLight, woodTexture, containerTexture, lightPositions, lightColors);
        //    glAccum(i ? GL_ACCUM : GL_LOAD, 1.0 / n);
        //}

        //glAccum(GL_RETURN, 1);
       /* glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, zNear, zFar);
        glm::mat4 view = camera.GetViewMatrix();*/
       
        shader.use();
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(vp));
        
        draw(shader, shaderLight, woodTexture, containerTexture, lightPositions, lightColors);
        glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_FLOAT, dofFBO1.data());
      
        glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, dofDepth.data());
        ////cout << dofFBO[0] << endl;
        ////#pragma omp parallel for
        float st = static_cast<float>(glfwGetTime());
        float _min = 10.0f;
        for (unsigned int i = 0; i < dofDepth.size(); i++) {

            dofDepth[i]  = normalizeDepth(dofDepth[i]);

          

           
        }
        //cout << _min << endl;
        //uniformDist(0, vector<GLubyte>(), dofFBO2);
        allR = vector<float>();


        for (unsigned int j = 0; j < SCR_HEIGHT; j++) {
            const unsigned int jMulWidth = j * SCR_WIDTH;
            for (unsigned int i = 0; i < SCR_WIDTH; i++) {
                //if (abs(i - SCR_WIDTH / 2.0) >= 30 || abs(j - SCR_HEIGHT / 2.0) >= 30) continue;
                const float invPixelDepth = 1/dofDepth[i + jMulWidth];
                const float idx = (i + jMulWidth) * 3;
                float c =abs(aperture * (screenPos * (invFocusLength - invPixelDepth) - 1));
                //c = min(20.0f,exp(6*(c - 1)));
                //c = min(20.0f,c);
                //c = abs(dofDepth[i + jMulWidth] - focusLength);
                //cout << c << endl;
            /*    if (i%8==0 && j%8==0)
                    if ((dofDepth[i + jMulWidth] - focus)>0.01f)
                        cout << dofDepth[i + jMulWidth] << " " << c << " <= " << focus << endl;*/
                ////c = 0.01f;
                //cout << 1/ invPixelDepth << endl;
                     uniformDist(c/2, i,j,vector<float>{dofFBO1[idx]
                         , dofFBO1[idx + 1]
                         , dofFBO1[idx + 2]}
                 , dofFBO2, dofFBO3);
            }
        
        }
        cout << "done" << endl;
        if (false) {
            float minIt = 10000,maxIt = -1,avg = 0.0f;
            for (unsigned int i = 0; i < dofFBO2.size(); i +=3) {
                if (dofFBO2[i] > maxIt)  maxIt = dofFBO2[i];
                if (dofFBO2[i] < minIt)  minIt = dofFBO2[i];
                avg += dofFBO2[i];
            }
           avg = avg/dofFBO2.size();
           for (unsigned int i = 0; i < dofFBO2.size(); i += 3) {
           
               dofFBO3[i] *= avg / dofFBO2[i];
               dofFBO3[i+1] *= avg / dofFBO2[i];
               dofFBO3[i+2] *= avg / dofFBO2[i];
           }
        }

        glBindTexture(GL_TEXTURE_2D, testText);
        if (pingpong == 0)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, dofFBO2.data());
        else 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, dofFBO3.data());
        pingpong = 1;// 1 - pingpong;
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        float et = static_cast<float>(glfwGetTime());
        //cout << et - st << endl;
        //glDrawPixels(SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_FLOAT, dofFBO2.data());

        // 2. blur bright fragments with two-pass Gaussian Blur 
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 0;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //// 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        //// --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderBloomFinal.use();
        glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glBindTexture(GL_TEXTURE_2D,testText);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        //glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderBloomFinal.setInt("bloom", bloom);
        shaderBloomFinal.setFloat("exposure", exposure);
        renderQuad();
        //glDrawPixels(SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_FLOAT, dofFBO2.data());
        //std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    cout << "allR" << endl;
    for (auto r : allR)
        cout << r << endl;

    glfwTerminate();
    return 0;
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.001f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.001f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    //camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }
      
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Function to create Gaussian filter

#define K  1
std::vector<float> filterCreation(const int size,const float sigma)
{
    float sigmaPow2 = sigma * sigma;
    auto IX = [&](int i, int j)
    {
        return i + j * size;
    };
    std::vector<float> kernel;

    // initialising standard deviation to 1.0
    double r, s = 2.0 * sigmaPow2;
    int size2 = int(size / 2);

    // sum is for normalization
    double sum = 0.0;
    for (int i = 0; i < size; ++i)
    {
        kernel.push_back(0.0f);
    }
    // generating 5x5 kernel
    for (int i = 0; i < size; i++) {
        float x = i - (size - 1) / 2;
        kernel[i] = K * exp((pow(x, 2) / s) * (-1));
        sum += kernel[i];
    }

    // normalising the Kernel
    for (int i = 0; i < kernel.size(); i++)
        kernel[i] /= sum;
    return kernel;
}


void uniformDist(float r,int x,int y,vector<float> prevValue, vector<float>& fb2, vector<float>& fb3) {

    //r = r * SCR_WIDTH;
    r = max(1.0f,r);
    //r = min(20.0f, r);

    //r = round(r);
   //r = 1.0f;
    //r = max(8.0f,r);
    //cout << r << endl;
    const float r2 = r * r;
    float a = glm::pi<float>()*r2;
    //cout << r << endl;
    
    float invA = 1 / a;
    //float rr = ceil(r * 100.0) / 100.0;
    const int maxPts = (int)(4 * r * r);
    vector<int> nxv, nyv;
    int pts = 0,area=0;
   /* if (r!=1)
    cout << r << endl;*/
    fb2[(int)(x + y * SCR_WIDTH) * 3] = r/2;
    fb2[(int)(x + y * SCR_WIDTH) * 3 + 1] = r / 2;
    fb2[(int)(x + y * SCR_WIDTH) * 3 + 2] = r / 2;
    //return;
    /*if (r!=1)
    cout << r << endl;*/
    glm::vec3 intensity(prevValue[0]  ,prevValue[1], prevValue[2]);
    //cout << intensity[0] << " " << intensity[1] << " " << intensity[2] << endl;
    for (int row = max(0.0f,-r + y); row <= min((float)SCR_WIDTH-1,r + y); row++) {
        const unsigned int row2 = row * row;
        const unsigned int rowMulWidth = row * SCR_WIDTH;
        int rawRow2 = pow(row-y,2);
        for (int col = max(0.0f, -r + x); col <= min((float)SCR_HEIGHT-1, r + x); col++) {
            int rawCol = col -x;
            float idx = (int)(col + rowMulWidth) * 3;
            
            if (rawCol * rawCol + rawRow2 <= r2) {
                //cout << col << " " << row << endl;
                //cout << rawCol << " : " << col << " " << (row - SCR_CENTER.y) << " : " << row << endl;
                //fb2[idx] += 0.1f;// intensity[0];
                //fb2[idx +1] += 0.1f;// intensity[1];
                //fb2[idx +2] += 0.1f;// intensity[2];
                pts++;
                area++;
                nxv.push_back( col);
                nyv.push_back( row);
               
                //if (isnan(static_cast<double>(fb2[idx])))
                //cout << intensity[0] << " " << intensity[1] << " " << intensity[2] << endl;
            }
        }
    }
    intensity /= area;
    //cout << "a" << pts << " " << nxv.size() << endl;
    for (unsigned int i = 0; i < pts; i++) {
        fb3[(nxv[i] + nyv[i] * SCR_WIDTH)*3] += intensity.r;
        fb3[(nxv[i] + nyv[i] * SCR_WIDTH) * 3 + 1] += intensity.g;
        fb3[(nxv[i] + nyv[i] * SCR_WIDTH) * 3 + 2] += intensity.b;

    }
    //cout << " b " << endl;


}


void draw(Shader shader,Shader shaderLight,unsigned int woodTexture,unsigned int containerTexture
    ,vector<glm::vec3> lightPositions,vector<glm::vec3> lightColors) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, zNear, zFar);
    //projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, zNear, zFar);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
   
    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodTexture);
    // set lighting uniforms
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        shader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
        shader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
    }
    shader.setVec3("viewPos", camera.Position);
    // create one large cube that acts as the floor
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
    shader.setMat4("model", model);
    renderCube();
    // then create multiple cubes as the scenery
    glBindTexture(GL_TEXTURE_2D, containerTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 0.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 4.0));
    model = glm::scale(model, glm::vec3(5.0f, 5.0f, 0.1f));
    shader.setMat4("model", model);
    //renderCube();


    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0));
    model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    shader.setMat4("model", model);
    //renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();

    // finally show all the light sources as bright cubes
    shaderLight.use();

    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(lightPositions[i]));
        model = glm::scale(model, glm::vec3(0.25f));
        shaderLight.setMat4("model", model);
        shaderLight.setVec3("lightColor", lightColors[i]);
        //renderCube();
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cout << lastX << " " << lastY << endl;
        focus = normalizeDepth(helper.geoData[(int)(lastX + (SCR_HEIGHT - lastY) * SCR_WIDTH)]);
        screenPos = ((1. / aperture) + 1) / (invFocusLength- 1/focus);
        //glm::vec3 data = helper.calWordPos(glm::vec2(lastX, lastY));
     
        cout << focus << endl;
    }
}


float normalizeDepth(float depth) {
    return  (zNear * zFar) / (zFar - depth * (zFar - zNear)) ;
}

