#include <stdio.h>
#include <iostream>
#include <ctime>

#include<GL/glew.h>
#include<GLFW/glfw3.h>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include"Camera.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);


const float toR = 3.14 / 180.0;
GLuint VAO, VBO, IBO, shader, v_uniform, f_uniform, uniform_Model, uniform_Projection, uniform_view, u_i;
float v_random, offset, angle;
bool dir;
const float radius = 10.0f;
float camX, camZ;
glm::mat4 view;


// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


static const char* vshader = " \n\
#version 330 \n\
in vec3 pos; \n\
out vec4 vColor; \n\
uniform float s; \n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
attribute vec3 position; \n\
attribute vec3 normal; \n\
uniform mat4 projection, modelview, normalMat; \n\
varying vec3 normalInterp; \n\
varying vec3 vertPos; \n\
void main()              \n\
{  \n\
	gl_Position = projection*view*model*vec4(pos,1.0); \n\
	vColor= vec4(clamp(pos,0.0,1.0), 1.0); \n\
	vec4 vertPos4 = modelview * vec4(position, 1.0);\n\
	vertPos = vec3(vertPos4) / vertPos4.w;\n\
	normalInterp = vec3(normalMat * vec4(normal, 0.0));\n\
	gl_Position = projection * vertPos4;\n\
} \n\
";

static const char* fshader = " \n\
#version 330 \n\
precision mediump float;\n\
varying vec3 normalInterp; \n\
varying vec3 vertPos;   \n\
uniform int mode; \n\
uniform float Ka;\n\
uniform float Kd;  \n\
uniform float Ks;   \n\
uniform float shininessVal; \n\
uniform vec3 ambientColor;\n\
uniform vec3 diffuseColor; \n\
uniform vec3 specularColor;\n\
uniform vec3 lightPos;\n\
void main()              \n\
{  \n\
	vec3 N = normalize(normalInterp); \n\
	vec3 L = normalize(lightPos - vertPos); \n\
	float lambertian = max(dot(N, L), 0.0); \n\
	float specular = 0.0; \n\
	if(lambertian > 0.0) { \n\
	vec3 R = reflect(-L, N); \n\
	vec3 V = normalize(-vertPos); \n\
	float specAngle = max(dot(R, V), 0.0); \n\
	specular = pow(specAngle, shininessVal); \n\
	}\n\
gl_FragColor = vec4(Ka * ambientColor +Kd* lambertian* diffuseColor +Ks * specular* specularColor, 1.0);\n\
	if(mode == 2) gl_FragColor = vec4(Ka * ambientColor, 1.0); \n\
	  if(mode == 3) gl_FragColor = vec4(Kd * lambertian * diffuseColor, 1.0); \n\
	  if(mode == 4) gl_FragColor = vec4(Ks * specular * specularColor, 1.0);\n\
	}\n\
} \n\
";

void createShape()
{
	unsigned int indicies[] = {
		0,1,2,
		1,2,3,
		0,2,3
	};

	GLfloat vertices[] = {
		-1.0f, -1.0f,0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, -1.0f, 1.0f
	};

	//VAO--VBO-->data-->pointer)
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void compileShader()
{
	shader = glCreateProgram();
	if (!shader)
	{
		printf("shader error");
		return;
	}
	//create shader 
	GLuint verShader = glCreateShader(GL_VERTEX_SHADER);
	//assign shader
	glShaderSource(verShader, 1, &vshader, NULL);
	//compile shader
	glCompileShader(verShader);

	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fshader, NULL);
	glCompileShader(fragShader);

	//attach shaders to shaderprogram 
	glAttachShader(shader, verShader);
	glAttachShader(shader, fragShader);
	//attach shaderprogram to program
	glLinkProgram(shader);

	v_uniform = glGetUniformLocation(shader, "s");
	f_uniform = glGetUniformLocation(shader, "c");
	uniform_Model = glGetUniformLocation(shader, "model");
	uniform_Projection = glGetUniformLocation(shader, "projection");
	uniform_view = glGetUniformLocation(shader, "view");
	u_i = glGetUniformLocation(shader, "i");


}


int main()
{
	angle = offset = 0.0;
	//init glfw
	if (!glfwInit())
	{
		printf("error: glfw");
		glfwTerminate();
		return 1;
	}
	//init & setup window
	GLFWwindow* mainwindow = glfwCreateWindow(800, 600, "shader", NULL, NULL);
	//assign window
	if (!mainwindow)
	{
		printf("error:mainWindow");
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(mainwindow);
	//glfwSetCursorPosCallback(mainwindow, mouse_callback);
	glfwSetScrollCallback(mainwindow, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(mainwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	if (glewInit() != GLEW_OK)
	{
		printf("error:glew");
		glfwDestroyWindow(mainwindow);
		glfwTerminate();
		return 1;
	}
	//while loop 
	glEnable(GL_DEPTH_TEST);
	createShape();
	srand(time(NULL));

	glm::mat4 projection = glm::perspective(65.0f, 1.3f, 0.1f, 100.0f);

	//glm::lookAt(glm::vec3(200.0,200.0, 10.0), glm::vec3(100.0, 100.0, 100.0), glm::vec3(1.0, 0.0, 0.0));
	while (!glfwWindowShouldClose(mainwindow))
	{
		if (dir)
		{
			offset += 0.005;
		}
		else
		{
			offset -= 0.005;
		}
		if (abs(offset) > 0.9)
		{
			dir = !dir;
		}
		angle += 0.5;
		if (angle > 360.0)
		{
			angle = 0.0;
		}
		compileShader();

		processInput(mainwindow);

		v_random = rand() % 10 / 10.0;
		printf("%f\n", v_random);
		//printf("random value %f", random);
		//handle event 
		//bg
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//draw
		glUseProgram(shader);
		//glUniform1f(v_uniform, v_random);
		//glUniform3f(f_uniform, v_random * 2.0, v_random / 3.0, v_random * v_random);
		projection = glm::perspective(glm::radians(camera.Zoom), 1.3f, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		glm::mat4 model(1.0);
		model = glm::translate(model, glm::vec3(0.0, offset, -2.0));
		//model = glm::rotate(model, angle * toR, glm::vec3(0.0, 1.0, 0.0));
		model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));

		glUniformMatrix4fv(uniform_Model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(uniform_Projection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniform_view, 1, GL_FALSE, glm::value_ptr(view));

		//glm::vec4 intensity(0.0f, 0.0f, 0.0f, 0.0f);
		glUniform4f(u_i, 0.2f, 0.2f, 0.9f, 1.0f);

		glBindVertexArray(VAO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		//buffer setup		
		glfwPollEvents();

		glfwSwapBuffers(mainwindow);
	}
	return 0;
}

void processInput(GLFWwindow * window)
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
}

void mouse_callback(GLFWwindow * window, double xpos, double ypos)
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
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}