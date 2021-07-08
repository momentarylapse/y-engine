<Layout>
	version = 420
	name = surface
</Layout>
<Module>

layout(binding = 6) uniform sampler2D tex3;//sampler_shadow;
layout(binding = 7) uniform sampler2D tex4;//sampler_shadow2;

#define tex_shadow0 tex3
#define tex_shadow1 tex4

#import lighting

uniform vec3 eye_pos;


uniform samplerCube tex_cube;


layout(location = 0) in vec4 in_pos; // world
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;


void surface_out(vec3 n, vec4 albedo, vec4 emission, float metal, float roughness) {
	out_color = emission;
	vec3 p = in_pos.xyz / in_pos.w;
	vec3 view_dir = normalize(p - eye_pos.xyz);
	
	roughness = max(roughness, 0.03);
	
///	float reflectivity = 1-((1-xxx.x) * (1-exp(-pow(dot(d, n),2) * 100)));

	//if (metal > 0.01 && false) {
	if (metal > 0.9 && roughness < 0.2) {
		vec3 L = reflect(view_dir, n);
		//for (int i=0; i<30; i++) {
		//vec3 L = normalize(L0 + vec3(_surf_rand3d(p)-0.5, _surf_rand3d(p)-0.5, _surf_rand3d(p)-0.5) / 50);
		vec4 r = texture(tex_cube, -L);
		//out_color = r;
		//return;
		/*if (roughness > 0.1) {
			r += texture(tex_cube, reflect(view_dir, normalize(n + vec3(_surf_rand3d(p),0,1) * roughness/10)));
			r += texture(tex_cube, reflect(view_dir, normalize(n + vec3(1,_surf_rand3d(p),0) * roughness/10)));
			r += texture(tex_cube, reflect(view_dir, normalize(n + vec3(0,1,_surf_rand3d(p)) * roughness/10)));
			r /= 5;
		}
		out_color += r * reflectivity;*/
		
		
		
	//	float R = (1-roughness) + roughness * pow(1 - dot(n, L), 5);
	//	out_color.rgb += R * r.rgb;

		vec3 F;
        	vec3 specular = min(_surf_specular(albedo.rgb, metal, roughness, -view_dir, L, n, F), vec3(1));
        	float NdotL = max(dot(n, L), 0.0);
        	out_color.rgb += specular * r.rgb * NdotL;
        	//out_color.rgb = specular * NdotL;
        	return;
	}
	

	out_color += perform_lighting(p, n, albedo, emission, metal, roughness, eye_pos);
	
/*	float distance = length(p - eye_pos.xyz);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	*/
	out_color.a = albedo.a;
}
</Module>
