#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

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
float invLerp(float a, float b, float t) {
	return (t - a) / (b - a);
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
		cout << "[INFO] Texture loaded: \"" << path << "\" color channels: " << numChannels << endl;
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
	unsigned int modelLocation, texLocation, colLocation, aLocation, bLocation, cLocation, dLocation, aabblLocation, aabbrLocation, aabbbLocation, aabbtLocation, ratioLocation,
	aaResLocation, resolutionLocation, transLocation, binarySearchIterationsLocation, combineMosaicLocation, combineModeLocation, showTransformLocation, gridLocation, gridNumberLocation,
	asdasd1Location, asdasd2Location;
	TriangleShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
		modelLocation = glGetUniformLocation(ID, "modelMat");
		texLocation = glGetUniformLocation(ID, "tex");
		colLocation = glGetUniformLocation(ID, "col");
		aLocation = glGetUniformLocation(ID, "a");
		bLocation = glGetUniformLocation(ID, "b");
		cLocation = glGetUniformLocation(ID, "c");
		dLocation = glGetUniformLocation(ID, "d");
		aabblLocation = glGetUniformLocation(ID, "aabbl");
		aabbrLocation = glGetUniformLocation(ID, "aabbr");
		aabbbLocation = glGetUniformLocation(ID, "aabbb");
		aabbtLocation = glGetUniformLocation(ID, "aabbt");
		ratioLocation = glGetUniformLocation(ID, "ratio");
		aaResLocation = glGetUniformLocation(ID, "aaRes");
		resolutionLocation = glGetUniformLocation(ID, "resolution");
		transLocation = glGetUniformLocation(ID, "trans");
		binarySearchIterationsLocation = glGetUniformLocation(ID, "binarySearchIterations");
		combineMosaicLocation = glGetUniformLocation(ID, "combineMosaic");
		combineModeLocation = glGetUniformLocation(ID, "combineMode");
		showTransformLocation = glGetUniformLocation(ID, "showTransform");
		gridLocation = glGetUniformLocation(ID, "grid");
		gridNumberLocation = glGetUniformLocation(ID, "gridNumber");
		asdasd1Location = glGetUniformLocation(ID, "asdasd1");
		asdasd2Location = glGetUniformLocation(ID, "asdasd2");
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
class CircleShader : public Shader {
public:
	unsigned int modelLocation, projLocation, colLocation;
	CircleShader(const char* vertexPath, const char* fragmentPath) : Shader(vertexPath, fragmentPath) {
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
int frameWidth = 640, frameHeight = 480;
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
	frameWidth = w;
	frameHeight = h;
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
	bool w, a, s, d, q, e, right, left, up, down, didMouseClick, didMouseUnclick, z;
	double mouseX, mouseY, transMouseX, transMouseY, scroll;
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
		if (key == GLFW_KEY_Z) controls.z = true;
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
	mouseX = xpos, mouseY = (double)frameHeight - ypos;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard) return;

	controls.scroll = yoffset;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard) return;

	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		controls.didMouseClick = true;
	}
	if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
		controls.didMouseUnclick = true;
	}
}
float clamp(float a, float lo, float hi) {
	return min(max(a, lo), hi);
}
class Point {
public:
	float x, y;
	Point(float xx, float yy) : x(xx), y(yy) {}
	Point() : x(0.f), y(0.f) {}
	static float squareDistance(Point a, Point b) {
		return square(a.x - b.x) + square(a.y - b.y);
	}
};
struct AABB {
	float l, r, b, t;
};
Point transformPointFromTo(Point p, AABB from, AABB to) {
	return {lerp(to.l, to.r, invLerp(from.l, from.r, p.x)), lerp(to.b, to.t, invLerp(from.b, from.t, p.y))};
}
bool only = false;
struct Raster {
	int texId;
	Raster() {
	}
};
bool save = false;

