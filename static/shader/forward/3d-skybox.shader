<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
/*layout(binding = 0)*/ uniform Matrix matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 1) out vec2 out_tex_coord;

void main() {
	gl_Position = matrix.project * matrix.view * matrix.model * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


uniform vec4 eye_pos;

struct Material {
	float ambient, specular, shininess;
	vec4 diffusive, emission;
};
/*layout(binding = 2)*/ uniform Material material;

layout(binding = 3) uniform sampler2D tex0;

layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


void main() {
	vec4 t = texture(tex0, in_tex_coord);
	//t = texture(sampler_shadow, in_tex_coord);
	//t.r = pow(t.r, 10);

	out_color = t * material.emission;
}

</FragmentShader>
