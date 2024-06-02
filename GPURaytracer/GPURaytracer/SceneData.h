#pragma once
#include "glm/glm.hpp"
#include <vector>

struct MaterialData {
	glm::vec3 albedo;
	float brightness;
	float ir = 0.0f;
	float roughness = 0.0f;
	float scale = 0.0f;
	int type = 0;
};

struct SphereData {
	MaterialData material;
	glm::vec3 position;
	float radius = 0.0f;
};

struct QuadData {
	MaterialData material;
	glm::vec3 Q;
	glm::vec3 u;
	glm::vec3 v;
	glm::vec3 normal;
	float D;
	glm::vec3 w;
};

struct HittableList {
	std::vector<SphereData> spheres;
	std::vector<QuadData> quads;
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
	CameraData camera;
	HittableList list;
};

MaterialData dielectric(float ir);
MaterialData metal(const glm::vec3& color, float roughness);
MaterialData lambertian(const glm::vec3& color);
MaterialData perlin(float scale);
MaterialData checker(float scale);
MaterialData light(float brightness);

SphereData createSphere(const glm::vec3& center, float radius);
QuadData createQuad(const glm::vec3& q, const glm::vec3& u, const glm::vec3& v);

void addSphere(SceneData& list, const SphereData& s);
void addQuad(SceneData& list, const QuadData& s);

void addCube(SceneData& list, glm::vec3 position, float width, MaterialData material);
void addRectangle(SceneData& list, glm::vec3 position, float width, float height, float length);


CameraData createCamera(int width, int height, const glm::vec3& position,
	const glm::vec3& lookAt, float focusDistance, float defocusAngle, float fov);