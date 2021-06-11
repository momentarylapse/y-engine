<Layout>
	bindings = [[buffer,sampler]]
	pushsize = 4
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable

uniform vec2 axis = vec2(1,1);
uniform float max_radius = 50.0;
uniform float focal_length = 1000.0;
uniform float focal_blur = 1.0;
uniform mat4 invproj;//mat_p;

layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;



//uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

vec3 blur(float r) {
	vec2 DD = 1.0 / textureSize(tex0, 0);
	vec3 bb = vec3(0,0,0);
	float sum = 0.0;
	//float d = 0.0015;
	int R = int(r);
	for (int i=-R; i<=R; i++) {
		float fi = i;
		float w = exp(-((fi*fi) / (r*r)) * 4.0);
		vec3 c = texture(tex0, in_tex_coord + DD * axis * fi).rgb;
		bb += w * c;
		sum += w;
	}
	return bb / sum;
}

void main() {
	float z = texture(tex1, in_tex_coord).r*2 -1;
	vec4 xx = invproj * vec4(0,0,z,1);
	float dist = xx.z / xx.w;
	float d = dist/focal_length;
	float r = min(pow(abs(1/d-d), 2)*focal_blur, 1);
	
	out_color.rgb = texture(tex0, in_tex_coord).rgb;
	if (r > 0.001)
		out_color.rgb = blur(max_radius * r);
	
	out_color.a = 1;
}
</FragmentShader>
