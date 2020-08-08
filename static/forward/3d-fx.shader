<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler]]
	pushsize = 100
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 450
#extension GL_ARB_separate_shader_objects : enable


uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

/*layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;

layout(push_constant) uniform PushConstants {
	mat4 projection;
	vec4 color;
};*/

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec2 out_tex_coord;
layout(location = 1) out vec4 out_pos_proj;

void main() {
	gl_Position = mat_mvp * vec4(in_position, 1.0);
	out_pos_proj = gl_Position;
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 3) uniform sampler2D tex0;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) in vec4 in_pos_proj;

layout(location = 0) out vec4 out_color;

uniform vec4 color;
uniform vec4 source;

/*layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;*/


layout(binding = 1) uniform Fog {
	vec4 color;
	float distance;
} fog;

//layout(push_constant) uniform PushConstants {
//	mat4 projection;
//	vec4 color;
//};



void main() {
	// particle pixel pos (screen space)
//	vec3 ppp = in_pos_proj.xyz / in_pos_proj.w;
	
//	vec2 tc = ppp.xy/2 - vec2(0.5,0.5);

	vec2 tc = vec2(source.x + (source.y-source.x) * in_tex_coord.x, source.z + (source.w-source.z) * in_tex_coord.y);

	// previous pixel pos (screen space)
	out_color = texture(tex0, tc) * color;
	//out_color.rgb = out_color.rgb * (1-fog_density) + fog_color.rgb * fog_density;
}
</FragmentShader>
