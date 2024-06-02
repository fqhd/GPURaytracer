#version 330 core

out vec4 outColor;

#define PI 3.14159265359
#define MAX_SPHERES 10
#define MAX_QUADS 20

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct Material {
	vec3 albedo;
	float brightness;
	float ir;
	float roughness;
	float scale;
	int type;
};

struct HitRecord {
	Material material;
	vec3 p;
	vec3 normal;
	float t;
	float u;
	float v;
	bool frontFace;
};

struct Sphere {
	Material material;
	vec3 center;
	float radius;
};

struct Quad {
	Material material;
	vec3 Q;
	vec3 u;
	vec3 v;
	vec3 w;
	vec3 normal;
	float D;
};

struct Camera {
	vec3 position;
	vec3 defocusDiskU;
	vec3 defocusDiskV;
	vec3 pixelDeltaU;
	vec3 pixelDeltaV;
	vec3 pixel00Loc;
	float defocusAngle;
};

struct HittableList {
	Quad quads[MAX_QUADS];
	Sphere spheres[MAX_SPHERES];
};

struct SceneData {
	int width;
	int height;
	int depth;
	Camera camera;
	HittableList list;
};

uniform SceneData sceneData;
uniform float rngState;
uniform int numSamples;
uniform int numQuads;
uniform int numSpheres;

// Utilities
/*
    static.frag
    by Spatial
    05 July 2013
*/

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

float randCounter = 0.0;
float rand(){
	randCounter += 0.01;
	return random(vec3(sceneData.width * gl_FragCoord.y + gl_FragCoord.x, randCounter, rngState));
}

vec3 randomInUnitSphere() {
    float u = rand();
    float v = rand();
    float theta = u * 2.0 * PI;
    float phi = acos(2.0 * v - 1.0);
    float r = pow(rand(), 1/3.0);
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);
    float x = r * sinPhi * cosTheta;
    float y = r * sinPhi * sinTheta;
    float z = r * cosPhi;
    return vec3(x, y, z);
}

vec3 randomUnitVector() {
	return normalize(randomInUnitSphere());
}

vec3 randomInHemisphere(vec3 normal) {
	vec3 inUnitSphere = randomInUnitSphere();
	if(dot(inUnitSphere, normal) > 0.0){
		return inUnitSphere;
	}else{
		return -inUnitSphere;
	}
}

bool nearZero(vec3 v){
	float s = 1e-8;
	return (abs(v.x) < s) && (abs(v.y) < s) && (abs(v.z) < s);
}

