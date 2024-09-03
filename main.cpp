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
float square(float x) {
	return x * x;
}

class Texture {
public:
	int width, height, numChannels;
	unsigned int id;
	Texture(const char* path) {
		stbi_set_flip_vertically_on_load(true);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		float borderColor[4] = {0.f, 0.f, 0.f, 0.f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_NV, borderColor);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_NV);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_NV);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


		unsigned char *data = stbi_load(path, &width, &height, &numChannels, 0);
		cout << "tex has " << numChannels << " color channels" << endl;
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, numChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
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
	unsigned int modelLocation, projLocation, texLocation, colLocation, aLocation, bLocation, cLocation, dLocation;
	TriangleShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		texLocation = glGetUniformLocation(ID, "tex");
		colLocation = glGetUniformLocation(ID, "col");
		aLocation = glGetUniformLocation(ID, "a");
		bLocation = glGetUniformLocation(ID, "b");
		cLocation = glGetUniformLocation(ID, "c");
		dLocation = glGetUniformLocation(ID, "d");
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
	unsigned int modelLocation, projLocation, colLocation, sizeLocation;
	OutlineShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		colLocation = glGetUniformLocation(ID, "col");
		sizeLocation = glGetUniformLocation(ID, "size");
	}
};
class LensShader : public Shader {
public:
	unsigned int modelLocation, projLocation, colLocation, texLocation;
	LensShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		projLocation = glGetUniformLocation(ID, "projMat");
		texLocation = glGetUniformLocation(ID, "tex");
		colLocation = glGetUniformLocation(ID, "col");
	}
};
class ColorShader : public Shader {
public:
	unsigned int modelLocation, projLocation, colLocation;
	ColorShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
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

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);

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

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);

		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
};
int WIDTH = 640, HEIGHT = 480;
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
	bool w, a, s, d, q, e, right, left, up, down, didMouseClick;
	double mouseX, mouseY, scroll;
} controls;
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
}
double mouseX = 0., mouseY = 0.;
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	mouseX = xpos, mouseY = (double)HEIGHT - ypos;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	controls.scroll = yoffset;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
		return;
	}
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		controls.didMouseClick = true;
	}
}
float clamp(float a, float lo, float hi) {
	return min(max(a, lo), hi);
}
struct AABB {
	float l, r, b, t;
};
bool only = false;
struct Raster {
	float x, y, width, height, rot, r, g, b; // x y center
	int texId;
	double difference = -1.;
	AABB aabb;
	Raster(bool randomize) {
		if (randomize) {
			x = randFloat();
			y = randFloat();
			width = randFloat() * 100.f;
			height = randFloat() * 100.f;
			rot = randFloat() * 3.14159f * 2.f;
			r = randFloat() * 3.f;
			g = randFloat() * 3.f;
			b = randFloat() * 3.f;
			texId = rand();
		} else {
			x = 0.5f;
			y = 0.5f;
			width = 1.f;
			height = 1.f;
			rot = 0.f;
			r = 1.f;
			g = 1.f;
			b = 1.f;
			texId = 0;
		}
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
		aabb = {
			max(min(min(points[0].x, points[1].x), min(points[2].x, points[3].x)), 0.f),
			min(max(max(points[0].x, points[1].x), max(points[2].x, points[3].x)), 1.f),
			max(min(min(points[0].y, points[1].y), min(points[2].y, points[3].y)), 0.f),
			min(max(max(points[0].y, points[1].y), max(points[2].y, points[3].y)), 1.f)
		};
	}
	void randomlyTransform() {
		bool add = rand() % 2;
		float r1 = randFloat();
		float r2 = randFloat();
		switch (only ? rand() % 3 + 3 : rand() % 7) {
		case 0:
			x = add ? clamp(x + r1 * 50.f - 25.f, 0.f, 1.f) : (r1 * 1.f);
			y = add ? clamp(y + r2 * 50.f - 25.f, 0.f, 1.f) : (r2 * 1.f);
			resetBoundingBox();
			break;
		case 1:
			width = add ? clamp(width + r1 * 30.f - 15.f, 0.1f, 1.f) : (r1 * 90.f + 10.f);
			height = add ? clamp(height + r2 * 30.f - 15.f, 0.1f, 1.f) : (r2 * 90.f + 10.f);
			resetBoundingBox();
			break;
		case 2:
			rot = add ? rot + r1 * 0.2f - 0.1f : r1 * 6.28f; //randFloat() * 0.2f - 0.1f;
			resetBoundingBox();
			break;
		case 3:
			r = r1 * 3.f;
			break;
		case 4:
			g = r1 * 3.f;
			break;
		case 5:
			b = r1 * 3.f;
			break;
		case 6:
			texId = rand();
			break;
		}
	}
};
AABB getDoubleBoundingBoxBoundingBox(AABB aabb1, AABB aabb2) {
	return {
		min(aabb1.l, aabb2.l),
		max(aabb1.r, aabb2.r),
		min(aabb1.b, aabb2.b),
		max(aabb1.t, aabb2.t)
	};
}
bool aabbIntersect(AABB a, AABB b) {
	return a.r > b.l && a.l < b.r && a.t > b.b && a.b < b.t;
}
float getAabbArea(AABB aabb) {
	return abs((aabb.r - aabb.l) * (aabb.t - aabb.b));
}
bool save = false;
double getAverageBufferColor(int x, int y, int w, int h, int step) {
	GLsizei stride = w * 4;
	GLsizei bufferSize = stride * h;

	glReadBuffer(GL_BACK);
	stbi_flip_vertically_on_write(true);
	unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
	glReadnPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bufferSize, bufferData); // maybe change to rgba

	double average = 0.;
	for (int ax = 0; ax < w; ax += step) {
		for (int ay = 0; ay < h; ay += step) {
			int n = 3 * (ay * w + ax);
			average += (double)bufferData[n] + (double)bufferData[n + 1] + (double)bufferData[n + 2];
		}
	}
	average /= (double)(w * h / (step * step) * 4 * 256);
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

