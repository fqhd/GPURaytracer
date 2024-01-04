#pragma once
#include "glm/glm.hpp"

#define NUM_SPHERES 4
#define NUM_QUADS 7

float random();

struct MaterialData {
	void randomize() {
		albedo.x = random();
		albedo.y = random();
		albedo.z = random();
		ir = random() * 2;
		roughness = random();
		scale = 1.5f;
		type = rand() % 4;
		if (type == 3) type = 4;
	}
	glm::vec3 albedo;
	float ir = 0.0f;
	float roughness = 0.0f;
	float scale = 0.0f;
	int type = 0;
};

struct SphereData {
	void randomize() {
		position.y = random() * 4 - 2;
		material.randomize();
	}
	MaterialData material;
	glm::vec3 position;
	float radius = 0.0f;
};

struct QuadData {
	QuadData(){}
	QuadData(const glm::vec3& _Q, const glm::vec3 _u, const glm::vec3 _v) {
		Q = _Q;
		u = _u;
		v = _v;
		auto n = cross(u, v);
		normal = normalize(n);
		D = dot(normal, Q);
		w = n / dot(n, n);
	}
	MaterialData material;
	glm::vec3 Q;
	glm::vec3 u;
	glm::vec3 v;
	glm::vec3 normal;
	float D;
	glm::vec3 w;
};

struct HittableList {
	SphereData spheres[NUM_SPHERES];
	QuadData quads[NUM_QUADS];
};

struct CameraData {
	glm::vec3 position;
	glm::vec3 defocusDiskU;
	glm::vec3 defocusDiskV;
	glm::vec3 pixelDeltaU;
	glm::vec3 pixelDeltaV;
	glm::vec3 pixel00Loc;
	float defocusAngle;
};

struct SceneData {
	int depth;
	int numSamples;
	CameraData camera;
	HittableList list;
};

MaterialData dielectric(float ir);
MaterialData metal(const glm::vec3& color, float roughness);
MaterialData lambertian(const glm::vec3& color);
MaterialData perlin(float scale);
MaterialData checker(float scale);
CameraData createCamera(int width, int height, const glm::vec3& position,
	const glm::vec3& lookAt, float focusDistance, float defocusAngle, float fov);