<Layout>
	name = vertex-lines
</Layout>
<Module>

#ifdef vulkan
struct Matrices {
	mat4 model;
	mat4 view;
	mat4 project;
};

layout(binding = 0) uniform Parameters {
	Matrices matrix;
};
#else
struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;
#endif

layout(location = 0) in vec3 in_pos;
layout(location = 1) in float in_r;
layout(location = 2) in vec4 in_col;

layout(location = 0) out vec4 out_pos; // view space
layout(location = 1) out float out_r;
layout(location = 2) out vec4 out_col;

void main() {
	gl_Position = matrix.project * matrix.view * matrix.model * vec4(in_pos, 1);
	out_r = in_r;
	out_col = in_col;
}

</Module>