float lengthSquared(vec3 v){
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

vec3 randomInUnitDisk() {
    while (true) {
        vec3 p = vec3(rand() * 2.0 - 1.0, rand() * 2.0 - 1.0, 0.0);
        if (lengthSquared(p) >= 1.0) continue;
        return p;
    }
}

float reflectance(float cosine, float refIndex) {
	// Use Schlick's approximation for reflectance.
	float r0 = (1.0-refIndex) / (1.0 + refIndex);
	r0 = r0*r0;
	return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

Ray createRay(vec3 origin, vec3 direction){
	Ray r;
	r.origin = origin;
	r.direction = direction;
	return r;
}

vec3 at(Ray r, float t){
	return r.origin + r.direction * t;
}

void setFaceNormal(inout HitRecord record, Ray r, vec3 outwardNormal){
	record.frontFace = dot(r.direction, outwardNormal) < 0.0;
	record.normal = record.frontFace ? outwardNormal : -outwardNormal;
}

vec3 value(float u, float v, vec3 p, float scale) {
	int xInteger = int(floor(scale * p.x));
	int yInteger = int(floor(scale * p.y));
	int zInteger = int(floor(scale * p.z));

	int isEven = int(xInteger % 2 == 0) ^ int(zInteger % 2 == 0);

	return bool(isEven) ? vec3(0.0) : vec3(1.0);
}

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

vec3 spherePerlin(float u, float v, vec3 p, float scale) {
    vec3 s = scale * p;
    return vec3(0.5)*(1.0 + sin(s.z + 10.0*noise(s*10.0)));
}

float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}

void getSphereUV(vec3 p, inout HitRecord rec) {
	float theta = acos(-p.y);
	float phi = atan2(-p.z, p.x) + PI;

	rec.u = phi / (2.0 * PI);
	rec.v = theta / PI;
}

bool hit(Sphere s, Ray r, float tmin, float tmax, inout HitRecord rec) {
	vec3 oc = r.origin - s.center;
	float a = lengthSquared(r.direction);
	float half_b = dot(oc, r.direction);
	float c = lengthSquared(oc) - s.radius * s.radius;

	float discriminant = half_b * half_b - a * c;
	if(discriminant < 0.0) return false;
	float sqrtd = sqrt(discriminant);

	float root = (-half_b - sqrtd) / a;
	if(root < tmin || tmax < root){
		root = (-half_b + sqrtd) / a;
		if(root < tmin || tmax < root){
			return false;
		}
	}

	rec.t = root;
	rec.p = at(r, rec.t);
	vec3 outwardNormal = (rec.p - s.center) / s.radius;
	setFaceNormal(rec, r, outwardNormal);
	getSphereUV(outwardNormal, rec);
	rec.material = s.material;

	return true;
}

bool isInterior(float a, float b, inout HitRecord rec) {
	if ((a < 0.0) || (1.0 < a) || (b < 0.0) || (1.0 < b)){
		return false;
	}

	rec.u = a;
	rec.v = b;
	return true;
}

bool hit(Quad q, Ray r, float tmin, float tmax, inout HitRecord rec) {
	float denom = dot(q.normal, r.direction);

	// No hit if the ray is parallel to the plane.
	if (abs(denom) < 1e-8) {
		return false;
	}

	// No hit if ray is opposite to the normal of the plane
	if (dot(q.normal, r.direction) > 0.0) {
		return false;
	}

	// Return false if the hit point parameter t is outside the ray interval.
	float t = (q.D - dot(q.normal, r.origin)) / denom;
	if (!(tmin <= t && t <= tmax)){
		return false;
	}

	// Determine the hit point lies within the planar shape using its plane coordinates.
	vec3 intersection = at(r, t);
	vec3 planar_hitpt_vector = intersection - q.Q;
	float alpha = dot(q.w, cross(planar_hitpt_vector, q.v));
	float beta = dot(q.w, cross(q.u, planar_hitpt_vector));

	if (!isInterior(alpha, beta, rec)){
		return false;
	}

	// Ray hits the 2D shape; set the rest of the hit record and return true.
	rec.t = t;
	rec.p = intersection;
	rec.material = q.material;
	setFaceNormal(rec, r, q.normal);

	return true;
}

vec3 defocusDiskSample(const Camera camera) {
    vec3 p = randomInUnitDisk();
    return camera.position + (p.x * camera.defocusDiskU) + (p.y * camera.defocusDiskV);
}

vec3 pixelSampleSquare(const Camera camera) {
    float px = -0.5 + rand();
    float py = -0.5 + rand();
    return (px * camera.pixelDeltaU) + (py * camera.pixelDeltaV);
}

Ray getRay(const Camera camera, int x, int y){
	vec3 pixel_center = camera.pixel00Loc + (x * camera.pixelDeltaU) + (y * camera.pixelDeltaV);
	vec3 pixel_sample = pixel_center + pixelSampleSquare(camera);

	vec3 ray_origin = (camera.defocusAngle <= 0) ? camera.position : defocusDiskSample(camera);
	vec3 ray_direction = pixel_sample - ray_origin;

	return createRay(ray_origin, ray_direction);
}

bool scatter(const HitRecord rec, inout vec3 throughput, inout Ray ray){
	if(rec.material.type == 0){ // Lambertian
		vec3 scatterDirection = rec.normal + randomUnitVector();

		if(nearZero(scatterDirection)){
			scatterDirection = rec.normal;
		}

		ray = createRay(rec.p, scatterDirection);
		throughput *= rec.material.albedo;
		return true;
	}else if(rec.material.type == 1){ // Metal
		vec3 reflected = reflect(normalize(ray.direction), rec.normal);
		ray = createRay(rec.p, reflected + rec.material.roughness * randomInUnitSphere());
		throughput *= rec.material.albedo;
		return true;
	}else if(rec.material.type == 2){ // Dielectric
		throughput *= vec3(1.0);
		float refractionRatio = rec.frontFace ? (1.0/rec.material.ir) : rec.material.ir;

		vec3 unitDirection = normalize(ray.direction);
		float cosTheta = min(dot(-unitDirection, rec.normal), 1.0);
		float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

		bool cannotRefract = refractionRatio * sinTheta > 1.0;
		vec3 direction;

		if (cannotRefract || reflectance(cosTheta, refractionRatio) > rand()){
			direction = reflect(unitDirection, rec.normal);
		}
		else{
			direction = refract(unitDirection, rec.normal, refractionRatio);
		}

		ray = createRay(rec.p, direction);
		return true;
	}else if(rec.material.type == 3){ // Checker
		vec3 scatterDirection = rec.normal + randomUnitVector();

		if(nearZero(scatterDirection)){
			scatterDirection = rec.normal;
		}

		ray = createRay(rec.p, scatterDirection);
		throughput *= value(rec.u, rec.v, rec.p, rec.material.scale);
		return true;
	}else if(rec.material.type == 4){ // Perlin
		vec3 scatterDirection = rec.normal + randomUnitVector();

		if(nearZero(scatterDirection)){
			scatterDirection = rec.normal;
		}

		ray = createRay(rec.p, scatterDirection);
		throughput *= spherePerlin(rec.u, rec.v, rec.p, rec.material.scale);
		return true;
	}else if(rec.material.type == 5){ // Light
		throughput += rec.material.brightness * throughput;
		return false;
	}
}

bool hitHittableList(HittableList list, Ray r, float tmin, float tmax, inout HitRecord rec){
	HitRecord tempRec;
	bool hitAnything = false;
	float closestSoFar = tmax;
	for(int i = 0; i < numSpheres; i++){
		if (hit(list.spheres[i], r, tmin, closestSoFar, tempRec)){
			hitAnything = true;
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}
	for(int i = 0; i < numQuads; i++){
		if (hit(list.quads[i], r, tmin, closestSoFar, tempRec)){
			hitAnything = true;
			closestSoFar = tempRec.t;
			rec = tempRec;
		}
	}
	return hitAnything;
}

vec3 getMissColor(Ray r){
	return vec3(0.0);
}

vec3 rayColor(Ray ray, HittableList list, int depth){
	vec3 color = vec3(0.0);
	vec3 throughput = vec3(1.0);
	HitRecord record;

	while (depth-- > 0) {
		if(!hitHittableList(list, ray, 0.001, 1000.0, record)){
			color += getMissColor(ray) * throughput;
			break;
		}
		
		if(!scatter(record, throughput, ray)){
			color += throughput;
			break;
		}
	}

	return color;
}

void main() {
	vec3 color = vec3(0.0);
	int depth = sceneData.depth;
	for(int i = 0; i < numSamples; i++){
		int x = int(gl_FragCoord.x);
		int y = sceneData.height - int(gl_FragCoord.y);
		Ray r = getRay(sceneData.camera, x, y);
		color += rayColor(r, sceneData.list, depth);
	}
	float scale = 1.0 / numSamples;
	color = sqrt(color * scale);
	outColor = vec4(color, 1.0);
}