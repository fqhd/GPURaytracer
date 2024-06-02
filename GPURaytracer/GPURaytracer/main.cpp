#include "SceneData.h"
#include "Benchmarker.h"
#include "GPUGL.h"
#include <iostream>

SceneData getScene(int width) {
	SceneData scene;
	
	scene.camera = createCamera(width, width, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -3.0f), 5.0, 0.02f, 36.0f);
	scene.depth = 10;

	float radius = 0.45f;
	float gap = 0.75f;
	float step = (5.0f - gap * 2) / 3.0f;
	float offset = -2.5f + gap;

	SphereData s = createSphere(glm::vec3(0.0f, 0.0f, 0.0f), 0.5f);
	s.material = perlin(1.5f);
	addSphere(scene, s);

	// Ground
	QuadData q = createQuad(glm::vec3(2.5f, -2.5f, -2.5f), glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 5.0f));
	q.material = checker(1.2);
	addQuad(scene, q);

	// Cyan
	q = createQuad(glm::vec3(2.5f, -2.5f, -2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(-5.0f, 0.0f, 0.0f));
	q.material = metal(glm::vec3(0.18f, 1.0f, 1.0f), 0.85f);
	addQuad(scene, q);

	// Red
	q = createQuad(glm::vec3(2.5f, -2.5f, 2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	q.material = lambertian(glm::vec3(0.65f, 0.05f, 0.05f));
	addQuad(scene, q);

	// Green
	q = createQuad(glm::vec3(-2.5f, -2.5f, -2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 5.0f));
	q.material = metal(glm::vec3(0.12f, 0.45f, 0.15f), 0.05);
	addQuad(scene, q);

	q = createQuad(glm::vec3(-2.5f, -2.5f, 2.5f), glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(5.0f, 0.0f, 0.0f));
	q.material = lambertian(glm::vec3(0.73f));
	addQuad(scene, q);

	q = createQuad(glm::vec3(-2.5f, 2.5f, 2.5f), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(5.0f, 0.0f, 0.0f));
	q.material = lambertian(glm::vec3(0.73f));
	addQuad(scene, q);

	q = createQuad(glm::vec3(-0.5f, 2.499f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	q.material = light(15.0f);
	addQuad(scene, q);

	return scene;
}

int main() {
	std::unique_ptr<GPUGL> gpu = std::make_unique<GPUGL>(1024, 1024);

	SceneData scene = getScene(1024);
		
	Benchmarker::Start("Draw");
	gpu->Run(scene, "image.png", 100000);
	Benchmarker::End("Draw");

	std::cout << "Done!" << std::endl;

	return 0;
}
