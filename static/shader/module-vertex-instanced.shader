<Layout>
	version = 420
	name = vertex-instanced
</Layout>
<Module>
struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 0) out vec4 out_pos; // world space
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_normal;

/*layout(binding = 5)*/ uniform Multi {
	mat4 multi[1000];
};



void main() {
	mat4 m = multi[gl_InstanceID];
	gl_Position = matrix.project * matrix.view * m * vec4(in_position, 1);
	out_normal = (matrix.view * m * vec4(in_normal, 0)).xyz;
	out_uv = in_uv;
	out_pos = matrix.view * m * vec4(in_position,1);
}
</Module>

