<Layout>
	version = 460
	extensions = GL_EXT_ray_tracing,GL_EXT_nonuniform_qualifier,GL_EXT_buffer_reference2,GL_EXT_scalar_block_layout
</Layout>

<RayClosestHitShader>

struct Vertex {
	vec3 p;
	vec3 n;
	vec2 uv;
};
layout(buffer_reference, scalar) readonly buffer XVertices { Vertex v[256]; };

struct Mesh {
	mat4 matrix;
	vec4 albedo;
	vec4 emission;
	XVertices vertices;
	XVertices indices_dummy;
	int num_triangles, _a, _b, _c;
};

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

layout(location = 0) rayPayloadInEXT RayPayload ray;
                     hitAttributeEXT vec2 bary_coord; // automatically filled
layout(binding=5, std430) uniform MeshData { Mesh mesh[256]; };

struct Triangle {
	vec3 a;
	vec3 b;
	vec3 c;
	vec3 na;
	vec3 nb;
	vec3 nc;
	vec4 albedo;
	vec4 emission;
};

Triangle unpack(int mid, int index) {
	Mesh m = mesh[mid];
	
	Triangle t;
	t.a = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3].p, 1);
	t.b = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3+1].p, 1);
	t.c = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3+2].p, 1);
	t.na = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3].n, 0);
	t.nb = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3+1].n, 0);
	t.nc = gl_ObjectToWorldEXT * vec4(m.vertices.v[index*3+2].n, 0);
	t.albedo = m.albedo;
	t.emission = m.emission;
	return t;
}


//layout(location=1) rayPayloadNV RayPayload SecondaryRay;

void main() {
	Triangle t = unpack(gl_InstanceID, gl_PrimitiveID);

	
	vec3 p = t.a * (1 - bary_coord.x - bary_coord.y) + t.b * bary_coord.x + t.c * bary_coord.y;
	vec3 n = t.na * (1 - bary_coord.x - bary_coord.y) + t.nb * bary_coord.x + t.nc * bary_coord.y;
	const float id = float(gl_InstanceCustomIndexEXT);
	
	if (dot(n, gl_WorldRayDirectionEXT) > 0)
		n = -n;

	ray.pos_and_dist = vec4(p, gl_HitTEXT);
	ray.albedo = t.albedo;
	ray.emission = t.emission;
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

layout(location = 0) rayPayloadInEXT RayPayload ray;

layout(set=0, binding=2, std140)  uniform MoreData {
	mat4 iview;
	vec4 background;
	int num_triangles;
	int num_lights;
} push;

void main() {
	ray.pos_and_dist = vec4(0,0,0, -1);
	ray.albedo = vec4(0,0,0, 1);
	ray.emission = //vec4(0,0,0, 1);
		push.background;
	ray.normal_and_id = vec4(0.0);
}
</RayMissShader>
