<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler]]
	pushsize = 100
	version = 450
</Layout>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

/*layout(binding = 3)*/ uniform sampler2D tex0;

layout(location = 0) in vec4 in_pos_proj;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color;

layout(location = 0) out vec4 out_color;


struct Fog {
	vec4 color;
	float distance;
};
/*layout(binding = 3)*/ uniform Fog fog;



void main() {
	// particle pixel pos (screen space)
//	vec3 ppp = in_pos.xyz / in_pos.w;
	
//	vec2 tc = ppp.xy/2 - vec2(0.5,0.5);

	vec2 tc = in_uv;

	// previous pixel pos (screen space)
	out_color = texture(tex0, tc) * in_color;
	//out_color.rgb = out_color.rgb * (1-fog_density) + fog_color.rgb * fog_density;
}
</FragmentShader>
