#define GLM_FORCE_RADIANS
#include <SOIL.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <list>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "cubemapVert.h"
//#include "sound.h"
#include "keys.h"

#include <stdio.h>  
#include <stdlib.h> 

#include <iostream>
#include <fstream> //fstream
#include <ctime> 

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, Keys keys);

enum Post_Mode { POST_NORM, POST_INVR, POST_GRAY };
enum Light_Mode { LIGHT_NORM, LIGHT_DIFF, LIGHT_SPEC, LIGHT_DFSP, LIGHT_DIST, LIGHT_TEXT };

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
//TODO: Make nicer!
bool i_press = false,
	 g_press = false,
	 m_press = false,
	 f1_press = false,
	 antialiasing = false;
Post_Mode post_mode = POST_NORM;
Light_Mode light_mode = LIGHT_NORM;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, -4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int loadCubemap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char *data = SOIL_load_image(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			SOIL_free_image_data(data);
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			SOIL_free_image_data(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfwWindowHint(GLFW_SAMPLES, 4);
	glDisable(GL_MULTISAMPLE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

														 // glfw window creation
														 // --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tickets, Please", NULL, NULL);
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

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glewExperimental = GL_TRUE;
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	GLenum err = glewInit();

	//If GLEW hasn't initialized  
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return -1;
	}

	// get cubemap texture
	std::vector<std::string> faces = {
		"../objects/skybox/right.tga",
		"../objects/skybox/left.tga",
		"../objects/skybox/top.tga",
		"../objects/skybox/bottom.tga",
		"../objects/skybox/back.tga",
		"../objects/skybox/front.tga"
	};
	unsigned int cubemapTexture = loadCubemap(faces);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// build and compile shaders
	// -------------------------
	Shader ourShader("shader.vert", "shader.frag");
	Shader skyboxShader("cubemap.vert", "cubemap.frag");
	Shader screenShader("motionBlur.vert", "motionBlur.frag");

	// Process skybox VAO/VBO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Process quad VAO/VBO
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));	

	std::vector<int> interestingKeys = std::vector<int>({
		GLFW_KEY_ESCAPE,
		GLFW_KEY_LEFT_CONTROL,
		GLFW_KEY_LEFT_SHIFT,
		GLFW_KEY_W,
		GLFW_KEY_S,
		GLFW_KEY_A,
		GLFW_KEY_D,
		GLFW_KEY_E,
		GLFW_KEY_I,
		GLFW_KEY_M,
		GLFW_KEY_G,
		GLFW_KEY_F1,
		GLFW_KEY_F2,
		GLFW_KEY_F3,
		GLFW_KEY_F4,
		GLFW_KEY_F5,

	});

	Keys keys = Keys(window, interestingKeys);

	// don't forget to enable shader before setting uniforms
	ourShader.use();
	Model ourModel3("../objects/map2.obj");
	Model ourModel4 = ourModel3;
	Model ourModel5("../objects/mapend.obj");
	Model ourModel6 = ourModel5;
	GLint light_mode_location = glGetUniformLocation(ourShader.ID, "light_mode");

	skyboxShader.use();

	screenShader.use();
	GLint post_mode_location = glGetUniformLocation(screenShader.ID, "mode");

	// Configure frame buffer
	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Multisampling!
	// 1. Configure multisample framebuffer
	unsigned int framebuffer;
	glGenTextures(1, &framebuffer);
	glBindTexture(GL_FRAMEBUFFER, framebuffer);

	// 2. Create multisampled colour attachment tex
	int MS_SIZE = 8;
	unsigned int texColBuffMS;
	glGenTextures(1, &texColBuffMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texColBuffMS);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SIZE, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texColBuffMS, 0);

	// 3. Create render buffer for depth & stencil
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, MS_SIZE, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 4. Create intermediate framebuffer
	unsigned int intermediateFBO;
	glGenFramebuffers(1, &intermediateFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

	// Current frame buffer
	unsigned int texColorBuffer;
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);
	glm::mat4 prevView;
	int frameCount = 0;
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// render loop
		// -----------
		glEnable(GL_DEPTH_TEST);
		while (!glfwWindowShouldClose(window))
		{
			// per-frame time logic
			// --------------------
			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			// input
			// -----
			processInput(window, keys);

			// render
			// ------
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glClearColor(0.00f, 0.00f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDisable(GL_BLEND);

			ourShader.use();

			// set the lighting mode
			glUniform1i(light_mode_location, light_mode);

			// view/projection transformations
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			ourShader.setMat4("proj", projection);
			ourShader.setMat4("view", view);

			glm::mat4 model3;
			//	model3 = glm::rotate(model3, 1.5f, glm::vec3(0.0f, 0.0f, 0.0f));
			model3 = glm::translate(model3, glm::vec3(-0.75f, -1.5f, 0.0f)); // translate it down so it's at the center of the scene
			model3 = glm::scale(model3, glm::vec3(0.65f, 0.50f, 0.50f));	// it's a bit too big for our scene, so scale it down
			ourShader.setMat4("model", model3);
			ourModel3.Draw(ourShader);

			glm::mat4 model4;
			model4 = glm::translate(model4, glm::vec3(-0.75f, -1.5f, -5.38f)); // translate it down so it's at the center of the scene
			model4 = glm::scale(model4, glm::vec3(0.65f, 0.50f, 0.50f));
			ourShader.setMat4("model", model4);
			ourModel4.Draw(ourShader);

			glm::mat4 model5;
			model5 = glm::translate(model5, glm::vec3(0.05f, -1.5f, -9.38f)); // translate it down so it's at the center of the scene
			model5 = glm::scale(model5, glm::vec3(0.65f, 0.50f, 0.50f));
			ourShader.setMat4("model", model5);
			ourModel5.Draw(ourShader);

			glm::mat4 model6;
			model6 = glm::rotate(model6, 3.15f, glm::vec3(0.0f, 1.0f, 0.0f));
			model6 = glm::translate(model6, glm::vec3(0.05f, -1.5f, -2.0f)); // translate it down so it's at the center of the scene
			model6 = glm::scale(model6, glm::vec3(0.65f, 0.50f, 0.50f));
			ourShader.setMat4("model", model6);
			ourModel6.Draw(ourShader);

			// create light position
			glm::vec4 light_position = model3 *  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			ourShader.setVec4("light_position", light_position);

			glm::vec4 light_colour(1.0f, 1.0f, 1.0f, 1.0f);
			ourShader.setVec4("light_colour", light_colour);

			glEnable(GL_BLEND);

			// draw skybox as last
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			skyboxShader.use();
			view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
			skyboxShader.setMat4("view", view);
			skyboxShader.setMat4("projection", projection);
			// skybox cube
			glBindVertexArray(skyboxVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
			glDepthFunc(GL_LESS); // set depth function back to default

			// draw
			// ------
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			// Now back to default framebuffer and draw our quad
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);

			screenShader.use();
			glUniform1i(post_mode_location, post_mode);
			screenShader.setMat4("view", view);
			screenShader.setMat4("proj", projection);
			glBindVertexArray(quadVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texColorBuffer);
			frameCount++;
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glEnable(GL_DEPTH_TEST);

			prevView = view;
			keys.update();
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glDeleteFramebuffers(1, &fbo);
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVAO);

	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow*window, Keys keys)
{
	if (keys.isPressed(GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, true);
	}

	if (keys.isJustPressed(GLFW_KEY_LEFT_CONTROL)) {
		camera.setCrouch(true);
	}

	if (keys.isJustReleased(GLFW_KEY_LEFT_CONTROL)) {
		camera.setCrouch(false);
	}

	// Post processing mode selection
	// Toggle inversion
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) { i_press = true; }
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE && i_press) {
		post_mode = (post_mode == POST_INVR ? POST_NORM : POST_INVR);
		i_press = false;
	}
	// Toggle grayscale
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) { g_press = true; }
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE && g_press) {
		post_mode = (post_mode == POST_GRAY ? POST_NORM : POST_GRAY);
		g_press = false;
	}
	// Toggle motion blur - TODO: Reimplement when working
	/*
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) { m_press = true; }
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE && m_press) {
		post_mode = (post_mode == 3 ? 0 : 3);
		m_press = false;
	}
	*/

	if (keys.isJustPressed(GLFW_KEY_E)) {
		std::cout << "The E key was JUST pressed" << std::endl;
	}

	// Lighting mode selection
	if (keys.isJustPressed(GLFW_KEY_F1)) {
		light_mode = (light_mode == LIGHT_DIFF ? LIGHT_NORM : LIGHT_DIFF);
	}

	if (keys.isJustPressed(GLFW_KEY_F2)) {
		light_mode = (light_mode == LIGHT_SPEC ? LIGHT_NORM : LIGHT_SPEC);
	}

	if (keys.isJustPressed(GLFW_KEY_F3)) {
		light_mode = (light_mode == LIGHT_DFSP ? LIGHT_NORM : LIGHT_DFSP);
	}

	if (keys.isJustPressed(GLFW_KEY_F4)) {
		light_mode = (light_mode == LIGHT_DIST ? LIGHT_NORM : LIGHT_DIST);
	}

	if (keys.isJustPressed(GLFW_KEY_F5)) {
		light_mode = (light_mode == LIGHT_TEXT ? LIGHT_NORM : LIGHT_TEXT);
	}

	int speed = 1;

	if (keys.isPressed(GLFW_KEY_LEFT_SHIFT)) {
		speed = 2;
	}

	if (keys.isPressed(GLFW_KEY_W)) {
		camera.ProcessKeyboard(FORWARD, deltaTime * speed);
	}

	if (keys.isPressed(GLFW_KEY_S)) {
		camera.ProcessKeyboard(BACKWARD, deltaTime * speed);
	}

	if (keys.isPressed(GLFW_KEY_A)) {
		camera.ProcessKeyboard(LEFT, deltaTime * speed);
	}

	if (keys.isPressed(GLFW_KEY_D)) {
		camera.ProcessKeyboard(RIGHT, deltaTime * speed);
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

