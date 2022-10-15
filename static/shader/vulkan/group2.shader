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

//layout(set=0, binding=0)        uniform accelerationStructureNV scene;

// argh, using float[], each float uses up 4x space... (losing 3x data)
//layout(set=0, binding=3) uniform Vertices { float v[99]; } vertices;
layout(set=0, binding=3) uniform Vertices { vec4 v[100000]; } vertices;

layout(location = 0) rayPayloadInNV RayPayload ray;
                     hitAttributeNV vec2 bary_coord; // automatically filled

struct Triangle {
	vec3 a;
	vec3 b;
	vec3 c;
	vec4 albedo;
	vec4 emission;
};

Triangle unpack(int index) {
	Triangle t;
	t.a = gl_ObjectToWorldNV * vec4(vertices.v[index*5].xyz, 1);
	t.b = gl_ObjectToWorldNV * vec4(vertices.v[index*5+1].xyz, 1);
	t.c = gl_ObjectToWorldNV * vec4(vertices.v[index*5+2].xyz, 1);
	t.albedo = vertices.v[index*5+3];
	t.emission = vertices.v[index*5+4];
	return t;
}


//layout(location=1) rayPayloadNV RayPayload SecondaryRay;

void main() {
	int index = gl_PrimitiveID + gl_InstanceCustomIndexNV;
	Triangle t = unpack(index);

	vec3 n = normalize(cross(t.b-t.a, t.c-t.a));
	if (dot(n, gl_WorldRayDirectionNV) > 0)
		n = -n;
	
	vec3 p = t.a * (1 - bary_coord.x - bary_coord.y) + t.b * bary_coord.x + t.c * bary_coord.y;
	const float id = float(gl_InstanceCustomIndexNV);
	
	ray.pos_and_dist = vec4(p, gl_HitTNV);
	ray.albedo = t.albedo;
	ray.emission = t.emission;
//	ray.emission.rg = bary_coord;
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
	ray.emission = vec4(0,0,0, 1);
	ray.normal_and_id = vec4(0.0);
}
</RayMissShader>
