<Layout>
	bindings = [[buffer,buffer,buffer,sampler,sampler,sampler,sampler,sampler]]
	pushsize = 76
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
//#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 1) uniform sampler2D tex;
layout(binding = 3) uniform sampler2D tex0;//sampler_color;
layout(binding = 4) uniform sampler2D tex1;//sampler_emission;
layout(binding = 5) uniform sampler2D tex2;//sampler_pos;
layout(binding = 6) uniform sampler2D tex3;//sampler_normal;
layout(binding = 7) uniform sampler2D tex4;//sampler_shadow;
layout(binding = 8) uniform sampler2D tex5;
//layout(binding = 7) uniform sampler2DShadow sampler_shadow;

#define tex_shadow0 tex4
#define tex_shadow1 tex5

#import lighting


//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


uniform vec3 eye_pos;




void main() {

	vec4 albedo = texture(tex0, in_tex_coord);
	vec4 emission = texture(tex1, in_tex_coord); // emission
	vec4 nr = texture(tex3, in_tex_coord);
	vec3 n = normalize(nr.xyz);
	float roughness = nr.w;
	float metal = 0;
	vec3 p = texture(tex2, in_tex_coord).xyz;
	
	out_color = emission;
	out_color += perform_lighting(p, n, albedo, emission, metal, roughness, eye_pos);
	
//	out_color.rgb = pow(out_color.rgb, vec3(1.0 / 2.2)); // gamma correction
	// should happen in post-processing stage....
}
</FragmentShader>
