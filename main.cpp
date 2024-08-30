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
};
class TriangleShader : public Shader {
public:
	unsigned int modelLocation, projLocation, texLocation, colLocation;
	TriangleShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		texLocation = glGetUniformLocation(ID, "tex");
		colLocation = glGetUniformLocation(ID, "col");
	}
};
class DifferenceShader : public Shader {
public:
	unsigned int modelLocation, projLocation, tex1Location, tex2Location, aabbLocation;
	DifferenceShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		tex1Location = glGetUniformLocation(ID, "tex1");
		tex2Location = glGetUniformLocation(ID, "tex2");
		aabbLocation = glGetUniformLocation(ID, "aabb");
	}
};
class OutlineShader : public Shader {
public:
	unsigned int modelLocation, projLocation, colLocation;
	OutlineShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		colLocation = glGetUniformLocation(ID, "col");
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
float clamp(float a, float lo, float hi) {
	return min(max(a, lo), hi);
}
struct Sprite {
	float x, y, width, height, rot, r, g, b; // x y center
	int texId;
	double difference = -1.;
	float* aabb;
	Sprite(float x_, float y_, float width_, float height_, float rot_, float r_, float g_, float b_) {
		x = x_;
		y = y_;
		width = width_;
		height = height_;
		rot = rot_;
		r = r_;
		g = g_;
		b = b_;
		texId = rand();
		resetBoundingBox();
	}
	Sprite() {
		x = randFloat() * (float)WIDTH;
		y = randFloat() * (float)HEIGHT;
		width = randFloat() * 100.f;
		height = randFloat() * 100.f;
		rot = randFloat() * 3.14159f * 2.f;
		r = randFloat() * 3.f;
		g = randFloat() * 3.f;
		b = randFloat() * 3.f;
		texId = rand();
		resetBoundingBox();
	}
	glm::mat4 getModelMat() {
		glm::mat4 model = glm::mat4(1.f);
		model = glm::translate(model, glm::vec3(x, y, 0.f));
		model = glm::rotate(model, rot, glm::vec3(0.f, 0.f, -1.f));
		model = glm::scale(model, glm::vec3(width, height, 1.f));
		return model;
	}
	void resetBoundingBox() {
		glm::vec4 points[4] = {
			glm::vec4(vertices[0], vertices[1], 0.f, 1.f),
			glm::vec4(vertices[4], vertices[5], 0.f, 1.f),
			glm::vec4(vertices[8], vertices[9], 0.f, 1.f),
			glm::vec4(vertices[12], vertices[13], 0.f, 1.f)
		};
		glm::mat4 focusModelMat = getModelMat();
		for (int i = 0; i < 4; i++) {
			points[i] = focusModelMat * points[i];
		}
		aabb = (float*)malloc(sizeof(float) * 4);
		aabb[0] = min(min(points[0].x, points[1].x), min(points[2].x, points[3].x)); // minx
		aabb[1] = max(max(points[0].x, points[1].x), max(points[2].x, points[3].x)); // maxx
		aabb[2] = min(min(points[0].y, points[1].y), min(points[2].y, points[3].y)); // miny
		aabb[3] = max(max(points[0].y, points[1].y), max(points[2].y, points[3].y)); // maxy
	}
	void randomlyTransform() {
		bool add = rand() % 2;
		float r = randFloat();
		float r2 = randFloat();
		const static float mwh = (float)max(WIDTH, HEIGHT);
		switch (rand() % 7) {
		case 0:
			x = add ? clamp(x + r * 50.f - 25.f, 0.f, (float)WIDTH) : (r * (float)WIDTH);
			y = add ? clamp(y + r2 * 50.f - 25.f, 0.f, (float)HEIGHT) : (r2 * (float)HEIGHT);
			resetBoundingBox();
			break;
		case 1:
			width = add ? clamp(width + r * 30.f - 15.f, 10.f, mwh) : (r * 90.f + 10.f);
			height = add ? clamp(height + r2 * 30.f - 15.f, 10.f, mwh) : (r2 * 90.f + 10.f);
			resetBoundingBox();
			break;
		case 2:
			rot = add ? rot + r * 0.2f - 0.1f : r * 6.28f; //randFloat() * 0.2f - 0.1f;
			resetBoundingBox();
			break;
		case 3:
			r = r * 3.f;
			break;
		case 4:
			g = r * 3.f;
			break;
		case 5:
			b = r * 3.f;
			break;
		case 6:
			texId = rand();
			break;
		}
	}
};
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

vector<Sprite> sprites;

void renderSprites(TriangleShader* triangleShader, float* aabb, int howManySpriteTextures, glm::mat4* aabbProj, int endI) {
	glClear(GL_COLOR_BUFFER_BIT);

	triangleShader->use();

	glUniformMatrix4fv(triangleShader->projLocation, 1, GL_FALSE, glm::value_ptr(*aabbProj));
	int i = 0;
	for (Sprite& sprite : sprites) {
		if (i > endI && endI != -1) break;
		i++;
		if (aabb) {
			if (!aabbIntersect(sprite.aabb, aabb)) continue;
		}
		glm::mat4 model = sprite.getModelMat();
		glUniformMatrix4fv(triangleShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3f(triangleShader->colLocation, sprite.r, sprite.g, sprite.b);
		glUniform1i(triangleShader->texLocation, (sprite.texId % howManySpriteTextures) + 2);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}
void renderDifference(DifferenceShader* differenceShader, unsigned int spritesTexture, float* screenSpaceDifferenceAabb) {
	const static glm::mat4 identity = glm::mat4(1.f);
	const static glm::mat4 fullscreenProj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);

	glClear(GL_COLOR_BUFFER_BIT);
	differenceShader->use();

	glUniformMatrix4fv(differenceShader->projLocation, 1, GL_FALSE, glm::value_ptr(fullscreenProj));
	glUniformMatrix4fv(differenceShader->modelLocation, 1, GL_FALSE, glm::value_ptr(identity));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, spritesTexture);
	glUniform1i(differenceShader->tex1Location, 0);
	glUniform1i(differenceShader->tex2Location, 1);
	glUniform1fv(differenceShader->aabbLocation, 4, screenSpaceDifferenceAabb);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

	TriangleShader triangleShader{"vertex.vsh", "fragment.fsh"};
	DifferenceShader differenceShader{"vertex.vsh", "difference.fsh"};
	OutlineShader outlineShader{"vertex.vsh", "outline.fsh"};

	Texture spriteTextures[] = {
		{"square.png"},
		{"circle.png"}
		/*{"807/DSC00148.jpg"},
		{"807/DSC00149.jpg"},
		{"807/DSC00150.jpg"},
		{"807/DSC00151.jpg"},
		{"807/DSC00152.jpg"},
		{"807/DSC00153.jpg"},
		{"807/DSC00154.jpg"},
		{"807/DSC00155.jpg"},
		{"807/DSC00156.jpg"},
		{"807/DSC00157.jpg"},
		{"807/DSC00158.jpg"},
		{"807/DSC00159.jpg"},
		{"807/DSC00160.jpg"},
		{"807/DSC00161.jpg"},
		{"807/DSC00163.jpg"},
		{"807/DSC00164.jpg"},
		{"807/DSC00166.jpg"},
		{"807/DSC00167.jpg"},
		{"807/DSC00168.jpg"},
		{"807/DSC00169.jpg"},
		{"807/DSC00174.jpg"},
		{"807/DSC00175.jpg"},
		{"807/DSC00176.jpg"}*/
	};
	int howManySpriteTextures = sizeof(spriteTextures) / sizeof(Texture);
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

	bool showDifference = false;

	int frameCount = 0;
	int fps = 0;
	double lastFpsFrameTime = glfwGetTime();
	for (int i = 0; i < 10; i++) {
		sprites.push_back({});
	}

	glClearColor(0.f, 0.f, 0.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, targetTexture.id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);
	for (int i = 0; i < howManySpriteTextures; i++) {
		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, spriteTextures[i].id);
	}

