<Layout>
	version = 420
	name = lighting
</Layout>
<Module>


#ifdef vulkan
#else
struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};

uniform int num_lights;
uniform int shadow_index = -1;

layout(std140 /*,binding = 1*/) uniform LightData {
	Light light[32];
};

struct Fog {
	vec4 color;
	float distance;
};
/*layout(binding = 3)*/ uniform Fog fog;
#endif

const float PI = 3.141592654;

float _surf_rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}

vec3 _surf_light_dir(Light l, vec3 p) {
	if (l.radius < 0)
		return l.dir.xyz;
	return normalize(p - l.pos.xyz);
}


float _surf_brightness(Light l, vec3 p) {
	// parallel
	if (l.radius < 0)
		return 1.0f;
	
	
	float d = length(p - l.pos.xyz);
	if (d > l.radius)
		return 0.0;
	float b = min(pow(1.0/d, 2), 1.0);
	
	// spherical
	if (l.theta < 0)
		return b;
	
	// cone
	float t = acos(dot(l.dir.xyz, normalize(p - l.pos.xyz)));
	float tmax = l.theta;
	return b * (1 - smoothstep(tmax*0.8, tmax, t));
}

vec3 _surf_light_add(Light l, vec3 p, vec3 n, vec3 albedo, float metal, float roughness, float ambient_occlusion, vec3 view_dir) {
	// calculate per-light radiance
	vec3 radiance = l.color.rgb * _surf_brightness(l, p);

	vec3 V = -view_dir;
	vec3 L = -_surf_light_dir(l, p);

	// add to outgoing radiance Lo
	float NdotL = max(dot(n, L), 0.0);
	return albedo * radiance * NdotL;
}

vec4 perform_lighting(vec3 p, vec3 n, vec4 albedo, vec4 emission, float metal, float roughness, float ambient_occlusion, vec3 eye_pos) {
	vec3 view_dir = normalize(p - eye_pos);
	
	roughness = max(roughness, 0.03);

	vec4 color = emission;
	for (int i=0; i<num_lights; i++)
		color.rgb += _surf_light_add(light[i], p, n, albedo.rgb, metal, roughness, ambient_occlusion, view_dir).rgb;
	
/*	float distance = length(p - eye_pos.xyz);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	*/
	color.a = albedo.a;
	return color;
}
</Module>