glm::mat3 adj(glm::mat3 m) { // no problem
	return glm::mat3(
		m[1][1]*m[2][2]-m[2][1]*m[1][2], m[2][0]*m[1][2]-m[1][0]*m[2][2], m[1][0]*m[2][1]-m[2][0]*m[1][1],
		m[2][1]*m[0][2]-m[0][1]*m[2][2], m[0][0]*m[2][2]-m[2][0]*m[0][2], m[2][0]*m[0][1]-m[0][0]*m[2][1],
		m[0][1]*m[1][2]-m[1][1]*m[0][2], m[1][0]*m[0][2]-m[0][0]*m[1][2], m[0][0]*m[1][1]-m[1][0]*m[0][1]
	);
}
glm::mat3 adj321(glm::mat3 m) { // no problem
	return glm::mat3(
		m[1][1]*m[2][2]-m[1][2]*m[2][1], m[0][2]*m[2][1]-m[0][1]*m[2][2], m[0][1]*m[1][2]-m[0][2]*m[1][1],
		m[1][2]*m[2][0]-m[1][0]*m[2][2], m[0][0]*m[2][2]-m[0][2]*m[2][0], m[0][2]*m[1][0]-m[0][0]*m[1][2],
		m[1][0]*m[2][1]-m[1][1]*m[2][0], m[0][1]*m[2][0]-m[0][0]*m[2][1], m[0][0]*m[1][1]-m[0][1]*m[1][0]
	);
}
glm::mat3 multmm(glm::mat3 a, glm::mat3 b) { // multiply two matrices
  glm::mat3 c;
  for (int i = 0; i != 3; ++i) {
    for (int j = 0; j != 3; ++j) {
      float cij = 0.f;
      for (int k = 0; k != 3; ++k) {
        cij += a[i][k]*b[k][j];
      }
      c[i][j] = cij;
    }
  }
  return c;
}
glm::mat3 basisToPoints(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) { // no problem
	glm::mat3 m = glm::mat3(
		x1, x2, x3,
		y1, y2, y3,
		1.f,  1.f,  1.f
	);
	glm::vec3 v = adj(m) * glm::vec3(x4, y4, 1.);
	//cout << v.x << ", " << v.y << ", " << v.z << endl;
	glm::mat3 asd = multmm(m, glm::mat3(
		v.x, 0.f, 0.f,
		0.f, v.y, 0.f,
		0.f, 0.f, v.z
	));
	//cout << asd[0][0] << ", " << asd[0][1] << ", " << asd[0][2] << ", " << asd[1][0] << ", " << asd[1][1] << ", " << asd[1][2] << ", " << asd[2][0] << ", " << asd[2][1] << ", " << asd[2][2] << endl;
	return asd;
}
glm::mat3 general2DProjection( // no problem
	float x1s, float y1s, float x1d, float y1d,
	float x2s, float y2s, float x2d, float y2d,
	float x3s, float y3s, float x3d, float y3d,
	float x4s, float y4s, float x4d, float y4d
) {
	glm::mat3 s = basisToPoints(x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s); // good
	glm::mat3 d = basisToPoints(x1d, y1d, x2d, y2d, x3d, y3d, x4d, y4d); // good
	glm::mat3 asd = multmm(d, adj321(s));
	return asd;

}
glm::mat3 transform2d(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {
	float w = 1.f;
	float h = 1.f;
	glm::mat3 t = general2DProjection(
		0.f, 0.f, x1, y1,
		w , 0.f, x4, y4,
		0.f, h , x2, y2,
		w , h , x3, y3
	);
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			t[x][y] /= t[2][2];
		}
	}
	t = glm::mat3(
		t[0][0], t[1][0], t[2][0],
		t[0][1], t[1][1], t[2][1],
		t[0][2], t[1][2], t[2][2]
	);
	return t;
}

Point transformQuad[4] = {{0.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}};
int gridX = 3;
int gridY = 3;

vector<Raster> rasters;
float a = 0.f, b = 0.f, c = 0.f, d = 1.f;
float ratio = 1.5f;
int binarySearchIterations = 10;
glm::mat3 trans = glm::mat3(1.f);

int tris = 0;
bool ddo = true;
bool combineMosaic = false;
int combineMode = 0;
bool showTransform = false;
int gridNumber = 0;
float asdasd1 = 0.38f;
float asdasd2 = 0.5f;

