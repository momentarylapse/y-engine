<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

/*layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;*/

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 1) out vec2 out_tex_coord;

void main() {
	gl_Position = mat_p * mat_v * mat_m * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


const float shininess = 20.0;
const float gloss = 0.2;


uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

uniform vec4 eye_pos;
uniform vec4 emission_factor;

layout(binding = 3) uniform sampler2D tex0;

layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


void main() {
	vec4 t = texture(tex0, in_tex_coord);
	//t = texture(sampler_shadow, in_tex_coord);
	//t.r = pow(t.r, 10);

	out_color = t * emission_factor;
}

</FragmentShader>
