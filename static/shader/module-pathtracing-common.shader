<Layout>
	name = pathtracing-common
</Layout>
<Module>

// ASSUMES:
//   bool simple_trace(vec3 p, vec3 dir, float max_depth);
//   float MAX_DEPTH;

const float pi = 3.141592654;

struct Light {
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
	int shadow_index;
};

layout(binding=9, std140) uniform LightData {
	int num_lights;
	int num_surfels;
	bool fog_enabled;
	float fog_density;
	ivec4 probe_cells;
	vec4 probe_min, probe_max;
	mat4 shadow_proj[2];
	vec4 fog_color;
	Light light[500];
};

float rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,42.1234))) * 43758.5453);
}

vec3 rand_dir(vec3 p) {
	float u = rand3d(p);
	float v = rand3d(p + vec3(10.43767,20.92546,30.7536));
	float z = 2*v-1;
	float a = sqrt(1-z*z);
	return vec3(cos(u*2*pi) * a, sin(u*2*pi) * a, z);
	//vec3 v = vec3(rand3d(p)*2-1, rand3d(p + vec3(10.43767,20.92546,30.7536))*2-1, rand3d(p + vec3(40.2695,50.976234,60.1567))*2-1);
	//return normalize(v);
}


float calc_light_visibility_point(vec3 p, vec3 LP, float light_radius, int N) {
	float light_visibility = 0.0;
	for (int i=0; i<N; i++) {
		vec3 lsp = LP + rand_dir(p + vec3(i,2*i,3*i)) * light_radius;
		float d = length(lsp - p);
		vec3 L = (lsp-p) / d;//normalize(lsp - p);
		if (!simple_trace(p, L, d+1))
			light_visibility += 1.0 / N;
	}
	return light_visibility;
}

float calc_light_visibility_directional(vec3 p, vec3 L, float fuzzyness, int N) {
	float light_visibility = 0.0;
	for (int i=0; i<N; i++) {
		vec3 LL = -normalize(L + fuzzyness * rand_dir(p + vec3(i,2*i,3*i)));
		if (!simple_trace(p, LL, MAX_DEPTH))
			light_visibility += 1.0 / N;
		else if (i == N/3 && light_visibility == 0.0)
			break;
	}
	return light_visibility;
}

vec3 calc_direct_light(vec3 p, vec3 n, vec3 albedo, vec2 cur_pixel, int N) {
	vec3 color = vec3(0);
	for (int i=0; i<num_lights; i++) {
		float f;
		if (light[i].radius > 0) {
			// point light
			vec3 LP = light[i].pos.xyz;
			vec3 L = normalize(LP - p);
			if (dot(n, L) < 0)
				continue;
			float d = length(LP - p);
			float light_radius = 10.0;
			float light_visibility = calc_light_visibility_point(p + n * 0.01, LP, light_radius, 1);//N);
			f = max(-dot(n, L), 0.05) * light_visibility / pow(d, 2);

		} else {
			// directional
			vec3 L = light[i].dir.xyz;
			if (dot(n, L) > 0)
				continue;
			float light_visibility = calc_light_visibility_directional(p + n * 0.01, L, 0.03, N);
			f = max(-dot(n, L), 0.05) * light_visibility;
		}
		color += f * albedo * light[i].color.rgb;
	}
	return color;
}

</Module>
