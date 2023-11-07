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
	int _dummy1, _dummy2;
};

struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};

layout(binding = 0) uniform ParameterData {
	Matrices matrix;
	Material material;
	int num_lights;
	int shadow_index;
};
layout(binding = 1) uniform LightData {
	Light light[32];
};

layout(location = 0) in vec4 in_pos; // view space
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


layout(binding = 2) uniform sampler2D tex_shadow0;
layout(binding = 3) uniform sampler2D tex_shadow1;
layout(binding = 5) uniform samplerCube tex_cube;


#import lighting


void surface_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness) {
	
	vec3 p = in_pos.xyz / in_pos.w;
	vec3 eye_pos = vec3(0,0,0);
	vec3 view_dir = normalize(p - eye_pos);
	
	float ambient_occlusion = 0.0;
	
	
	if (metal > 0.9 && roughness < 0.2) {
		mat3 R = transpose(mat3(matrix.view));
		vec3 L = reflect(view_dir, n);
		//for (int i=0; i<30; i++) {
		//vec3 L = normalize(L0 + vec3(_surf_rand3d(p)-0.5, _surf_rand3d(p)-0.5, _surf_rand3d(p)-0.5) / 50);
		//vec4 r = texture(tex_cube, n);//R*L);
		vec4 r = texture(tex_cube, R*L);
		r.a = 1;
		out_color = r;
		return;
	}
	
	out_color = perform_lighting(p, n, albedo, emission, metal, roughness, ambient_occlusion, eye_pos);
	out_color.a = albedo.a;
}


void surface_transmissivity_out(vec3 n, vec4 transmissivity) {
	vec3 T = transmissivity.rgb;
	T *= pow(abs(n.z), 2);
	out_color = vec4(T,1);
}


void surface_reflectivity_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness, vec4 transmissivity) {
	if (!gl_FrontFacing)
		n = - n;
	surface_out(n, albedo, emission, metal, roughness);
	
#ifndef vulkan
	vec3 p = in_pos.xyz / in_pos.w;
	mat3 R = transpose(mat3(matrix.view));
	vec3 L = reflect(p, n);
	vec4 cube = texture(tex_cube, R*L);
	out_color += cube * 0.05;
#endif

	out_color *= 1 - transmissivity * pow(abs(n.z), 2);
}

</Module>