float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * glm::pow(r, 3.f) + b * glm::pow(r, 2.f) + c * r + d) * r;
}
float inverseLensDistortion(float distortedR, float a, float b, float c, float d) {
    float r = distortedR;

    for (int i = 0; i < binarySearchIterations; i++) { // 10 iterations should be sufficient
        float f = lensDistortion(r, a, b, c, d) - distortedR;
        float derivative = r * r * (4.f * a * r + 3.f * b) + 2.f * c * r + d;

        if (abs(derivative) < 1e-6) {
            break;
        }

        r -= f / derivative;
    }

    return r;
}
glm::vec2 transformPoint(glm::vec2 uv, bool lens) {
	if (showTransform) {
		glm::vec3 transformed = glm::vec3(uv, 1.f);
		transformed = trans * transformed;
		uv = glm::vec2(transformed.x, transformed.y) / transformed.z;
	}

	if (!lens) return uv;
	uv = uv * 2.f - 1.f;
	uv.x *= ratio;
	float r = glm::length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = glm::normalize(uv) * r;
	uv.x /= ratio;
	return (uv + 1.f) * 0.5f;
}
glm::vec2 inverseTransformPoint(glm::vec2 uv) {
	uv = uv * 2.f - 1.f;
	uv.x *= ratio;
	float r = glm::length(uv);
	r = lensDistortion(r, a, b, c, d);
	uv = glm::normalize(uv) * r;
	uv.x /= ratio;
	uv = (uv + 1.f) * 0.5f;

	if (showTransform) {
		glm::vec3 transformed = glm::vec3(uv, 1.f);
		transformed = glm::inverse(trans) * transformed;
		uv = glm::vec2(transformed.x, transformed.y) / transformed.z;
	}


	return uv;
}
void renderRasters(TriangleShader* triangleShader, AABB aabb, int aaRes, int width, int height, int howManyRasterTextures, int endI) {
	glClear(GL_COLOR_BUFFER_BIT);

	triangleShader->use();

	int i = 0;
	for (Raster& raster : rasters) {
		if (i > endI && endI != -1) break;
		i++;
		//if (!aabbIntersect(raster.aabb, aabb)) continue;
		glm::mat4 model = glm::mat4(1.f);
		glUniformMatrix4fv(triangleShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3f(triangleShader->colLocation, 1.f, 1.f, 1.f);
		glUniform1i(triangleShader->texLocation, (raster.texId % howManyRasterTextures) + 2);
		glUniform1f(triangleShader->aLocation, a);
		glUniform1f(triangleShader->bLocation, b);
		glUniform1f(triangleShader->cLocation, c);
		glUniform1f(triangleShader->dLocation, d);
		glUniform1f(triangleShader->aabblLocation, aabb.l);
		glUniform1f(triangleShader->aabbrLocation, aabb.r);
		glUniform1f(triangleShader->aabbbLocation, aabb.b);
		glUniform1f(triangleShader->aabbtLocation, aabb.t);
		glUniform1f(triangleShader->ratioLocation, ratio);
		glUniform1i(triangleShader->aaResLocation, aaRes);
		glUniform2f(triangleShader->resolutionLocation, (float)width, (float)height);
		glUniform1i(triangleShader->binarySearchIterationsLocation, binarySearchIterations);
		if (ddo) trans = transform2d(transformQuad[0].x, transformQuad[0].y, transformQuad[1].x, transformQuad[1].y, transformQuad[2].x, transformQuad[2].y, transformQuad[3].x, transformQuad[3].y);
		glUniformMatrix3fv(triangleShader->transLocation, 1, GL_FALSE, glm::value_ptr(trans));
		glUniform1i(triangleShader->combineMosaicLocation, combineMosaic);
		glUniform1i(triangleShader->combineModeLocation, combineMode);
		glUniform1i(triangleShader->showTransformLocation, showTransform);
		glUniform2i(triangleShader->gridLocation, gridX, gridY);
		glUniform1i(triangleShader->gridNumberLocation, gridNumber);
		glUniform1f(triangleShader->asdasd1Location, asdasd1);
		glUniform1f(triangleShader->asdasd2Location, asdasd2);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		tris += 2;
	}
}
const glm::mat4 identity = glm::mat4(1.f);
const glm::mat4 fullscreenProj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, -1.f, 1.f);
class Line {
public:
	Point start, end;
	Line(Point a, Point b) : start(a), end(b) {}
	Line() : start({}), end({}) {}
	float length() {
		return sqrt(square(end.x - start.x) + square(end.y - start.y));
	}
	Point center() {
		return {(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f};
	}
	void getStandardForm(float* a, float* b, float* c) {
		*a = end.y - start.y;
		*b = -(end.x - start.x);
		*c = *a * start.x + *b * start.y;
	}
	float getPointInterpolation(Point p) {
		if (start.x == end.x) return (p.y - start.y) / (end.y - start.y);
		float slope = (end.y - start.y) / (end.x - start.x);
		return ((p.x + slope * (p.y - start.y + slope * start.x)) / (slope * slope + 1.f) - start.x) / (end.x - start.x);
	}
	static Point intersect(Line line1, Line line2, bool* didIntersect, bool endless1, bool endless2, bool endInclusive = true) {
		float a1, b1, c1;
		float a2, b2, c2;
		line1.getStandardForm(&a1, &b1, &c1);
		line2.getStandardForm(&a2, &b2, &c2);
		float denominator = a1 * b2 - a2 * b1;
		if (denominator == 0) {
			*didIntersect = false;
			return {0.f, 0.f};
		}
		Point p = {
			(b2 * c1 - b1 * c2) / denominator,
			(a1 * c2 - a2 * c1) / denominator
		};
		*didIntersect = true;
		if (!endless1) {
			float t = line1.getPointInterpolation(p);
			if (t < 0.f || (endInclusive ? t > 1.f : t >= 1.f)) *didIntersect = false;
		}
		if (!endless2) {
			float t = line2.getPointInterpolation(p);
			if (t < 0.f || (endInclusive ? t > 1.f : t >= 1.f)) *didIntersect = false;
		}
		return p;
	}
	void extendToAABB(AABB aabb) {
		// top, bottom, left, right
		Line edges[4] = {
			{{aabb.l, aabb.t}, {aabb.r, aabb.t}},
			{{aabb.r, aabb.b}, {aabb.r, aabb.t}},
			{{aabb.l, aabb.b}, {aabb.r, aabb.b}},
			{{aabb.l, aabb.b}, {aabb.l, aabb.t}}
		};

		Point firstPoint;
		Point secondPoint;
		int intersections = 0;

		for (int i = 0; i < 4 && intersections < 2; i++) {
			bool didIntersect;
			Point intersection = Line::intersect(*this, edges[i], &didIntersect, true, false, false);
			if (didIntersect) { // edit first or second point
				(intersections == 0 ? firstPoint : secondPoint) = intersection;
				intersections++;
			}
		}
		start = firstPoint;
		end = secondPoint;
	}
};

enum ClickMode {
	CM_NONE,CM_LINE_START,CM_LINE_END,CM_REFERENCEPOINT
};
/*struct LensLine {
	float x1, float y1, float x2, float y2, float x3, float y3;
};*/
void renderLine(Line line, ColorShader* colorShader, AABB viewAabb, bool endless) {
	float aspect = (float)frameWidth / (float)frameHeight;
	const float width = 8.f / (float)frameWidth;
	Line screenSpaceLine = {transformPointFromTo(line.start, viewAabb, {-1.f, 1.f, -1.f, 1.f}), transformPointFromTo(line.end, viewAabb, {-1.f, 1.f, -1.f, 1.f})};

	if (endless) {
		screenSpaceLine.extendToAABB({-1.f, 1.f, -1.f, 1.f});
	}


	colorShader->use();

	glm::mat4 proj = glm::mat4(1.f);///glm::ortho(viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t, -1.f, 1.f);
	glUniformMatrix4fv(colorShader->projLocation, 1, GL_FALSE, glm::value_ptr(proj));


	screenSpaceLine.start.y /= aspect; // uniform width and stuff
	screenSpaceLine.end.y /= aspect;
	float length = screenSpaceLine.length();
	Point center = screenSpaceLine.center();

	glm::mat4 model = glm::mat4(1.f);
	model = glm::scale(model, glm::vec3(1.f, aspect, 1.f));
	model = glm::translate(model, glm::vec3(center.x, center.y, 0.f));
	model = glm::rotate(model, atan((screenSpaceLine.end.x - screenSpaceLine.start.x) / (screenSpaceLine.end.y - screenSpaceLine.start.y)), glm::vec3(0.f, 0.f, -1.f));
	model = glm::scale(model, glm::vec3(width, length, 1.f));

	glUniformMatrix4fv(colorShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(colorShader->colLocation, 0.f, 1.f, 1.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	tris += 2;
}
void renderCircle(float x, float y, float r, CircleShader* circleShader, AABB viewAabb) {
	x = invLerp(viewAabb.l, viewAabb.r, x) * 2.f - 1.f;
	y = invLerp(viewAabb.b, viewAabb.t, y) * 2.f - 1.f;

	circleShader->use();

	glm::mat4 proj = glm::mat4(1.f);///glm::ortho(viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t, -1.f, 1.f);
	glUniformMatrix4fv(circleShader->projLocation, 1, GL_FALSE, glm::value_ptr(proj));

	glm::mat4 model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(x, y, 0.f));
	float width = r / (float)frameWidth;
	model = glm::scale(model, glm::vec3(width, width * (float)frameWidth / (float)frameHeight, 1.f));

	glUniformMatrix4fv(circleShader->modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(circleShader->colLocation, 0.5f, 1.f, 1.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	tris += 2;
}
int main(void) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
	glfwWindowHint(GLFW_SAMPLES, 1);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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
	glfwSwapInterval(1);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLES_NV);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	TriangleShader triangleShader{"shaders/raster.vsh", "shaders/fragment.fsh"};
	DifferenceShader differenceShader{"shaders/vertex.vsh", "shaders/difference.fsh"};
	OutlineShader outlineShader{"shaders/vertex.vsh", "shaders/outline.fsh"};
	ColorShader colorShader{"shaders/vertex.vsh", "shaders/color.fsh"};
	CircleShader circleShader{"shaders/vertex.vsh", "shaders/circle.fsh"};

	shared_ptr<Texture> rasterTextures[] = {
		make_shared<Texture>("../807/DSC00159.png")
	};
	int howManyRasterTextures = sizeof(rasterTextures) / sizeof(shared_ptr<Texture>);

	frameWidth = 640;
	frameHeight = 480;
	glfwSetWindowSize(window, frameWidth, frameHeight);

	FrameBuffer renderBuffer(frameWidth, frameHeight);

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
	double last = glfwGetTime();
	float dt = 1.f;
	for (int i = 0; i < 1; i++) {
		rasters.push_back({});
	}

	glClearColor(0.f, 0.f, 0.f, 0.f);

	for (int i = 0; i < howManyRasterTextures; i++) {
		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, rasterTextures[i]->id);
	}

	glBindVertexArray(VAO);

	vector<Line> lines;
	vector<Point> referencePoints;
	//vector<LensLine> lensLines;
	int selectedQuadPoint = -1;

	ClickMode clickMode = CM_NONE;
	Point lineStartPoint{0.f, 0.f};

	int iterations = 0;
	bool ignoreDifference = false;
	bool playing = false;
	bool isRandom = false;
	float howLongHasTheArrowKeyBeenPressed = 0.f;

	AABB viewAabb = {0.f, 1.f, 0.f, 1.f};
	
	// dropdown menu imgui
	const char* combineModeItems[] = {"mean", "median", "single", "mode", "mad"};
	int currentCombineModeItemNumber = 0;
	//llooop
	while (!glfwWindowShouldClose(window)) {
		controls.mouseX = mouseX / (double)frameWidth * (double)(viewAabb.r - viewAabb.l) + viewAabb.l;
		controls.mouseY = mouseY / (double)frameHeight * (double)(viewAabb.t - viewAabb.b) + viewAabb.b;
		glm::vec2 transMouse = glm::vec2(controls.mouseX, controls.mouseY);
		transMouse = transformPoint(transMouse, true);
		controls.transMouseX = transMouse.x;
		controls.transMouseY = transMouse.y;

		const float scrollSpeed = 0.1f;
		viewAabb.l = lerp(viewAabb.l, controls.mouseX, controls.scroll * scrollSpeed);
		viewAabb.r = lerp(viewAabb.r, controls.mouseX, controls.scroll * scrollSpeed);
		viewAabb.b = lerp(viewAabb.b, controls.mouseY, controls.scroll * scrollSpeed);
		viewAabb.t = lerp(viewAabb.t, controls.mouseY, controls.scroll * scrollSpeed);
		controls.scroll = 0.;

		if (controls.left || controls.right || controls.down || controls.up) howLongHasTheArrowKeyBeenPressed += dt;
			else howLongHasTheArrowKeyBeenPressed = 0.f;
		float howmuch = max(howLongHasTheArrowKeyBeenPressed, howLongHasTheArrowKeyBeenPressed * howLongHasTheArrowKeyBeenPressed) * 0.1f * dt;
		if (controls.left) {
			float t = controls.w ? -howmuch : howmuch;
			glm::vec3 one = glm::vec3(t, 0.f, 1.f);
			one = trans * one;
			transformQuad[0].x = one.x / one.z;
			transformQuad[0].y = one.y / one.z;

			glm::vec3 two = glm::vec3(t, 1.f, 1.f);
			two = trans * two;
			transformQuad[1].x = two.x / two.z;
			transformQuad[1].y = two.y / two.z;
		}
		if (controls.right) {
			float t = controls.w ? 1.f + howmuch : 1.f - howmuch;
			glm::vec3 one = glm::vec3(t, 0.f, 1.f);
			one = trans * one;
			transformQuad[3].x = one.x / one.z;
			transformQuad[3].y = one.y / one.z;

			glm::vec3 two = glm::vec3(t, 1.f, 1.f);
			two = trans * two;
			transformQuad[2].x = two.x / two.z;
			transformQuad[2].y = two.y / two.z;
		}
		if (controls.down) {
			float t = controls.w ? -howmuch : howmuch;
			glm::vec3 one = glm::vec3(0.f, t, 1.f);
			one = trans * one;
			transformQuad[0].x = one.x / one.z;
			transformQuad[0].y = one.y / one.z;

			glm::vec3 two = glm::vec3(1.f, t, 1.f);
			two = trans * two;
			transformQuad[3].x = two.x / two.z;
			transformQuad[3].y = two.y / two.z;
		}
		if (controls.up) {
			float t = controls.w ? 1.f + howmuch : 1.f - howmuch;
			glm::vec3 one = glm::vec3(0.f, t, 1.f);
			one = trans * one;
			transformQuad[1].x = one.x / one.z;
			transformQuad[1].y = one.y / one.z;

			glm::vec3 two = glm::vec3(1.f, t, 1.f);
			two = trans * two;
			transformQuad[2].x = two.x / two.z;
			transformQuad[2].y = two.y / two.z;
		}

		if (controls.z) {
			controls.z = false;
			lines.pop_back();
		}

		if (controls.didMouseClick) {
			switch (clickMode) {
			case CM_NONE:{
				if (selectedQuadPoint == -1) {
					for (int i = 0; i < 4; i++) {
						if (square(transformQuad[i].x - controls.mouseX) + square((transformQuad[i].y - controls.mouseY) * (float)frameHeight / (float)frameWidth) < square((viewAabb.r - viewAabb.l) * (30.f / (float)frameWidth))) {
							selectedQuadPoint = i;
							break;
						}
					}
				}
				break;
			}
			case CM_LINE_START:{
				lineStartPoint.x = controls.transMouseX;
				lineStartPoint.y = controls.transMouseY;
				clickMode = CM_LINE_END;
				break;
			}
			case CM_LINE_END:{
				float x = controls.transMouseX;
				float y = controls.transMouseY;
				lines.push_back({{lineStartPoint.x, lineStartPoint.y}, {x, y}});
				clickMode = CM_NONE;
				break;
			}
			case CM_REFERENCEPOINT:{
				float x = controls.transMouseX;
				float y = controls.transMouseY;
				referencePoints.push_back({x, y});
				clickMode = CM_NONE;
				break;
			}
			}
		}
		if (controls.didMouseUnclick) {
			selectedQuadPoint = -1;
		}
		controls.didMouseClick = false;
		controls.didMouseUnclick = false;

		if (selectedQuadPoint != -1) {
			transformQuad[selectedQuadPoint].x = controls.mouseX;
			transformQuad[selectedQuadPoint].y = controls.mouseY;
		}

		Raster* current;
		bool didSucceedMove = true;
		int index = -1;
		tris = 0;
		/*if (playing) {
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
						AABB screenSpaceAabb = {
							aabb.l,
							aabb.r,
							aabb.b,
							aabb.t
						};

						// render=====
						glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
						renderRasters(&triangleShader, aabb, 1, w, h, howManyRasterTextures, i);
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
						}*
					}
				}
				//save = false;
				index = largestDifferenceIndex;
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


			int w = (int)(differenceAabb.r - differenceAabb.l);
			int h = (int)(differenceAabb.t - differenceAabb.b);

			//resize
			glActiveTexture(GL_TEXTURE1);
			boundingBoxBuffer.resize(w, h);
			differenceBuffer.resize(w, h);
			glViewport(0, 0, w, h);

			// render before transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderRasters(&triangleShader, differenceAabb, 1, w, h, howManyRasterTextures, index);
			// get difference==================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, differenceBuffer.framebuffer);
			renderDifference(&differenceShader, boundingBoxBuffer.textureColorBuffer, screenSpaceDifferenceAabb);

			double averageDifferenceBefore = getAverageBufferColor(0, 0, w, h, 3);

			Raster before = *current;
			*current = after;

			// render after transform===================================================================================================================
			glBindFramebuffer(GL_FRAMEBUFFER, boundingBoxBuffer.framebuffer);
			renderRasters(&triangleShader, differenceAabb, 1, w, h, howManyRasterTextures, index);
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
		}*/

		//init imgui frame render frame==================================================================
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, frameWidth, frameHeight);
		glClear(GL_COLOR_BUFFER_BIT);
		double difference = -1.;
		glm::mat4 proj = glm::ortho(viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t, -1.f, 1.f);

		int w = frameWidth, h = frameHeight;
		if (save) {
			//w = frameWidth * 2;// * (viewAabb.r - viewAabb.l);
			//h = frameHeight * 2;// * (viewAabb.t - viewAabb.b);
			//glViewport(0, 0, w, h);
		}
		renderRasters(&triangleShader, viewAabb, save ? 3 : 1, frameWidth, frameHeight, howManyRasterTextures, -1);
		if (save) {
			GLsizei stride = w * 4;
			GLsizei bufferSize = stride * h;

			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadBuffer(GL_BACK);
			stbi_flip_vertically_on_write(true);
			unsigned char* bufferData = (unsigned char*)malloc(sizeof(unsigned char) * bufferSize);
			glReadnPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bufferSize, bufferData);
			stbi_write_png("output.png", w, h, 4, bufferData, stride);
			free(bufferData);
		}
		save = false;

