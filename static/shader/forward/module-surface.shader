<Layout>
	version = 420
	name = surface-forward
</Layout>
<Module>


uniform vec3 eye_pos;


struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};
uniform int num_lights;
uniform int shadow_index = 0;

/*layout(binding = 1)*/ uniform LightData {
	Light light[32];
};

struct Fog {
	vec4 color;
	float distance;
};
/*layout(binding = 3)*/ uniform Fog fog;



layout(binding = 6) uniform sampler2D tex3;//sampler_shadow;
layout(binding = 7) uniform sampler2D tex4;//sampler_shadow2;

layout(location = 0) in vec4 in_pos; // world space
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

float _surf_rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}


vec3 _surf_light_dir(Light l, vec3 p) {
	if (l.radius < 0)
		return l.dir.xyz;
	return normalize(p - l.pos.xyz);
}


float _surf_brightness(Light l, vec3 p, vec3 n) {
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
	
	float t = acos(dot(l.dir.xyz, normalize(p - l.pos.xyz)));
	float tmax = l.theta;
	return b * (1 - smoothstep(tmax*0.8, tmax, t));
}

// blinn phong
float _surf_specular(Light l, vec3 p, vec3 n, float shininess) {
	vec3 view_dir = normalize(p - eye_pos);
	vec3 half_dir = normalize(_surf_light_dir(l, p) + view_dir);

	float spec_angle = max(-dot(half_dir, n), 0.0);
	return pow(spec_angle, shininess);
}

float _surf_shadow_pcf_step(vec3 p, vec2 dd, ivec2 ts) {
	vec2 d = dd / ts * 0.8;
	vec2 tp = p.xy + d;
	float epsilon = 0.004;
	float shadow_z = texture(tex4, p.xy + d).r + epsilon;
	if (tp.x > 0.38 && tp.y > 0.38 && tp.x < 0.62 && tp.y < 0.62)
		shadow_z = texture(tex3, (p.xy - vec2(0.5,0.5))*4 + vec2(0.5,0.5) + d).r + epsilon/4;
	if (p.z > shadow_z)
		return 1.0;
	return 0.0;
}

float _surf_shadow_pcf(vec3 p) {
	ivec2 ts = textureSize(tex3, 0);
	float value = 0;//shadow_pcf_step(p, vec2(0,0), ts);
	const float R = 1.8;
	const int N = 3;
	for (int i=0; i<N; i++) {
		float phi = _surf_rand3d(p + p*i) * 2 * 3.1415;
		float r = R * sqrt(fract(phi * 235.3545));
		value += _surf_shadow_pcf_step(p, r * vec2(cos(phi), sin(phi)), ts);
	}
	//value += shadow_pcf_step(p, vec2( 1.0, 1.1), ts);
	//value += shadow_pcf_step(p, vec2(-0.8, 0.9), ts);
	//value += shadow_pcf_step(p, vec2( 0.7,-0.8), ts);
	//value += shadow_pcf_step(p, vec2(-0.5,-1.1), ts);
	return value / N;//(N+1);
}

vec3 _surf_light_add(Light l, vec3 p, vec3 n, vec3 t, float roughness, float metal, bool with_shadow) {
	float shadow = 1.0;
	
	if (with_shadow) {
		vec4 proj = l.proj * vec4(p,1);
		proj.xyz /= proj.w;
		proj.x = (proj.x +1)/2;
		proj.y = (proj.y +1)/2;
		proj.z = (proj.z +1)/2;
	
		if (proj.x > 0.01 && proj.x < 0.99 && proj.y > 0.01 && proj.y < 0.99 && proj.z < 1.0) {
			shadow = 1.0 - _surf_shadow_pcf(proj.xyz) * l.harshness;
		}
	}
	
	

	float b = _surf_brightness(l, p, n) * shadow;
	float lambert = max(-dot(n, _surf_light_dir(l, p)), 0);
	
	float bb = l.harshness * lambert + (1 - l.harshness);
	vec3 col = t * l.color.rgb * bb * b;
	
	// specular
	if (lambert > 0 && roughness < 0.8) {
		float shininess = 5 / (1.1 - roughness);
		float spx = (1 - roughness) * _surf_specular(l, p, n, shininess);
		col += spx * l.color.rgb * b;
	}
	return col;
}

void surface_out(vec3 n, vec4 albedo, vec4 emission, float roughness, float metal) {
	out_color = emission;
	vec3 p = in_pos.xyz / in_pos.w;
	
///	float reflectivity = 1-((1-xxx.x) * (1-exp(-pow(dot(d, n),2) * 100)));
	float reflectivity = 0.0;
	

	for (int i=0; i<num_lights; i++)
		out_color.rgb += _surf_light_add(light[i], p, n, albedo.rgb, roughness, metal, i == shadow_index).rgb;
	
/*	float distance = length(p - eye_pos);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	*/
	
	out_color.a = albedo.a;
}
</Module>
