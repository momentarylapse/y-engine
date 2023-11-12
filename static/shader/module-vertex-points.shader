<Layout>
	name = vertex-points
</Layout>
<Module>
#import basic-interface

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
