<Layout>
	bindings = [[buffer,sampler]]
	pushsize = 4
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;

uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = mat_p * mat_v * mat_m * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

uniform float blur = 0.0;
uniform float exposure = 1.0;
uniform float gamma = 2.2;
uniform vec4 color = vec4(1,1,1,1);

vec3 tone_map(vec3 c) {
	return vec3(1.0) - exp(-c * exposure*0.5);
}

vec3 blur_bg(vec2 uv, vec2 d) {
		vec3 bg = vec3(0,0,0);//texture(tex1, uv).rgb;
		float sum = 0;
		float R = 40;
		for (float t=0; t<R*1.5; t+=0.28) {
			float w = exp(-t/R);
			sum += w;
			float x = cos(t) * t * 0.5;
			float y = sin(t) * t * 0.5;
			bg += w * texture(tex1, uv + vec2(d.x * x, d.y * y)).rgb;
		}
		return bg / sum;
}

void main() {
	out_color = texture(tex0, in_tex_coord) * color;
	if (blur > 0) {
		vec3 t = out_color.rgb;
		ivec2 ts = textureSize(tex1, 0);
		vec2 uv = gl_FragCoord.xy / ts;
		vec3 bg = blur_bg(uv, vec2(1,1) / ts);
		float a = out_color.a;
		out_color.rgb = tone_map(bg);
		out_color.rgb = pow(out_color.rgb, vec3(1.0 / gamma));
		out_color.rgb = (1-a) * out_color.rgb + a * t;
		out_color.a = 1;
	}
}
</FragmentShader>