vector<Raster> rasters;
float a = 0.f, b = 0.f, c = 0.f, d = 1.f;

void renderRasters(TriangleShader* triangleShader, AABB aabb, int howManyRasterTextures, glm::mat4* aabbProj, int endI) {
	glClear(GL_COLOR_BUFFER_BIT);

	triangleShader->use();

	glUniformMatrix4fv(triangleShader->projLocation, 1, GL_FALSE, glm::value_ptr(*aabbProj));
	int i = 0;
	for (Raster& raster : rasters) {
		if (i > endI && endI != -1) break;
		i++;
		//if (!aabbIntersect(raster.aabb, aabb)) continue;
		glm::mat4 model = raster.getModelMat();
		glUniformMatrix4fv(triangleShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3f(triangleShader->colLocation, raster.r, raster.g, raster.b);
		glUniform1i(triangleShader->texLocation, (raster.texId % howManyRasterTextures) + 2);
		glUniform1f(triangleShader->aLocation, a);
		glUniform1f(triangleShader->bLocation, b);
		glUniform1f(triangleShader->cLocation, c);
		glUniform1f(triangleShader->dLocation, d);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}
const glm::mat4 identity = glm::mat4(1.f);
const glm::mat4 fullscreenProj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);
void renderDifference(DifferenceShader* differenceShader, unsigned int rastersTexture, AABB screenSpaceDifferenceAabb) {
	glClear(GL_COLOR_BUFFER_BIT);
	differenceShader->use();

	glUniformMatrix4fv(differenceShader->projLocation, 1, GL_FALSE, glm::value_ptr(fullscreenProj));
	glUniformMatrix4fv(differenceShader->modelLocation, 1, GL_FALSE, glm::value_ptr(identity));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rastersTexture);
	glUniform1i(differenceShader->tex1Location, 0);
	glUniform1i(differenceShader->tex2Location, 1);
	float* ssdba = reinterpret_cast<float*>(&screenSpaceDifferenceAabb);
	glUniform1fv(differenceShader->aabbLocation, 4, ssdba);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
struct Line {
	float x1, y1, x2, y2;
};
float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * glm::pow(r, 3.f) + b * glm::pow(r, 2.f) + c * r + d) * r;
}
float inverseLensDistortion(float r, float a, float b, float c, float d) {
	float answer = 0.f;

	float start = 0.f;
	float end = 10.f;

	for (int i = 0; i < 32; i++) {
		float mid = (start + end) * 0.5f;
		float l = lensDistortion(mid, a, b, c, d);
		answer = mid;

		if (l < r) {
			start = mid;
			answer = mid;
		} else {
			end = mid;
		}
	}

	return answer;
}
void renderLine(Line line, ColorShader* colorShader, AABB viewAabb) {
	const float width = 1.f;
	colorShader->use();

	glm::mat4 proj = glm::ortho(viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t, -1.f, 1.f);
	glUniformMatrix4fv(colorShader->projLocation, 1, GL_FALSE, glm::value_ptr(proj));

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3((line.x1 + line.x2) / 2.f, (line.y1 + line.y2) / 2.f, 0.f));
	model = glm::rotate(model, atan((line.x2 - line.x1) / (line.y2 - line.y1)), glm::vec3(0.f, 0.f, -1.f));
	float length = sqrt(square(line.x2 - line.x1) + square(line.y2 - line.y1));
	model = glm::scale(model, glm::vec3(width, length, 1.f));

	glUniformMatrix4fv(colorShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(colorShader->colLocation, 1.f, 1.f, 1.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
int main(void) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	//init glfw
	GLFWwindow* window = glfwCreateWindow(640, 480, "Reduction", NULL, NULL);
	if (window == NULL) {
		cout << "[ERROR] Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

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
	glEnable(GL_MULTISAMPLES_NV);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	TriangleShader triangleShader{"shaders/raster.vsh", "shaders/fragment.fsh"};
	DifferenceShader differenceShader{"shaders/vertex.vsh", "shaders/difference.fsh"};
	OutlineShader outlineShader{"shaders/vertex.vsh", "shaders/outline.fsh"};
	ColorShader colorShader{"shaders/vertex.vsh", "shaders/color.fsh"};

	Texture rasterTextures[] = {
		{"807/DSC00161.jpg"}
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
	int howManyRasterTextures = sizeof(rasterTextures) / sizeof(Texture);
	Texture targetTexture{"target.png"};

	WIDTH = targetTexture.width;
	HEIGHT = targetTexture.height;
	glfwSetWindowSize(window, WIDTH, HEIGHT);

	FrameBuffer boundingBoxBuffer{WIDTH, HEIGHT};
	FrameBuffer differenceBuffer{WIDTH, HEIGHT};
	FrameBuffer renderBuffer(WIDTH, HEIGHT);

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
	for (int i = 0; i < 1; i++) {
		rasters.push_back({false});
	}

	glClearColor(0.f, 0.f, 0.f, 0.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, targetTexture.id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, boundingBoxBuffer.textureColorBuffer);
	for (int i = 0; i < howManyRasterTextures; i++) {
		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, rasterTextures[i].id);
	}

	glBindVertexArray(VAO);

	vector<Line> lines;
	int addingLineMode = 0; // 0 nothing 1 add first point 2 add 2nd point
	float firstPointX = 0.f;
	float firstPointY = 0.f;

	int iterations = 0;
	bool ignoreDifference = false;
	bool playing = false;
	bool isRandom = false;

	AABB viewAabb = {0.f, 1.f, 0.f, 1.f};
	//llooop
	while (!glfwWindowShouldClose(window)) {
		controls.mouseX = mouseX * (double)(viewAabb.r - viewAabb.l) + viewAabb.l;
		controls.mouseY = mouseY * (double)(viewAabb.t - viewAabb.b) + viewAabb.b;

		viewAabb.l = lerp(viewAabb.l, controls.mouseX, controls.scroll * 0.05f);
		viewAabb.r = lerp(viewAabb.r, controls.mouseX, controls.scroll * 0.05f);
		viewAabb.b = lerp(viewAabb.b, controls.mouseY, controls.scroll * 0.05f);
		viewAabb.t = lerp(viewAabb.t, controls.mouseY, controls.scroll * 0.05f);
		controls.scroll = 0.;

		if (controls.didMouseClick) {
			controls.didMouseClick = false;
			if (addingLineMode == 1) {
				firstPointX = controls.mouseX;
				firstPointY = controls.mouseY;
				addingLineMode = 2;
			} else if (addingLineMode == 2) {
				float x = controls.mouseX;
				float y = controls.mouseY;
				lines.push_back({firstPointX, firstPointY, x, y});
				addingLineMode = 0;
			}
		}

		Raster* current;
		bool didSucceedMove = true;
		int index = -1;
		double dif = -1.;
		if (playing) {
			//find one to edit=====================================================
			if (isRandom) {
				index = randIntRange(0, rasters.size());
			} else {
				unsigned int largestDifferenceIndex = 0U;
				double largestDifference = 0.;
				for (unsigned int i = 0; i < rasters.size(); i++) {
					if (rasters.at(i).difference >= 0.) {
						if (rasters.at(i).difference > largestDifference) {
							largestDifference = rasters.at(i).difference;
							largestDifferenceIndex = i;
						}
					} else {
						AABB aabb = rasters.at(i).aabb;
						int w = (int)(aabb.r - aabb.l);
						int h = (int)(aabb.t - aabb.b);
						glActiveTexture(GL_TEXTURE1);
						boundingBoxBuffer.resize(w, h);
						differenceBuffer.resize(w, h);
						glViewport(0, 0, w, h);
						glm::mat4 aabbProj = glm::ortho(aabb.l, aabb.r, aabb.b, aabb.t, -1.f, 1.f);
						AABB screenSpaceAabb = {
							aabb.l,
							aabb.r,
							aabb.b,
							aabb.t
						};

						// render=====
						glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
						renderRasters(&triangleShader, aabb, howManyRasterTextures, &aabbProj, i);
						// get difference===
						glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
						renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceAabb);

						rasters.at(i).difference = getAverageBufferColor(0, 0, w, h, 3);
						if (rasters.at(i).difference > largestDifference) {
							largestDifference = rasters.at(i).difference;
							largestDifferenceIndex = i;
							//if (save) cout << "highest diff: " << i << endl;
						}
						/*if (save) {
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
						}*/
					}
				}
				//save = false;
				index = largestDifferenceIndex;
				dif = largestDifference;
			}
			current = &rasters.at(index);

			Raster after = *current;
			after.randomlyTransform();
			AABB aabbBefore = current->aabb;
			AABB aabbAfter = after.aabb;
			AABB differenceAabb = getDoubleBoundingBoxBoundingBox(aabbBefore, aabbAfter);
			AABB screenSpaceDifferenceAabb = {
				differenceAabb.l,
				differenceAabb.r,
				differenceAabb.b,
				differenceAabb.t
			};

			glm::mat4 aabbProj = glm::ortho(differenceAabb.l, differenceAabb.r, differenceAabb.b, differenceAabb.t, -1.f, 1.f);

			int w = (int)(differenceAabb.r - differenceAabb.l);
			int h = (int)(differenceAabb.t - differenceAabb.b);

			//resize
			glActiveTexture(GL_TEXTURE1);
			boundingBoxBuffer.resize(w, h);
			differenceBuffer.resize(w, h);
			glViewport(0, 0, w, h);

			// render before transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderRasters(&triangleShader, differenceAabb, howManyRasterTextures, &aabbProj, index);
			// get difference==================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceDifferenceAabb);

			double averageDifferenceBefore = getAverageBufferColor(0, 0, w, h, 3);

			Raster before = *current;
			*current = after;

			// render after transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderRasters(&triangleShader, differenceAabb, howManyRasterTextures, &aabbProj, index);
			// get difference==================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceDifferenceAabb);

			double averageDifferenceAfter = getAverageBufferColor(0, 0, w, h, 3);

			if (averageDifferenceAfter > averageDifferenceBefore && !ignoreDifference) {
				*current = before;
				averageDifferenceAfter = averageDifferenceBefore;
				didSucceedMove = false;
			} else {
				for (Raster& raster : rasters) {
					if (aabbIntersect(raster.aabb, aabbBefore) || aabbIntersect(raster.aabb, aabbAfter)) {
						raster.difference = -1.;
					}
				}
			}
			iterations++;
		}

		//init imgui frame render frame==================================================================
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindFramebuffer(GL_FRAMEBUFFER, showDifference ? boundingBoxBuffer.framebuffer : 0);
		glViewport(0, 0, WIDTH, HEIGHT);
		double difference = -1.;
		glm::mat4 proj = glm::ortho(viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t, -1.f, 1.f);
		if (showDifference) {
			glActiveTexture(GL_TEXTURE1);
			boundingBoxBuffer.resize(WIDTH, HEIGHT);
			renderRasters(&triangleShader, viewAabb, howManyRasterTextures, &proj, -1);

			AABB uv = {0.f, 1.f, 0.f, 1.f};

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, uv);

			difference = getAverageBufferColor(0, 0, WIDTH, HEIGHT, 4);
		} else {
			renderRasters(&triangleShader, viewAabb, howManyRasterTextures, &proj, -1);
			if (save) {
				GLsizei stride = WIDTH * 4;
				GLsizei bufferSize = stride * HEIGHT;

				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadBuffer(GL_BACK);
				stbi_flip_vertically_on_write(true);
				unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
				glReadnPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, bufferSize, bufferData);
				stbi_write_png("output.png", WIDTH, HEIGHT, 4, bufferData, stride);
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
			glUniform3f(outlineShader.colLocation, didSucceedMove ? 0.f : 1.f, didSucceedMove ? 1.f : 0.f, 0.f);
			glUniform2f(outlineShader.sizeLocation, current->width, current->height);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		if (addingLineMode == 2) {
			float x = controls.mouseX;
			float y = controls.mouseY;
			lines.push_back({firstPointX, firstPointY, x, y});
		}
		for (unsigned int i = 0U; i < lines.size(); i++) {
			Line line = lines.at(i);

			line.x1 = line.x1 * 2.f - 1.f;
			line.y1 = line.y1 * 2.f- 1.f;
			line.x2 = line.x2 * 2.f - 1.f;
			line.y2 = line.y2 * 2.f - 1.f;

			glm::vec2 one = glm::vec2(line.x1, line.y1);
			float r = glm::length(one);
			r = lensDistortion(r, a, b, c, d);
			one = glm::normalize(one) * r;
			line.x1 = one.x;
			line.y1 = one.y;

			glm::vec2 two = glm::vec2(line.x2, line.y2);
			r = glm::length(two);
			r = lensDistortion(r, a, b, c, d);
			two = glm::normalize(two) * r;
			line.x2 = two.x;
			line.y2 = two.y;

			line.x1 = (line.x1 + 1.f) * 0.5f;
			line.y1 = (line.y1 + 1.f) * 0.5f;
			line.x2 = (line.x2 + 1.f) * 0.5f;
			line.y2 = (line.y2 + 1.f) * 0.5f;

			renderLine(line, &colorShader, viewAabb);
		}
		if (addingLineMode == 2) {
			lines.pop_back();
		}



		ImGui::Begin("Reduction");
		ImGui::Text("%d FPS", fps);
		ImGui::Checkbox("playing", &playing);
		if (ImGui::Button("add line")) {
			addingLineMode = 1;
		}
		ImGui::Text("i %d, dif %f", index, dif);
		ImGui::Text("rasters: %d", (int)rasters.size());
		static int howmany = 1;
		if (ImGui::Button("add")) {
			rasters.reserve(howmany);
			for (int i = 0; i < howmany; i++) {
				rasters.push_back({true});
			}
		}
		ImGui::SameLine();
		ImGui::SliderInt("howmany", &howmany, 1, 100);
		if (ImGui::Button("show difference")) showDifference = !showDifference;
		ImGui::Text("average difference: %f", (float)difference);
		ImGui::Text("iters: %d", iterations);
		if (ImGui::Button("save")) save = true;
		ImGui::Checkbox("only color", &only);
		ImGui::Checkbox("random raster", &isRandom);
		ImGui::Checkbox("ignore difference", &ignoreDifference);
		if (ImGui::Button("dasd")) {
			cout << viewAabb.l << ", " << viewAabb.r << ", " << viewAabb.b << ", " << viewAabb.t << endl;
		}
		//ImGui::Text("%f %f %f %f", aabb[0], aabb[1], aabb[2], aabb[3]);
		ImGui::SliderFloat("a", &a, 0.f, 0.1f);
		ImGui::SliderFloat("b", &b, 0.f, 0.1f);
		ImGui::SliderFloat("c", &c, 0.f, 0.1f);
		//d = 1.f - (a + b + c);
		ImGui::SliderFloat("d", &d, -1.f, 1.f);
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
