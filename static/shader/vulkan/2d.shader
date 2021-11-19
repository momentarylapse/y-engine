<Layout>
	bindings = [[buffer,sampler]]
	pushsize = 4
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = mat.proj * mat.view * mat.model * vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex0;
//layout(binding = 2) uniform sampler2D tex1;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


layout(binding = 0) uniform Parameters {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 color;
	float blur;
	float exposure;
	float gamma;
} param;

/*vec3 tone_map(vec3 c) {
	return vec3(1.0) - exp(-c * param.exposure*0.5);
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
}*/

void main() {
	out_color = texture(tex0, in_tex_coord) * param.color;
/*	if (param.blur > 0) {
		vec3 t = out_color.rgb;
		ivec2 ts = textureSize(tex1, 0);
		vec2 uv = gl_FragCoord.xy / ts;
		vec3 bg = blur_bg(uv, vec2(1,1) / ts);
		float a = out_color.a;
		out_color.rgb = tone_map(bg);
		out_color.rgb = pow(out_color.rgb, vec3(1.0 / param.gamma));
		out_color.rgb = (1-a) * out_color.rgb + a * t;
		out_color.a = 1;
	}*/
}
</FragmentShader>
