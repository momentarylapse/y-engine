<Layout>
	version = 420
	name = surface-forward
</Layout>
<Module>

struct Matrices {
	mat4 model;
	mat4 view;
	mat4 project;
};
struct Material {
	vec4 albedo, emission;
	float roughness, metal;
};

layout(binding = 0) uniform Parameters {
	Matrices matrix;
	Material material;
};

layout(location = 0) in vec4 in_pos; // world
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

void surface_out(vec3 n, vec4 di, vec4 em, float metal, float roughness) {
	out_color = di * max(n.y,0) + em;
}

</Module>