	glBindVertexArray(VAO);
	int iterations = 0;
	bool ignoreDifference = false;
	bool playing = false;
	//llooop
	while (!glfwWindowShouldClose(window)) {
		Sprite* current;
		if (playing) {
			//find one to edit=====================================================
			unsigned int largestDifferenceIndex = 0U;
			double largestDifference = 0.;
			for (unsigned int i = 0; i < sprites.size(); i++) {
				if (sprites.at(i).difference >= 0. && false) {
					if (sprites.at(i).difference > largestDifference) {
						largestDifference = sprites.at(i).difference;
						largestDifferenceIndex = i;
					}
				} else {
					float* aabb = sprites.at(i).aabb;
					int w = (int)(aabb[1] - aabb[0]);
					int h = (int)(aabb[3] - aabb[2]);
					glActiveTexture(GL_TEXTURE1);
					boundingBoxBuffer.resize(w, h);
					differenceBuffer.resize(w, h);
					glViewport(0, 0, w, h);
					glm::mat4 aabbProj = glm::ortho(aabb[0], aabb[1], aabb[2], aabb[3], -1.f, 1.f);
					float* screenSpaceAabb = (float*)malloc(sizeof(float) * 4);
					screenSpaceAabb[0] = aabb[0] / (float)WIDTH;
					screenSpaceAabb[1] = aabb[1] / (float)WIDTH;
					screenSpaceAabb[2] = aabb[2] / (float)HEIGHT;
					screenSpaceAabb[3] = aabb[3] / (float)HEIGHT;

					// render=====
					glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
					renderSprites(&triangleShader, aabb, howManySpriteTextures, &aabbProj, i);
					// get difference===
					glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
					renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceAabb);

					sprites.at(i).difference = getAverageBufferColor(0, 0, w, h, 3);
					if (sprites.at(i).difference > largestDifference) {
						largestDifference = sprites.at(i).difference;
						largestDifferenceIndex = i;
						if (save) cout << "highest diff: " << i << endl;
					}
					if (save) {
						GLsizei stride = w * 3;
						GLsizei bufferSize = stride * h;

						glPixelStorei(GL_PACK_ALIGNMENT, 1);
						glReadBuffer(GL_BACK);
						stbi_flip_vertically_on_write(true);
						unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
						glReadnPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba
						string fileName = "dif" + to_string(i) + ".png";
						stbi_write_png(fileName.c_str(), w, h, 3, bufferData, stride);
						cout << "saved " << fileName << endl;
						free(bufferData);
					}
				}
			}
			save = false;
			current = &sprites.at(largestDifferenceIndex);//&sprites.at(randIntRange(0, sprites.size()));//&sprites.at(randIntRange(max(0, sprites.size() - 10), sprites.size()));

			Sprite after = *current;
			after.randomlyTransform();
			float* aabbBefore = current->aabb;
			float* aabbAfter = after.aabb;
			float* differenceAabb = getDoubleBoundingBoxBoundingBox(aabbBefore, aabbAfter);
			float* screenSpaceDifferenceAabb = (float*)malloc(sizeof(float) * 4);
			screenSpaceDifferenceAabb[0] = differenceAabb[0] / (float)WIDTH;
			screenSpaceDifferenceAabb[1] = differenceAabb[1] / (float)WIDTH;
			screenSpaceDifferenceAabb[2] = differenceAabb[2] / (float)HEIGHT;
			screenSpaceDifferenceAabb[3] = differenceAabb[3] / (float)HEIGHT;

			glm::mat4 aabbProj = glm::ortho(differenceAabb[0], differenceAabb[1], differenceAabb[2], differenceAabb[3], -1.f, 1.f);

			int w = (int)(differenceAabb[1] - differenceAabb[0]);
			int h = (int)(differenceAabb[3] - differenceAabb[2]);

			//resize
			glActiveTexture(GL_TEXTURE1);
			boundingBoxBuffer.resize(w, h);
			differenceBuffer.resize(w, h);
			glViewport(0, 0, w, h);

			// render before transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderSprites(&triangleShader, differenceAabb, howManySpriteTextures, &aabbProj, -1);
			// get difference==================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceDifferenceAabb);

			double averageDifferenceBefore = getAverageBufferColor(0, 0, w, h, 3);

			Sprite before = *current;
			*current = after;

			// render after transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderSprites(&triangleShader, differenceAabb, howManySpriteTextures, &aabbProj, -1);
			// get difference==================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceDifferenceAabb);

			double averageDifferenceAfter = getAverageBufferColor(0, 0, w, h, 3);

			if (averageDifferenceAfter > averageDifferenceBefore && !ignoreDifference) {
				*current = before;
				averageDifferenceAfter = averageDifferenceBefore;
			} else {
				for (Sprite& sprite : sprites) {
					if (aabbIntersect(sprite.aabb, aabbBefore) || aabbIntersect(sprite.aabb, aabbAfter)) {
						sprite.difference = -1.;
					}
				}
			}
		}

