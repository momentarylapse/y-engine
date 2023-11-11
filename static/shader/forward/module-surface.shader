<Layout>
	version = 420
	name = surface-forward
</Layout>
<Module>

layout(binding = 2) uniform sampler2D tex3;//sampler_shadow;
layout(binding = 3) uniform sampler2D tex4;//sampler_shadow2;
layout(binding = 4) uniform samplerCube tex_cube;//sampler_shadow2;

#define tex_shadow0 tex3
#define tex_shadow1 tex4


struct Material { vec4 albedo, emission; float roughness, metal; };
uniform Material material;
//struct Matrix { mat4 model, view, project; };
///*layout(binding = 0)*/ uniform Matrix matrix;

//uniform vec3 eye_pos;
const vec3 eye_pos = vec3(0,0,0);

struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;


layout(location = 0) in vec4 in_pos; // view space
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



#import lighting


// n - view space
void surface_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness) {
	//out_color = emission;
	vec3 p = in_pos.xyz / in_pos.w;
	vec3 view_dir = normalize(p - eye_pos.xyz);
	

	out_color = perform_lighting(p, n, albedo, emission, metal, roughness, 0, eye_pos);
	
/*	float distance = length(p - eye_pos.xyz);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	*/
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

	// reduced reflectivity
	out_color *= 1 - transmissivity * pow(abs(n.z), 2);
}

</Module>
