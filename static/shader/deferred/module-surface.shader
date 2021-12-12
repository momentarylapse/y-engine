<Layout>
	name = surface-deferred
</Layout>
<Module>


struct Material {
	vec4 albedo, emission;
	float roughness, metal;
};
uniform Material material;
//struct Matrix { mat4 model, view, project; };
///*layout(binding = 0)*/ uniform Matrix matrix;

struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};
uniform int num_lights;
uniform int shadow_index = 0;

/*layout(binding = 1)*/ uniform LightData {
	Light light[32];
};

struct Fog {
	vec4 color;
	float distance;
};
/*layout(binding = 3)*/ uniform Fog fog;

layout(location = 0) in vec4 in_pos; // world space
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color; // albedo
layout(location = 1) out vec4 out_emission;
layout(location = 2) out vec4 out_pos;
layout(location = 3) out vec4 out_normal;


void surface_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness) {
	out_color = albedo;
	out_color.a = 1;
	out_pos.xyz = in_pos.xyz / in_pos.w;
	out_pos.a = 1;
	out_emission = vec4(emission.rgb, metal);
	out_normal = vec4(n, roughness);
}

</Module>
