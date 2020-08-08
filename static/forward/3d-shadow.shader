<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 330
#extension GL_ARB_separate_shader_objects : enable

uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

void main() {
	gl_Position = mat_p * mat_v * mat_m * vec4(in_position, 1.0);
}
</VertexShader>
<FragmentShader>
#version 330
#extension GL_ARB_separate_shader_objects : enable


void main() {
}
</FragmentShader>
