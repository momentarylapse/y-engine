<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler]]
	pushsize = 100
	version = 450
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable



struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
/*layout(binding = 0)*/ uniform Matrix matrix;


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_pos_proj;
layout(location = 1) out vec4 out_color;
layout(location = 2) out vec2 out_uv;

void main() {
	gl_Position = matrix.project * matrix.view * matrix.model * vec4(in_position, 1.0);
	out_pos_proj = gl_Position;
	out_color = in_color;
	out_uv = in_uv;
}
</VertexShader>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

/*layout(binding = 3)*/ uniform sampler2D tex0;

layout(location = 0) in vec4 in_pos_proj;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


struct Fog {
	vec4 color;
	float distance;
};
/*layout(binding = 3)*/ uniform Fog fog;



void main() {
	// particle pixel pos (screen space)
//	vec3 ppp = in_pos_proj.xyz / in_pos_proj.w;
	
//	vec2 tc = ppp.xy/2 - vec2(0.5,0.5);

	vec2 tc = in_uv;

	// previous pixel pos (screen space)
	out_color = texture(tex0, tc) * in_color;
	//out_color.rgb = out_color.rgb * (1-fog_density) + fog_color.rgb * fog_density;
}
</FragmentShader>
