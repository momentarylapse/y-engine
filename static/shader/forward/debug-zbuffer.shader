<Layout>
	bindings = [[buffer,sampler]]
	pushsize = 4
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

#ifdef vulkan
layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 project;
} matrix;
#else
struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
uniform Matrix matrix;
#endif

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = matrix.project * matrix.view * matrix.model * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;



void main() {
	out_color = texture(tex0, in_tex_coord);
	out_color.rgb = vec3(1-pow(out_color.r, 3));
	out_color.a = 1;
}
</FragmentShader>
