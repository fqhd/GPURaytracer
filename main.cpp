#include "SceneData.h"
#include "Benchmarker.h"
#include "GPUGL.h"
#include <iostream>

SceneData getRandomScene(int width) {
	SceneData scene;
	
	scene.camera = createCamera(width, width, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -3.0f), 5.0, 0.02f, 36.0f);
	scene.depth = 10;

	float radius = 0.45f;
	float gap = 0.75f;
	float step = (5.0f - gap * 2) / 3.0f;
	float offset = -2.5f + gap;

	SphereData& s1 = scene.list.spheres[0];
	s1.material = perlin(1.5f);
	s1.position = glm::vec3(offset + step * 0, 0.0f, 0.0f);
	s1.radius = radius;
	s1.randomize();

	SphereData& s2 = scene.list.spheres[1];
	s2.material = lambertian(glm::vec3(0.4f, 0.2f, 0.1f));
	s2.position = glm::vec3(offset + step * 1, 0.0f, 0.0f);
	s2.radius = radius;
	s2.randomize();

	SphereData& s3 = scene.list.spheres[2];
	s3.material = dielectric(1.5f);
	s3.position = glm::vec3(offset + step * 2, 0.0f, 0.0f);
	s3.radius = radius;
	s3.randomize();

	SphereData& s4 = scene.list.spheres[3];
	s4.material = metal(glm::vec3(0.7f, 0.6f, 0.5f), 0.5f);
	s4.position = glm::vec3(offset + step * 3, 0.0f, 0.0f);
	s4.radius = radius;
	s4.randomize();

	// Ground
	scene.list.quads[0] = QuadData(glm::vec3(2.5f, -2.5f, -2.5f), glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 5.0f));
	scene.list.quads[0].material = checker(1.2);

	// Cyan
	scene.list.quads[1] = QuadData(glm::vec3(2.5f, -2.5f, -2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(-5.0f, 0.0f, 0.0f));
	scene.list.quads[1].material = metal(glm::vec3(0.18f, 1.0f, 1.0f), 0.85f);

	// Red
	scene.list.quads[2] = QuadData(glm::vec3(2.5f, -2.5f, 2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	scene.list.quads[2].material = lambertian(glm::vec3(0.65f, 0.05f, 0.05f));

	// Green
	scene.list.quads[3] = QuadData(glm::vec3(-2.5f, -2.5f, -2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 5.0f));
	scene.list.quads[3].material = metal(glm::vec3(0.12f, 0.45f, 0.15f), 0.05);

	scene.list.quads[4] = QuadData(glm::vec3(-2.5f, -2.5f, 2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(5.0f, 0.0f, 0.0f));
	scene.list.quads[4].material = lambertian(glm::vec3(0.73f));

	scene.list.quads[5] = QuadData(glm::vec3(-2.5f, 2.5f, 2.5f), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(5.0f, 0.0f, 0.0f));
	scene.list.quads[5].material = lambertian(glm::vec3(0.73f));

	scene.list.quads[6] = QuadData(glm::vec3(-1.5f, 2.499f, -1.5f), glm::vec3(3.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 3.0f));
	scene.list.quads[6].material.type = 5;

	return scene;
}

int main() {
	
	int numScenes = 0;
	std::cout << "Number of Scenes: ";
	std::cin >> numScenes;
	int imageWidth = 0;
	std::cout << "Image Width: ";
	std::cin >> imageWidth;
	int numSamplesLow = 0;
	std::cout << "Num Samples Low: ";
	std::cin >> numSamplesLow;
	int numSamplesHigh = 0;
	std::cout << "Num Samples High: ";
	std::cin >> numSamplesHigh;

	std::unique_ptr<GPUGL> gpu = std::make_unique<GPUGL>(imageWidth, imageWidth);

	for (int i = 0; i < numScenes; i++) {
		std::string lowPath = "low/";
		std::string highPath = "high/";
		
		SceneData scene = getRandomScene(imageWidth);
		
		// Write image
		Benchmarker::Start("Draw");
		gpu->Run(scene, lowPath + std::to_string(i) + ".png", numSamplesLow);
		gpu->Run(scene, highPath + std::to_string(i) + ".png", numSamplesHigh);
		Benchmarker::End("Draw");
	}

	std::cout << "Done!" << std::endl;

	system("pause");

	return 0;
}
