#include "GPUGL.h"
#include <iostream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

GPUGL::GPUGL(int width, int height)
{
	m_Width = width;
	m_Height = height;
	srand(time(0));
	stbi_flip_vertically_on_write(1);
	if(!glfwInit()){
		std::cout << "Failed to initialize GLFW for opengl window" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	m_Window = glfwCreateWindow(width, height, "GPUGL", nullptr, nullptr);
	if(!m_Window){
		std::cout << "Failed to create OpenGL window for opengl context" << std::endl;
		return;
	}

	glfwMakeContextCurrent(m_Window);
	if(!gladLoadGL()){
		std::cout << "Failed to initialize GLEW" << std::endl;
		return;
	}

	m_Shader = std::make_unique<Shader>();
	CreateQuad();
}

GPUGL::~GPUGL()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void GPUGL::CreateQuad()
{
	GLuint vao;
	GLuint vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0);

	float vertices[] = {
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GPUGL::Run(const SceneData& data, const std::string& path, int numSamples)
{
	const int batchSize = 100;
	int numBatches = numSamples / batchSize;
	int remainingSamples = numSamples % batchSize;
	int dataSize = m_Width * m_Height * 4;
	uint8_t* drawBuffer = new uint8_t[dataSize];
	uint64_t* accumulationBuffer = new uint64_t[dataSize];
	for (int i = 0; i < dataSize; i++) {
		accumulationBuffer[i] = 0;
	}
	UploadSceneData(data);
	for (int i = 0; i < numBatches; i++) {
		glfwPollEvents();
		m_Shader->loadUniform("numSamples", batchSize);
		m_Shader->loadUniform("rngState", (rand() / (float)RAND_MAX) * 1000000.0f); // Pass in different rng state before each draw call.
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, drawBuffer);
		glfwSwapBuffers(m_Window);
		glFlush();
		
		// Accumulate buffer data
		for (int j = 0; j < dataSize; j++) {
			accumulationBuffer[j] += drawBuffer[j];
		}
	}
	if (remainingSamples) {
		glfwPollEvents();
		m_Shader->loadUniform("numSamples", remainingSamples);
		m_Shader->loadUniform("rngState", (rand() / (float)RAND_MAX) * 1000000.0f); // Pass in different rng state before each draw call.
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, drawBuffer);
		glfwSwapBuffers(m_Window);
		glFlush();

		// Accumulate buffer data
		for (int j = 0; j < dataSize; j++) {
			accumulationBuffer[j] += drawBuffer[j];
		}
		numBatches++;
	}

	for (int i = 0; i < dataSize; i++) {
		drawBuffer[i] = accumulationBuffer[i] / numBatches;
	}

	stbi_write_png(path.c_str(), m_Width, m_Height, 4, drawBuffer, sizeof(unsigned char) * m_Width * 4);

	delete[] drawBuffer;
	delete[] accumulationBuffer;
}

void GPUGL::UploadSceneData(const SceneData& data)
{
	m_Shader->loadUniform("sceneData.width", m_Width);
	m_Shader->loadUniform("sceneData.height", m_Height);
	m_Shader->loadUniform("sceneData.depth", data.depth);

	m_Shader->loadUniform("sceneData.camera.position", data.camera.position);
	m_Shader->loadUniform("sceneData.camera.defocusDiskU", data.camera.defocusDiskU);
	m_Shader->loadUniform("sceneData.camera.defocusDiskV", data.camera.defocusDiskV);
	m_Shader->loadUniform("sceneData.camera.pixelDeltaU", data.camera.pixelDeltaU);
	m_Shader->loadUniform("sceneData.camera.pixelDeltaV", data.camera.pixelDeltaV);
	m_Shader->loadUniform("sceneData.camera.pixel00Loc", data.camera.pixel00Loc);
	m_Shader->loadUniform("sceneData.camera.defocusAngle", data.camera.defocusAngle);

	for (int i = 0; i < NUM_SPHERES; i++) {
		std::string sphere = "sceneData.list.spheres[";
		sphere += std::to_string(i);
		sphere += "]";
		m_Shader->loadUniform(sphere + ".material.albedo", data.list.spheres[i].material.albedo);
		m_Shader->loadUniform(sphere + ".material.ir", data.list.spheres[i].material.ir);
		m_Shader->loadUniform(sphere + ".material.roughness", data.list.spheres[i].material.roughness);
		m_Shader->loadUniform(sphere + ".material.type", data.list.spheres[i].material.type);
		m_Shader->loadUniform(sphere + ".material.scale", data.list.spheres[i].material.scale);

		m_Shader->loadUniform(sphere + ".center", data.list.spheres[i].position);
		m_Shader->loadUniform(sphere + ".radius", data.list.spheres[i].radius);
	}

	for (int i = 0; i < NUM_QUADS; i++) {
		std::string quad = "sceneData.list.quads[";
		quad += std::to_string(i);
		quad += "]";
		m_Shader->loadUniform(quad + ".material.albedo", data.list.quads[i].material.albedo);
		m_Shader->loadUniform(quad + ".material.ir", data.list.quads[i].material.ir);
		m_Shader->loadUniform(quad + ".material.roughness", data.list.quads[i].material.roughness);
		m_Shader->loadUniform(quad + ".material.type", data.list.quads[i].material.type);
		m_Shader->loadUniform(quad + ".material.scale", data.list.quads[i].material.scale);
		m_Shader->loadUniform(quad + ".Q", data.list.quads[i].Q);
		m_Shader->loadUniform(quad + ".u", data.list.quads[i].u);
		m_Shader->loadUniform(quad + ".v", data.list.quads[i].v);
		m_Shader->loadUniform(quad + ".w", data.list.quads[i].w);
		m_Shader->loadUniform(quad + ".normal", data.list.quads[i].normal);
		m_Shader->loadUniform(quad + ".D", data.list.quads[i].D);
	}
}