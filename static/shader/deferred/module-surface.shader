<Layout>
	name = surface-deferred
</Layout>
<Module>

#import basic-data

//layout(location = 0) out vec4 out_color; // albedo
layout(location = 1) out vec4 out_emission;
layout(location = 2) out vec4 out_pos;
layout(location = 3) out vec4 out_normal;


void surface_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness) {
	out_color = albedo;
	out_color.a = 1;
	out_pos.xyz = in_pos.xyz / in_pos.w;
	out_pos.a = 1;
	out_emission = vec4(emission.rgb, metal);
	out_normal = vec4(n, roughness);
}

</Module>
