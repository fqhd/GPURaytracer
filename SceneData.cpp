#include "SceneData.h"
#include <iostream>

float random() {
    return rand() / (float)RAND_MAX;
}

MaterialData perlin(float scale) {
    MaterialData m;
    m.type = 4;
    m.scale = scale;
    return m;
}

MaterialData checker(float scale) {
	MaterialData m;
	m.type = 3;
	m.scale = scale;
	return m;
}

MaterialData dielectric(float ir) {
	MaterialData m;
	m.type = 2;
	m.ir = ir;
	return m;
}

MaterialData metal(const glm::vec3& color, float roughness) {
	MaterialData m;
	m.type = 1;
	m.albedo = color;
	m.roughness = roughness;
	return m;
}

MaterialData lambertian(const glm::vec3& color) {
	MaterialData m;
	m.type = 0;
	m.albedo = color;
	return m;
}

CameraData createCamera(int width, int height, const glm::vec3& position,
	const glm::vec3& lookAt, float focusDistance, float defocusAngle, float fov) {
	CameraData cam;

    // Determine viewport dimensions.
    float theta = glm::radians(fov);
    float h = tan(theta / 2);
    float viewport_height = 2 * h * focusDistance;
    float viewport_width = viewport_height * (static_cast<float>(width) / height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    glm::vec3 w = glm::normalize(position - lookAt);
    glm::vec3 u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), w));
    glm::vec3 v = glm::cross(w, u);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    glm::vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
    glm::vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

    // Calculate the horizontal and vertical delta vectors to the next pixel.
    glm::vec3 pixelDeltaU = viewport_u / (float)width;
    glm::vec3 pixelDeltaV = viewport_v / (float)height;

    // Calculate the location of the upper left pixel.
    glm::vec3 viewport_upper_left = position - (focusDistance * w) - viewport_u / 2.0f - viewport_v / 2.0f;
    glm::vec3 pixel00Loc = viewport_upper_left + 0.5f * (pixelDeltaU + pixelDeltaV);

    // Calculate the camera defocus disk basis vectors.
    float defocus_radius = focusDistance * tan(glm::radians(defocusAngle / 2.0f));
    glm::vec3 defocusDiskU = u * defocus_radius;
    glm::vec3 defocusDiskV = v * defocus_radius;

    cam.position = position;
    cam.defocusDiskU = defocusDiskU;
    cam.defocusDiskV = defocusDiskV;
    cam.pixelDeltaU = pixelDeltaU;
    cam.pixelDeltaV = pixelDeltaV;
    cam.pixel00Loc = pixel00Loc;
    cam.defocusAngle = defocusAngle;

    return cam;
}