		//init imgui frame render frame==================================================================
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindFramebuffer(GL_FRAMEBUFFER, showDifference ? boundingBoxBuffer.framebuffer : 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		double difference = -1.;
		glm::mat4 proj = glm::ortho(0.f, (float)WIDTH, 0.f, (float)HEIGHT, -1.f, 1.f);
		if (showDifference) {
			boundingBoxBuffer.resize(WIDTH, HEIGHT);
			renderSprites(&triangleShader, nullptr, howManySpriteTextures, &proj, -1);

			float uv[4] = {0.f, 1.f, 0.f, 1.f};

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, uv);

			difference = getAverageBufferColor(0, 0, WIDTH, HEIGHT, 4);
		} else {
			renderSprites(&triangleShader, nullptr, howManySpriteTextures, &proj, -1);
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

		if (playing) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			outlineShader.use();

			glUniformMatrix4fv(outlineShader.projLocation, 1, GL_FALSE, glm::value_ptr(proj));

			glm::mat4 outlineModel = current->getModelMat();
			glUniformMatrix4fv(outlineShader.modelLocation, 1, GL_FALSE, glm::value_ptr(outlineModel));
			glUniform3f(outlineShader.colLocation, 1.f, 1.f, 0.2f);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}



		ImGui::Begin("Reduction");
		ImGui::Text("%d FPS", fps);
		ImGui::Checkbox("playing", &playing);
		ImGui::Text("sprites: %d", (int)sprites.size());
		static int howmany = 1;
		if (ImGui::Button("add")) {
			sprites.reserve(howmany);
			for (int i = 0; i < howmany; i++) {
				sprites.push_back({});
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
