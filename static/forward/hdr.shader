<Layout>
	bindings = [[buffer,sampler]]
	pushsize = 4
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

uniform float exposure = 1.0;
uniform float bloom_factor = 0.2;
uniform float gamma = 2.2;

layout(binding = 1) uniform sampler2D tex0;
layout(binding = 1) uniform sampler2D tex1;

//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

vec3 tone_map(vec3 c) {
	return vec3(1.0) - exp(-c * exposure*0.5);
	//return pow(c * exposure, vec3(1,1,1)*0.2);
}

void main() {
	out_color.rgb = texture(tex0, in_tex_coord).rgb;
	vec3 bloom = texture(tex1, in_tex_coord).rgb;
	out_color.rgb += bloom * bloom_factor;
	out_color.rgb = tone_map(out_color.rgb);
	
	out_color.rgb = pow(out_color.rgb, vec3(1.0 / gamma));
	out_color.a = 1;
}
</FragmentShader>
