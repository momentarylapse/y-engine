<Layout>
	bindings = [[sampler,sampler,sampler,sampler,sampler,sampler,sampler,sampler,buffer,buffer]]
	pushsize = 0
	input = [vec3,vec4,vec2]
	topology = triangles
	version = 450
</Layout>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform sampler2D tex0;

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color;

layout(location = 0) out vec4 out_color;

/*struct Fog {
	vec4 color;
	float distance;
};
layout(binding = 3) uniform Fog fog;*/



void main() {
	// particle pixel pos (screen space)
//	vec3 ppp = in_pos_proj.xyz / in_pos_proj.w;
	
	// previous pixel pos (screen space)
	out_color = texture(tex0, in_uv) * in_color;
	//out_color = vec4(1,0,0,1);
	//out_color.rgb = out_color.rgb * (1-fog_density) + fog_color.rgb * fog_density;
}
</FragmentShader>
