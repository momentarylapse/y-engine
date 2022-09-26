<Layout>
	bindings = [[sampler]]
	pushsize = 20
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
#ifdef vulkan
layout(push_constant) uniform ParameterData {
	Matrix matrix;
};
#else
uniform Matrix matrix;
#endif

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_uv;

void main() {
	gl_Position = matrix.project * vec4(in_position, 1.0);
	out_uv = in_uv;
}
</VertexShader>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

/*struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};*/

#ifdef vulkan


layout(push_constant) uniform ParameterData {
	//Matrix matrix;
	mat4 model;
	mat4 view;
	mat4 project;
	float exposure;
	float bloom_factor;
	float gamma;
	float scale_x;
	float scale_y;
};

#else

//uniform Matrix matrix;
uniform float exposure = 1.0;
uniform float bloom_factor = 0.2;
uniform float gamma = 2.2;
uniform float scale_x = 1.0;
uniform float scale_y = 1.0;

#endif

layout(binding = 0) uniform sampler2D tex0;
layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
	vec2 uv = in_uv * vec2(scale_x, scale_y);
	uv.y += 1 - scale_y;
	out_color.rgb = textureLod(tex0, uv, 0).rgb;
	out_color.a = 1;
}
</FragmentShader>