		if (clickMode == CM_LINE_END) {
			float x = controls.transMouseX;
			float y = controls.transMouseY;
			lines.push_back({{lineStartPoint.x, lineStartPoint.y}, {x, y}});
		}
		// render lines
		for (unsigned int i = 0U; i < lines.size(); i++) {
			Line line = lines.at(i);

			glm::vec2 one = glm::vec2(line.start.x, line.start.y);
			one = inverseTransformPoint(one);
			line.start.x = one.x;
			line.start.y = one.y;

			glm::vec2 two = glm::vec2(line.end.x, line.end.y);
			two = inverseTransformPoint(two);
			line.end.x = two.x;
			line.end.y = two.y;

			renderLine(line, &colorShader, viewAabb, true);
		}
		// rener reference points
		for (unsigned int i = 0U; i < referencePoints.size(); i++) {
			glm::vec2 point = glm::vec2(referencePoints.at(i).x, referencePoints.at(i).y);
			point = inverseTransformPoint(point);

			renderCircle(point.x, point.y, 20.f, &circleShader, viewAabb);
		}

		//render perspective grid
		if (!showTransform) {
			for (int i = 0; i < 4; i++) {
				Line line = {{transformQuad[i].x, transformQuad[i].y}, {transformQuad[(i + 1) % 4].x, transformQuad[(i + 1) % 4].y}};
				renderLine(line, &colorShader, viewAabb, false);
				renderCircle(transformQuad[i].x, transformQuad[i].y, 30.f, &circleShader, viewAabb);
			}
			for (int x = 1; x < gridX; x++) {
				float t = (float)x / (float)gridX;
				glm::vec3 one = glm::vec3(t, 0.f, 1.f);
				one = trans * one;

				glm::vec3 two = glm::vec3(t, 1.f, 1.f);
				two = trans * two;

				Line line = {{one.x / one.z, one.y / one.z}, {two.x / two.z, two.y / two.z}};
				renderLine(line, &colorShader, viewAabb, false);
			}
			for (int y = 1; y < gridY; y++) {
				float t = (float)y / (float)gridY;
				glm::vec3 one = glm::vec3(0.f, t, 1.f);
				one = trans * one;

				glm::vec3 two = glm::vec3(1.f, t, 1.f);
				two = trans * two;

				Line line = {{one.x / one.z, one.y / one.z}, {two.x / two.z, two.y / two.z}};
				renderLine(line, &colorShader, viewAabb, false);
			}
		}

