<Layout>
	version = 450
	bindings = [[buffer,sampler]]
	pushsize = 0
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<FragmentShader>
#import surface

layout(binding = 1) uniform sampler2D tex0;

void main() {
	// Mesh
	vec3 tmp1 = in_pos.xyz / in_pos.w;
	vec3 tmp2 = normalize(in_normal);
	vec2 tmp3 = in_uv;
	// Material
	vec4 tmp4 = material.albedo;
	float tmp5 = material.roughness;
	float tmp6 = material.metal;
	vec4 tmp7 = material.emission;
	// Texture0
	vec4 tmp8 = texture(tex0, tmp3);
	// Multiply
	vec4 tmp9 = tmp8 * tmp4;
	// SurfaceOutput
	surface_out(tmp2, tmp9, tmp7, tmp6, tmp5);
}
</FragmentShader>
