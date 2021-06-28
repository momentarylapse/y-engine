<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<FragmentShader>

#import surface




struct Material {
	float roughness, metal;
	vec4 albedo, emission;
};
/*layout(binding = 2)*/ uniform Material material;


layout(binding = 3) uniform sampler2D tex0;
layout(binding = 4) uniform sampler2D tex1;//_mat;
layout(binding = 5) uniform sampler2D tex2;//_glow;


void main() {
	vec3 n = normalize(in_normal);
	vec4 albedo = texture(tex0, in_uv) * material.albedo;
	vec4 emission = texture(tex2/*_glow*/, in_uv) * material.emission;

	surface_out(n, albedo, emission, material.roughness, material.metal);
}

</FragmentShader>
