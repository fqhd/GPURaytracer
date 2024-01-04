#pragma once
#include <memory>
#include "SceneData.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "glad.h"
#include "Shader.h"

class GPUGL {
public:

	GPUGL(int width, int height);
	~GPUGL();

    void Run(const SceneData& data, const std::string& path, int numSamples);

private:

	void UploadSceneData(const SceneData& data);
	void CreateQuad();

	GLFWwindow* m_Window = nullptr;
	std::unique_ptr<Shader> m_Shader = nullptr;
	int m_Width;
	int m_Height;

};