		if (clickMode == CM_LINE_END) {
			lines.pop_back();
		}



		ImGui::Begin("Raster Doer");
		ImGui::Text("%d FPS %f", fps, dt);
		ImGui::Text("%d", tris);
		if (ImGui::Button("Save")) save = true;
		if (ImGui::CollapsingHeader("stats and stuff")) {
			ImGui::Text("View %f %f %f %f", viewAabb.l, viewAabb.r, viewAabb.b, viewAabb.t);
			ImGui::Text("Mouse %f %f", controls.mouseX, controls.mouseY);
		}
		if (ImGui::Button("Add line")) {
			clickMode = CM_LINE_START;
		}
		if (ImGui::Button("Add point")) {
			clickMode = CM_REFERENCEPOINT;
		}
		if (ImGui::Button("Align")) {
			glm::vec2 bl = glm::vec2(-ratio, -1.f);;
			float r = glm::length(bl);
			r = lensDistortion(r, a, b, c, d);
			bl = glm::normalize(bl) * r;
			viewAabb.l = bl.x * 0.5f / ratio + 0.5f;
			viewAabb.b = bl.y * 0.5f + 0.5f;

			glm::vec2 tr = glm::vec2(ratio, 1.f);
			r = glm::length(tr);
			r = lensDistortion(r, a, b, c, d);
			tr = glm::normalize(tr) * r;
			viewAabb.r = tr.x * 0.5f / ratio + 0.5f;
			viewAabb.t = tr.y * 0.5f + 0.5f;
		}
		static bool nearest = true;
		if (ImGui::Checkbox("Nearest", &nearest)) {
			for (int i = 0; i < howManyRasterTextures; i++) {
				glBindTexture(GL_TEXTURE_2D, rasterTextures[i]->id);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
			}
		}
		if (ImGui::BeginCombo("##combo", combineModeItems[currentCombineModeItemNumber])) {
			for (int n = 0; n < IM_ARRAYSIZE(combineModeItems); n++) {
				if (n == 1) continue;;
				bool is_selected = currentCombineModeItemNumber == n;
				if (ImGui::Selectable(combineModeItems[n], is_selected)) {
					combineMode = n;
					currentCombineModeItemNumber = n;
				}
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::Checkbox("Show transform", &showTransform);
		ImGui::Checkbox("Combine mosaic", &combineMosaic);
		int* grid[] = {&gridX, &gridY};
		ImGui::SliderInt2("Grid res", *grid, 1, 30);
		ImGui::SliderInt("Grid num", &gridNumber, 0, gridX * gridY - 1);
		ImGui::SameLine();
		static bool yes = false;
		ImGui::Checkbox("Cycle through", &yes);
		ImGui::SameLine();
		static bool random = false;
		ImGui::Checkbox("Rand", &random);
		if (yes) gridNumber = random ? rand() % (gridX * gridY) : (gridNumber + 1) % (gridX * gridY);
		ImGui::SliderInt("Lens iterations", &binarySearchIterations, 1, 50);
		//ImGui::Text("%f %f %f %f", aabb[0], aabb[1], aabb[2], aabb[3]);
		ImGui::SliderFloat("a", &a, 0.f, 0.2f);
		ImGui::SliderFloat("b", &b, 0.f, 0.2f);
		ImGui::SliderFloat("c", &c, 0.f, 0.2f);
		//d = 1.f - (a + b + c);
		ImGui::SliderFloat("d", &d, -1.f, 1.f);
		ImGui::SliderFloat("asdasd1", &asdasd1, 0.f, 1.f);
		ImGui::SliderFloat("asdasd2", &asdasd2, 0.f, 1.f);

		ImGui::Checkbox("do", &ddo);
		/*float* them = glm::value_ptr(trans);
		float* row1[] = {them    , them + 1 , them + 2};
		float* row2[] = {them + 3, them + 4 , them + 5};
		float* row3[] = {them + 6, them + 7 , them + 8};
		ImGui::SliderFloat3("[0]", *row1, -1.f, 1.f);
		ImGui::SliderFloat3("[1]", *row2, -1.f, 1.f);
		ImGui::SliderFloat3("[2]", *row3, -1.f, 1.f);*/
		if (ImGui::Button("<")) {
			float t = -1.f / (float)gridX;
			glm::vec3 one = glm::vec3(t, 0.f, 1.f);
			one = trans * one;
			transformQuad[0].x = one.x / one.z;
			transformQuad[0].y = one.y / one.z;

			glm::vec3 two = glm::vec3(t, 1.f, 1.f);
			two = trans * two;
			transformQuad[1].x = two.x / two.z;
			transformQuad[1].y = two.y / two.z;
			gridX++;
		}
		ImGui::SameLine();
		if (ImGui::Button(">")) {
			float t = 1.f + 1.f / (float)gridX;
			glm::vec3 one = glm::vec3(t, 0.f, 1.f);
			one = trans * one;
			transformQuad[3].x = one.x / one.z;
			transformQuad[3].y = one.y / one.z;

			glm::vec3 two = glm::vec3(t, 1.f, 1.f);
			two = trans * two;
			transformQuad[2].x = two.x / two.z;
			transformQuad[2].y = two.y / two.z;
			gridX++;
		}
		ImGui::SameLine();
		if (ImGui::Button("v")) {
			float t = -1.f / (float)gridY;
			glm::vec3 one = glm::vec3(0.f, t, 1.f);
			one = trans * one;
			transformQuad[0].x = one.x / one.z;
			transformQuad[0].y = one.y / one.z;

			glm::vec3 two = glm::vec3(1.f, t, 1.f);
			two = trans * two;
			transformQuad[3].x = two.x / two.z;
			transformQuad[3].y = two.y / two.z;
			gridY++;
		}
		ImGui::SameLine();
		if (ImGui::Button("^")) {
			float t = 1.f + 1.f / (float)gridY;
			glm::vec3 one = glm::vec3(0.f, t, 1.f);
			one = trans * one;
			transformQuad[1].x = one.x / one.z;
			transformQuad[1].y = one.y / one.z;

			glm::vec3 two = glm::vec3(1.f, t, 1.f);
			two = trans * two;
			transformQuad[2].x = two.x / two.z;
			transformQuad[2].y = two.y / two.z;
			gridY++;
		}
		float* p1[] = {&transformQuad[0].x, &transformQuad[0].y};
		float* p2[] = {&transformQuad[1].x, &transformQuad[1].y};
		float* p3[] = {&transformQuad[2].x, &transformQuad[2].y};
		float* p4[] = {&transformQuad[3].x, &transformQuad[3].y};
		ImGui::SliderFloat2("1", *p1, 0.f, 1.f);
		ImGui::SliderFloat2("2", *p2, 0.f, 1.f);
		ImGui::SliderFloat2("3", *p3, 0.f, 1.f);
		ImGui::SliderFloat2("4", *p4, 0.f, 1.f);

		ImGui::SliderFloat("ratio", &ratio, 0.5f, 2.f);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		dt = glfwGetTime() - last;
		last = glfwGetTime();
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
