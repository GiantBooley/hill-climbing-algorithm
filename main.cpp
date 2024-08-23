#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;

float randFloat() {
	return (float)rand() / static_cast<float>(RAND_MAX);
}

class Texture {
public:
	int width, height, numChannels;
	unsigned int texture;
	Texture(const char* path) {
		stbi_set_flip_vertically_on_load(true);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		unsigned char *data = stbi_load(path, &width, &height, &numChannels, 0);
		cout << "tex has " << numChannels << " color channels" << endl;
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		} else {
			cout << "[ERROR] failed to load \"" << path << "\"" << endl;
		}
		stbi_image_free(data);
	}
	void use() {
		glBindTexture(GL_TEXTURE_2D, texture);
	}
};
class Shader {
public:
	unsigned int ID;

	Shader(const char* vertexPath, const char* fragmentPath) {
		string vertexText;
		string fragmentText;
		ifstream vertexFile;
		ifstream fragmentFile;
		vertexFile.exceptions(ifstream::failbit | ifstream::badbit);
		fragmentFile.exceptions(ifstream::failbit | ifstream::badbit);
		try {
			vertexFile.open(vertexPath);
			fragmentFile.open(fragmentPath);
			stringstream vertexStream, fragmentStream;
			vertexStream << vertexFile.rdbuf();
			fragmentStream << fragmentFile.rdbuf();
			vertexFile.close();
			fragmentFile.close();
			vertexText = vertexStream.str();
			fragmentText = fragmentStream.str();
		} catch (ifstream::failure const&) {
			cout << "[ERROR] failed to get fragment or vertex text" << endl;
		}
		const char* vertexCode = vertexText.c_str();
		const char* fragmentCode = fragmentText.c_str();

		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		//vertex n fragment
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vertexCode, NULL);
		glCompileShader(vertex);
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragmentCode, NULL);
		glCompileShader(fragment);

		//errors shaders
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			cout << "[ERROR] vertex shader compile failed\n" << infoLog << endl;
		}
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			cout << "[ERROR] fragment shader compile failed\n" << infoLog << endl;
		}

		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);

		//errors
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			cout << "[ERROR] program failed linking\n" << infoLog << endl;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}
	void use() {
		glUseProgram(ID);
	}

	// uniform shortcuts
	void setBool(const string &name, bool value) const {
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setInt(const string &name, int value) {
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setFloat(const string &name, float value) {
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
};
int width = 640, height = 480;
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
	glViewport(0, 0, width, height);
}

float vertices[] = {
	-0.5f, -0.5f, 0.f, 0.f,
	-0.5f, 0.5f, 0.f, 1.f,
	0.5f, 0.5f, 1.f, 1.f,
	0.5f, -0.5f, 1.f, 0.f
};
unsigned int indices[] = {
	0, 1, 2,
	0, 2, 3
};

struct {
	bool w, a, s, d, q, e, right, left, up, down;
} controls;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_Q) controls.q = true;
		if (key == GLFW_KEY_W) controls.w = true;
		if (key == GLFW_KEY_E) controls.e = true;
		if (key == GLFW_KEY_A) controls.a = true;
		if (key == GLFW_KEY_S) controls.s = true;
		if (key == GLFW_KEY_D) controls.d = true;
		if (key == GLFW_KEY_RIGHT) controls.right = true;
		if (key == GLFW_KEY_LEFT) controls.left = true;
		if (key == GLFW_KEY_UP) controls.up = true;
		if (key == GLFW_KEY_DOWN) controls.down = true;
		if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_Q) controls.q = false;
		if (key == GLFW_KEY_W) controls.w = false;
		if (key == GLFW_KEY_E) controls.e = false;
		if (key == GLFW_KEY_A) controls.a = false;
		if (key == GLFW_KEY_S) controls.s = false;
		if (key == GLFW_KEY_D) controls.d = false;
		if (key == GLFW_KEY_RIGHT) controls.right = false;
		if (key == GLFW_KEY_LEFT) controls.left = false;
		if (key == GLFW_KEY_UP) controls.up = false;
		if (key == GLFW_KEY_DOWN) controls.down = false;
	}
};
struct Sprite {
	float x, y, width, height, rot; // x y center
};

int main(void) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//init glfw
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Reduction", NULL, NULL);
	if (window == NULL) {
		cout << "[ERROR] Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	// init glad
	if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
		cout << "[ERROR] Failed to initialize glad" << endl;
		return -1;
	}

	//stuff
	glViewport(0, 0, 640, 480);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	Shader triangleShader{"vertex.vsh", "fragment.fsh"};
	Texture skateboardTexture{"asdasdasdasd.jpeg"};

	//buffers
	unsigned int VBO, EBO, VAO;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//i am graphical user interface
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 300 es");

	vector<Sprite> sprites;

	int frameCount = 0;
	int fps = 0;
	double lastFpsFrameTime = glfwGetTime();
	//llooop
	while (!glfwWindowShouldClose(window)) {
		sprites.push_back({randFloat() * 20.f, randFloat() * 20.f, randFloat() * 3.f, randFloat() * 3.f, randFloat() * 3.14159f});
		//init imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glClearColor(0.5f, 0.8f, 0.3f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 proj = glm::ortho(0.f, 20.f, 0.f, 20.f);

		unsigned int modelLocation = glGetUniformLocation(triangleShader.ID, "modelMat");
		unsigned int projLocation = glGetUniformLocation(triangleShader.ID, "projMat");

		triangleShader.use();
		skateboardTexture.use();
		glBindVertexArray(VAO);

		glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(proj));
		for (Sprite& sprite : sprites) {
			glm::mat4 model = glm::mat4(1.f);
			model = glm::translate(model, glm::vec3(sprite.x, sprite.y, 0.f));
			model = glm::rotate(model, sprite.rot, glm::vec3(0.f, 0.f, -1.f));
			model = glm::scale(model, glm::vec3(sprite.width, sprite.height, 1.f));

			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}


		ImGui::Begin("reduction");
		ImGui::Text("FPS: %d", fps);
		ImGui::Text("count: %d", (int)sprites.size());
		if (ImGui::Button("add")) {
			sprites.push_back({randFloat() * 20.f, randFloat() * 20.f, randFloat() * 3.f, randFloat() * 3.f, randFloat() * 3.14159f});
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();

		frameCount++;
		double frameTime = glfwGetTime();
		if (frameTime > lastFpsFrameTime + 1.) {
			lastFpsFrameTime = frameTime;
			fps = frameCount;
			frameCount = 0;
		}
	}
	glfwTerminate();
	return 0;
}
