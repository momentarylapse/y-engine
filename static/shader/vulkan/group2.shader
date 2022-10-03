<Layout>
	version = 460
	extensions = GL_NV_ray_tracing,GL_EXT_nonuniform_qualifier
</Layout>

<RayClosestHitShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

layout(set = 0, binding = 0)        uniform accelerationStructureNV scene;

// argh, using float[], each float uses up 4x space... (losing 3x data)
//layout(set=0, binding=3) uniform Vertices { float v[99]; } vertices;
layout(set=0, binding=3) uniform Vertices { vec4 v[66]; } vertices;

layout(location = 0) rayPayloadInNV RayPayload ray;
                     hitAttributeNV vec2 hit_attribs;

struct Vertex {
	vec3 p;
	vec3 em;
};

Vertex unpack(int index) {
	Vertex v;
	v.p = vertices.v[index*2].xyz;
	v.em = vertices.v[index*2+1].rgb;
	return v;
}


layout(location = 1) rayPayloadNV RayPayload SecondaryRay;

void main() {
	Vertex A = unpack(3*gl_PrimitiveID);
	Vertex B = unpack(3*gl_PrimitiveID + 1);
	Vertex C = unpack(3*gl_PrimitiveID + 2);
	vec3 n = normalize(cross(B.p-A.p, C.p-A.p));
	if (dot(n, gl_WorldRayDirectionNV) > 0)
		n = -n;
	
	vec3 p = A.p * (1 - hit_attribs.x - hit_attribs.y) + B.p * hit_attribs.x + C.p * hit_attribs.y;
	const float id = float(gl_InstanceCustomIndexNV);
	
	ray.pos_and_dist = vec4(p, gl_HitTNV);
	ray.albedo = vec4(0.7,0.7,0.7, 1);
	ray.emission = vec4(A.em, 1);
	ray.normal_and_id = vec4(n, id);
}
</RayClosestHitShader>

<RayMissShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

layout(location = 0) rayPayloadInNV RayPayload ray;

void main() {
	ray.pos_and_dist = vec4(0,0,0, -1);
	ray.albedo = vec4(0,0,0, 1);
	ray.normal_and_id = vec4(0.0);
}
</RayMissShader>