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
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
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
	unsigned int id;
	Texture(const char* path) {
		stbi_set_flip_vertically_on_load(true);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
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
class FrameBuffer {
public:
	unsigned int framebuffer;
	unsigned int textureColorBuffer;
	unsigned int rbo;
	int width, height;
	FrameBuffer(int w, int h) {
		width = w;
		height = h;

		glGenFramebuffers(1, &framebuffer);

		glGenTextures(1, &textureColorBuffer);
		glBindTexture(GL_TEXTURE_2D, textureColorBuffer);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_INT, NULL);

		//texture settings
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) cout << "[ERROR] Framebuffer is not complete" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void resize(int w, int h) {
		width = w;
		height = h;
		glBindTexture(GL_TEXTURE_2D, textureColorBuffer);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_INT, NULL);

		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
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
glm::mat4 getSpriteModelMat(Sprite* sprite) {
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(sprite->x, sprite->y, 0.f));
	model = glm::rotate(model, sprite->rot, glm::vec3(0.f, 0.f, -1.f));
	model = glm::scale(model, glm::vec3(sprite->width, sprite->height, 1.f));
	return model;
}
float lerp(float a, float b, float t) {
	return (b - a) * t + a;
}
float* getSpriteBoundingBox(Sprite* sprite) {
	glm::vec4 points[4] = {
		glm::vec4(vertices[0], vertices[1], 0.f, 1.f),
		glm::vec4(vertices[4], vertices[5], 0.f, 1.f),
		glm::vec4(vertices[8], vertices[9], 0.f, 1.f),
		glm::vec4(vertices[12], vertices[13], 0.f, 1.f)
	};
	glm::mat4 focusModelMat = getSpriteModelMat(sprite);
	for (int i = 0; i < 4; i++) {
		points[i] = focusModelMat * points[i];
	}

	float* aabb = (float*)malloc(sizeof(float) * 4);
	aabb[0] = min(min(points[0].x, points[1].x), min(points[2].x, points[3].x)); // minx
	aabb[1] = max(max(points[0].x, points[1].x), max(points[2].x, points[3].x)); // maxx
	aabb[2] = min(min(points[0].y, points[1].y), min(points[2].y, points[3].y)); // miny
	aabb[3] = max(max(points[0].y, points[1].y), max(points[2].y, points[3].y)); // maxy
	return aabb;
}
float* getDoubleBoundingBoxBoundingBox(float* aabb1, float* aabb2) {
	float* aabb = (float*)malloc(sizeof(float) * 4);
	aabb[0] = min(aabb1[0], aabb2[0]);
	aabb[1] = max(aabb1[1], aabb2[1]);
	aabb[2] = min(aabb1[2], aabb2[2]);
	aabb[3] = max(aabb1[3], aabb2[3]);
	return aabb;
}
void randomlyTransformSprite(Sprite* sprite) {
	sprite->x += randFloat() * 0.2f - 0.1f;
	sprite->y += randFloat() * 0.2f - 0.1f;
	sprite->width += randFloat() * 0.2f - 0.1f;
	sprite->height += randFloat() * 0.2f - 0.1f;
	sprite->rot += randFloat() * 0.2f - 0.1f;
}
float getAabbArea(float* aabb) {
	return abs((aabb[1] - aabb[0]) * (aabb[3] - aabb[2]));
}
bool save = false;
double getAverageDifference(float x, float y, float w, float h) {
	GLsizei stride = w * 3;
	GLsizei bufferSize = stride * h;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK);
	stbi_flip_vertically_on_write(true);
	unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
	glReadnPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba

	double average = 0.;
	for (int i = 0; i < bufferSize; i += 3) { // 256*3
		average += (double)bufferData[i] / 768. + (double)bufferData[i + 1] / 768. + (double)bufferData[i + 2] / 768.;
	}
	average /= (double)bufferSize;
	if (save) {
		int write = stbi_write_png("difference.png", w, h, 3, bufferData, stride);
		save = false;
	}
	free(bufferData);
	return average;
}

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


	//init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 300 es");

	//stuff
	glViewport(0, 0, 640, 480);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(0);

	Shader triangleShader{"vertex.vsh", "fragment.fsh"};
	unsigned int triangleShaderModelLocation = glGetUniformLocation(triangleShader.ID, "modelMat");
	unsigned int triangleShaderProjLocation = glGetUniformLocation(triangleShader.ID, "projMat");
	unsigned int triangleShaderTexLocation = glGetUniformLocation(triangleShader.ID, "tex");

	Shader differenceShader{"vertex.vsh", "difference.fsh"};
	unsigned int differenceShaderModelLocation = glGetUniformLocation(differenceShader.ID, "modelMat");
	unsigned int differenceShaderProjLocation = glGetUniformLocation(differenceShader.ID, "projMat");
	unsigned int differenceShaderTex1Location = glGetUniformLocation(differenceShader.ID, "tex1");
	unsigned int differenceShaderTex2Location = glGetUniformLocation(differenceShader.ID, "tex2");
	unsigned int differenceShaderAabbLocation = glGetUniformLocation(differenceShader.ID, "aabb");

	Texture skateboardTexture{"asdasdasdasd.jpeg"};
	Texture targetTexture{"target.jpg"};

	FrameBuffer boundingBoxBuffer{640, 480};
	FrameBuffer differenceBuffer{640, 480};

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

	vector<Sprite> sprites;
	bool showDifference = false;

	int frameCount = 0;
	int fps = 0;
	double lastFpsFrameTime = glfwGetTime();
	sprites.push_back({randFloat() * 640.f, randFloat() * 480.f, randFloat() * 100.f, randFloat() * 100.f, randFloat() * 3.14159f * 2.f});

	glClearColor(0.f, 0.f, 0.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skateboardTexture.id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, targetTexture.id);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);

	glBindVertexArray(VAO);
	//llooop
	while (!glfwWindowShouldClose(window)) {
		Sprite* last = &sprites.at(sprites.size() - 1);
		Sprite after = *last;
		randomlyTransformSprite(&after);
		float* aabbBefore = getSpriteBoundingBox(last);
		float* aabbAfter = getSpriteBoundingBox(&after);
		float* differenceAabb = getDoubleBoundingBoxBoundingBox(aabbBefore, aabbAfter);
		float* screenDifferenceAabb = (float*)malloc(sizeof(float) * 4);
		screenDifferenceAabb[0] = differenceAabb[0] / 640.f;
		screenDifferenceAabb[1] = differenceAabb[1] / 640.f;
		screenDifferenceAabb[2] = differenceAabb[2] / 480.f;
		screenDifferenceAabb[3] = differenceAabb[3] / 480.f;

		glm::mat4 fullscreenProj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);
		glm::mat4 aabbProj = glm::ortho(differenceAabb[0], differenceAabb[1], differenceAabb[2], differenceAabb[3], -1.f, 1.f);
		glm::mat4 identity = glm::mat4(1.f);

		int w = (int)(differenceAabb[1] - differenceAabb[0]);
		int h = (int)(differenceAabb[3] - differenceAabb[2]);

		//resize
		boundingBoxBuffer.resize(w, h);
		differenceBuffer.resize(w, h);
		glViewport(0, 0, w, h);

		// render before transform===================================================================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		triangleShader.use();

		glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(aabbProj));
		glUniform1i(triangleShaderTexLocation, 0);
		for (Sprite& sprite : sprites) {
			glm::mat4 model = getSpriteModelMat(&sprite);
			glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		// get difference==================================================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		differenceShader.use();

		glUniformMatrix4fv(differenceShaderProjLocation, 1, GL_FALSE, glm::value_ptr(fullscreenProj));
		glUniformMatrix4fv(differenceShaderModelLocation, 1, GL_FALSE, glm::value_ptr(identity));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);
		glUniform1i(differenceShaderTex1Location, 1);
		glUniform1i(differenceShaderTex2Location, 2);
		glUniform1fv(differenceShaderAabbLocation, 4, screenDifferenceAabb);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		double averageDifferenceBefore = getAverageDifference(0, 0, w, h);


		Sprite before = *last;
		*last = after;
		// render after transform===================================================================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		triangleShader.use();

		glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(aabbProj));
		glUniform1i(triangleShaderTexLocation, 0);
		for (Sprite& sprite : sprites) {
			glm::mat4 model = getSpriteModelMat(&sprite);
			glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		// get difference============================================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		differenceShader.use();

		glUniformMatrix4fv(differenceShaderProjLocation, 1, GL_FALSE, glm::value_ptr(fullscreenProj));
		glUniformMatrix4fv(differenceShaderModelLocation, 1, GL_FALSE, glm::value_ptr(identity));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);
		glUniform1i(differenceShaderTex1Location, 1);
		glUniform1i(differenceShaderTex2Location, 2);
		glUniform1fv(differenceShaderAabbLocation, 4, screenDifferenceAabb);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		double averageDifferenceAfter = getAverageDifference(0, 0, w, h);

		if (averageDifferenceAfter <= averageDifferenceBefore) {
			*last = before;
			//averageDifferenceAfter = averageDifferenceBefore;
		}

		//init imgui frame render frame==================================================================
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindFramebuffer(GL_FRAMEBUFFER, showDifference ? boundingBoxBuffer.framebuffer : 0);
		glViewport(0, 0, 640, 480);
		if (showDifference) {
			boundingBoxBuffer.resize(640, 480);
			glClear(GL_COLOR_BUFFER_BIT);

			triangleShader.use();

			glm::mat4 proj = glm::ortho(0.f, 640.f, 0.f, 480.f, -1.f, 1.f);

			glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1i(triangleShaderTexLocation, 0);
			for (Sprite& sprite : sprites) {
				glm::mat4 model = getSpriteModelMat(&sprite);
				glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}


			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			differenceShader.use();

			glUniformMatrix4fv(differenceShaderProjLocation, 1, GL_FALSE, glm::value_ptr(fullscreenProj));
			glUniformMatrix4fv(differenceShaderModelLocation, 1, GL_FALSE, glm::value_ptr(identity));
			glUniform1i(differenceShaderTex1Location, 1);
			glUniform1i(differenceShaderTex2Location, 2);
			float uv[4] = {0.f, 1.f, 0.f, 1.f};
			glUniform1fv(differenceShaderAabbLocation, 4, uv);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		} else {
			glClear(GL_COLOR_BUFFER_BIT);

			triangleShader.use();

			glm::mat4 proj = glm::ortho(0.f, 640.f, 0.f, 480.f, -1.f, 1.f);

			glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1i(triangleShaderTexLocation, 0);
			for (Sprite& sprite : sprites) {
				glm::mat4 model = getSpriteModelMat(&sprite);
				glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}


		ImGui::Begin("Reduction");
		ImGui::Text("%d FPS", fps);
		ImGui::Text("sprites: %d", (int)sprites.size());
		if (ImGui::Button("add")) {
			sprites.push_back({randFloat() * 640.f, randFloat() * 480.f, randFloat() * 100.f, randFloat() * 100.f, randFloat() * 3.14159f * 2.f});
		}
		if (ImGui::Button("show difference")) showDifference = !showDifference;
		ImGui::Text("average differenceb: %f", (float)averageDifferenceBefore);
		ImGui::Text("average differencea: %f", (float)averageDifferenceAfter);
		ImGui::Text("wh %d %d", w, h);
		if (ImGui::Button("save")) save = true;
		//ImGui::Text("%f %f %f %f", aabb[0], aabb[1], aabb[2], aabb[3]);
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
