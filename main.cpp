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

float lerp(float a, float b, float t) {
	return (b - a) * t + a;
}
int randIntRange(int mn, int mx) {// excluding end
	return rand() % (mx - mn) + mn;
}

class Texture {
public:
	int width, height, numChannels;
	unsigned int id;
	Texture(const char* path) {
		stbi_set_flip_vertically_on_load(true);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		float borderColor[4] = {0.f, 0.f, 0.f, 1.f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_NV, borderColor);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_NV);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_NV);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		unsigned char *data = stbi_load(path, &width, &height, &numChannels, 0);
		cout << "tex has " << numChannels << " color channels" << endl;
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, numChannels == 3 ? GL_RGB : GL_RGBA, width, height, 0, numChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
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
int WIDTH, HEIGHT;
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
	WIDTH = w;
	HEIGHT = h;
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
	float x, y, width, height, rot, r, g, b; // x y center
};
glm::mat4 getSpriteModelMat(Sprite* sprite) {
	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(sprite->x, sprite->y, 0.f));
	model = glm::rotate(model, sprite->rot, glm::vec3(0.f, 0.f, -1.f));
	model = glm::scale(model, glm::vec3(sprite->width, sprite->height, 1.f));
	return model;
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
bool aabbIntersect(float* a, float* b) {
	return a[1] > b[0] && a[0] < b[1] && a[3] > b[2] && a[2] < b[3];
}
float clamp(float a, float lo, float hi) {
	return min(max(a, lo), hi);
}
void randomlyTransformSprite(Sprite* sprite) {
	bool add = rand() % 2;
	float r = randFloat();
	float r2 = randFloat();
	switch (rand() % 6) {
	case 0:
		sprite->x = add ? clamp(sprite->x + r * 50.f - 25.f, 0.f, (float)WIDTH) : (r * (float)WIDTH);
		sprite->y = add ? clamp(sprite->y + r2 * 50.f - 25.f, 0.f, (float)HEIGHT) : (r2 * (float)HEIGHT);
		break;
	case 1:
		sprite->width = add ? max(sprite->width + r * 30.f - 15.f, 10.f) : (r * 90.f + 10.f);
		sprite->height = add ? max(sprite->height + r2 * 30.f - 15.f, 10.f) : (r2 * 90.f + 10.f);
		break;
	case 2:
		sprite->rot = add ? sprite->rot + r * 0.2f - 0.1f : r * 6.28f; //randFloat() * 0.2f - 0.1f;
		break;
	case 3:
		sprite->r = r * 3.f;
		break;
	case 4:
		sprite->g = r * 3.f;
		break;
	case 5:
		sprite->b = r * 3.f;
		break;
	}
}
float getAabbArea(float* aabb) {
	return abs((aabb[1] - aabb[0]) * (aabb[3] - aabb[2]));
}
bool save = false;
double getAverageBufferColor(int x, int y, int w, int h, int step) {
	GLsizei stride = w * 3;
	GLsizei bufferSize = stride * h;

	glReadBuffer(GL_BACK);
	stbi_flip_vertically_on_write(true);
	unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
	glReadnPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba

	double average = 0.;
	for (int ax = 0; ax < w; ax += step) {
		for (int ay = 0; ay < h; ay += step) {
			int n = 3 * (ay * w + ax);
			average += (double)bufferData[n] + (double)bufferData[n + 1] + (double)bufferData[n + 2];
		}
	}
	average /= (double)(w * h / (step * step) * 3 * 256);
	/*if (save) {
		glReadBuffer(GL_BACK);
		stbi_flip_vertically_on_write(true);
		glReadnPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba
		stbi_write_png("diff.png", w, h, 3, bufferData, stride);
		save = false;
	}*/
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	Shader triangleShader{"vertex.vsh", "fragment.fsh"};
	unsigned int triangleShaderModelLocation = glGetUniformLocation(triangleShader.ID, "modelMat");
	unsigned int triangleShaderProjLocation = glGetUniformLocation(triangleShader.ID, "projMat");
	unsigned int triangleShaderTexLocation = glGetUniformLocation(triangleShader.ID, "tex");
	unsigned int triangleShaderColLocation = glGetUniformLocation(triangleShader.ID, "col");

	Shader differenceShader{"vertex.vsh", "difference.fsh"};
	unsigned int differenceShaderModelLocation = glGetUniformLocation(differenceShader.ID, "modelMat");
	unsigned int differenceShaderProjLocation = glGetUniformLocation(differenceShader.ID, "projMat");
	unsigned int differenceShaderTex1Location = glGetUniformLocation(differenceShader.ID, "tex1");
	unsigned int differenceShaderTex2Location = glGetUniformLocation(differenceShader.ID, "tex2");
	unsigned int differenceShaderAabbLocation = glGetUniformLocation(differenceShader.ID, "aabb");

	Texture skateboardTexture{"sprite.png"};
	Texture targetTexture{"target.png"};

	WIDTH = targetTexture.width;
	HEIGHT = targetTexture.height;
	glfwSetWindowSize(window, WIDTH, HEIGHT);

	FrameBuffer boundingBoxBuffer{WIDTH, HEIGHT};
	FrameBuffer differenceBuffer{WIDTH, HEIGHT};

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
	for (int i = 0; i < 100; i++) {
		sprites.push_back({randFloat() * (float)WIDTH, randFloat() * (float)HEIGHT, randFloat() * 100.f, randFloat() * 100.f, randFloat() * 3.14159f * 3.f, randFloat(), randFloat() * 3.f, randFloat() * 3.f});
	}

	glClearColor(0.f, 0.f, 0.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skateboardTexture.id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, targetTexture.id);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);

	glBindVertexArray(VAO);
	int iterations = 0;
	bool ignoreDifference = false;
	//llooop
	while (!glfwWindowShouldClose(window)) {
		int hm = sprites.size();//howmany
		Sprite* current = &sprites.at(randIntRange(0, hm));//&sprites.at(randIntRange(max(0, hm - 10), hm));
		Sprite after = *current;
		randomlyTransformSprite(&after);
		float* aabbBefore = getSpriteBoundingBox(current);
		float* aabbAfter = getSpriteBoundingBox(&after);
		float* differenceAabb = getDoubleBoundingBoxBoundingBox(aabbBefore, aabbAfter);
		float* screenDifferenceAabb = (float*)malloc(sizeof(float) * 4);
		screenDifferenceAabb[0] = differenceAabb[0] / (float)WIDTH;
		screenDifferenceAabb[1] = differenceAabb[1] / (float)WIDTH;
		screenDifferenceAabb[2] = differenceAabb[2] / (float)HEIGHT;
		screenDifferenceAabb[3] = differenceAabb[3] / (float)HEIGHT;

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
			float* aabb = getSpriteBoundingBox(&sprite);
			if (!aabbIntersect(aabb, differenceAabb)) continue;
			glm::mat4 model = getSpriteModelMat(&sprite);
			glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(triangleShaderColLocation, sprite.r, sprite.g, sprite.b);
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

		double averageDifferenceBefore = getAverageBufferColor(0, 0, w, h, 3);


		Sprite before = *current;
		*current = after;
		// render after transform===================================================================================================================
		glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		triangleShader.use();

		glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(aabbProj));
		glUniform1i(triangleShaderTexLocation, 0);
		for (Sprite& sprite : sprites) {
			float* aabb = getSpriteBoundingBox(&sprite);
			if (!aabbIntersect(aabb, differenceAabb)) continue;
			glm::mat4 model = getSpriteModelMat(&sprite);
			glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3f(triangleShaderColLocation, sprite.r, sprite.g, sprite.b);
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

		double averageDifferenceAfter = getAverageBufferColor(0, 0, w, h, 3);

		if (averageDifferenceAfter > averageDifferenceBefore && !ignoreDifference) {
			*current = before;
			averageDifferenceAfter = averageDifferenceBefore;
		}

		//init imgui frame render frame==================================================================
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindFramebuffer(GL_FRAMEBUFFER, showDifference ? boundingBoxBuffer.framebuffer : 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		double difference = -1.;
		if (showDifference) {
			boundingBoxBuffer.resize(WIDTH, HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT);

			triangleShader.use();

			glm::mat4 proj = glm::ortho(0.f, (float)WIDTH, 0.f, (float)HEIGHT, -1.f, 1.f);

			glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1i(triangleShaderTexLocation, 0);
			for (Sprite& sprite : sprites) {
				glm::mat4 model = getSpriteModelMat(&sprite);
				glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3f(triangleShaderColLocation, sprite.r, sprite.g, sprite.b);
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
			difference = getAverageBufferColor(0, 0, WIDTH, HEIGHT, 4);
		} else {
			glClear(GL_COLOR_BUFFER_BIT);

			triangleShader.use();

			glm::mat4 proj = glm::ortho(0.f, (float)WIDTH, 0.f, (float)HEIGHT, -1.f, 1.f);

			glUniformMatrix4fv(triangleShaderProjLocation, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1i(triangleShaderTexLocation, 0);
			for (Sprite& sprite : sprites) {
				glm::mat4 model = getSpriteModelMat(&sprite);
				glUniformMatrix4fv(triangleShaderModelLocation, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3f(triangleShaderColLocation, sprite.r, sprite.g, sprite.b);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			if (save) {
				GLsizei stride = WIDTH * 3;
				GLsizei bufferSize = stride * HEIGHT;

				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadBuffer(GL_BACK);
				stbi_flip_vertically_on_write(true);
				unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
				glReadnPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba
				stbi_write_png("output.png", WIDTH, HEIGHT, 3, bufferData, stride);
				save = false;
				free(bufferData);
			}
		}


		ImGui::Begin("Reduction");
		ImGui::Text("%d FPS", fps);
		ImGui::Text("sprites: %d", (int)sprites.size());
		static int howmany = 1;
		if (ImGui::Button("add")) {
			for (int i = 0; i < howmany; i++) {
				sprites.push_back({randFloat() * (float)WIDTH, randFloat() * (float)HEIGHT, randFloat() * 100.f, randFloat() * 100.f, randFloat() * 3.14159f * 3.f, randFloat(), randFloat() * 3.f, randFloat() * 3.f});
			}
		}
		ImGui::SameLine();
		ImGui::SliderInt("howmany", &howmany, 1, 100);
		if (ImGui::Button("show difference")) showDifference = !showDifference;
		ImGui::Text("average difference: %f", (float)difference);
		iterations++;
		ImGui::Text("iters: %d", iterations);
		if (ImGui::Button("save")) save = true;
		ImGui::Checkbox("ignore difference", &ignoreDifference);
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
