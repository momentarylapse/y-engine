<Layout>
	version = 460
	extensions = GL_EXT_ray_tracing,GL_EXT_buffer_reference2,GL_EXT_scalar_block_layout
	bindings = [[acceleration-structure,image,buffer,buffer,buffer,buffer]]
</Layout>

<RayGenShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

// packed std140
/*struct UniformParams {
	// Camera
	vec4 camPos;
	vec4 camDir;
	vec4 camUp;
	vec4 camSide;
	vec4 camNearFarFov;
	mat4 m;
};*/

struct Light {
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
	int shadow_index;
};

layout(set=0, binding=0)          uniform accelerationStructureEXT scene;
layout(set=0, binding=1, rgba16f) uniform image2D image;
layout(set=0, binding=2, std140)  uniform MoreData {
	mat4 iview;
	vec4 background;
	int num_triangles;
	int _num_lights;
} push;
layout(set=0, binding=4) uniform LightData {
	int num_lights;
	int num_surfels;
	int _dummy0, _dummy1;
	ivec4 probe_cells;
	vec4 probe_min, probe_max;
	mat4 shadow_proj[2];
	Light light[500];
};
//layout(set=0, binding=3, std140) uniform Vertices { float v[]; } vertices;

/*layout(set = 0,      binding = 2)     uniform AppData {
    UniformParams Params;
};*/

layout(location = 0) rayPayloadEXT RayPayload ray;

const int NUM_REFLECTIONS = 50;
const int NUM_SHADOW_SAMPLES = 10;

const float MAX_DEPTH = 20000;


vec3 calc_ray_dir(vec2 screen_uv, float aspect) {
	vec3 u = (push.iview * vec4(1,0,0,0)).xyz;
	vec3 v = (push.iview * vec4(0,1,0,0)).xyz;
	vec3 dir = (push.iview * vec4(0,0,1,0)).xyz;

	const float plane_width = 0.7;//tan(Params.camNearFarFov.z * 0.5f);

	u *= (plane_width * aspect);
	v *= plane_width;

	const vec3 ray_dir = normalize(dir + (u * screen_uv.x) - (v * screen_uv.y));
	return ray_dir;
}

float rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}

vec3 rand_dir(vec3 p) {
	vec3 v = vec3(rand3d(p)*2-1, rand3d(p + vec3(0.43767,0.92546,0.7536))*2-1, rand3d(p + vec3(0.2695,0.976234,0.1567))*2-1);
	return normalize(v);
}

float calc_light_visibility_point(vec3 p, vec3 LP, float light_radius, int N) {
	float light_visibility = 0.0;
	for (int i=0; i<N; i++) {
		vec3 lsp = LP + rand_dir(p + vec3(i,2*i,3*i)) * light_radius;
		vec3 L = normalize(lsp - p);
		float d = length(lsp - p);
		traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 0, 1, 0, p, 0.0, L, d, 0);
		if (ray.pos_and_dist.w < 0)
			light_visibility += 1.0 / N;
	}
	return light_visibility;
}

float calc_light_visibility_directional(vec3 p, vec3 L, float fuzzyness, int N) {
	float light_visibility = 0.0;
	for (int i=0; i<N; i++) {
		vec3 LL = -normalize(L + fuzzyness * rand3d(p + vec3(i,2*i,3*i)));
		traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 0, 1, 0, p, 0.0, LL, MAX_DEPTH, 0);
		if (ray.pos_and_dist.w < 0)
			light_visibility += 1.0 / N;
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
			float d = length(LP - p);
			float light_radius = 10.0;
			float light_visibility = calc_light_visibility_point(p + n * 0.01, LP, light_radius, N);
			f = max(-dot(n, L), 0.05) * light_visibility / pow(d, 2);

		} else {
			// directional
			vec3 L = (push.iview * vec4(light[i].dir.xyz,0)).xyz;
			float light_visibility = calc_light_visibility_directional(p + n * 0.01, L, 0.02, N);
			f = max(-dot(n, L), 0.05) * light_visibility;
		}
		color += f * albedo * light[i].color.rgb;
	}
	return color;
}


vec3 calc_bounced_light(vec3 p, vec3 n, vec3 eye_dir, vec3 albedo, float roughness, vec2 cur_pixel) {

	vec3 refl = reflect(eye_dir, n);

	vec3 color = vec3(0);
	for (int i=0; i<NUM_REFLECTIONS; i++) {
		vec3 dir = mix(refl, normalize(n + 0.7 * rand_dir(p + vec3(cur_pixel,1)*i*0.732538)), roughness);
		traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 1, 1, 1, p + n * 0.01, 0.0, dir, MAX_DEPTH, 0);
		color += albedo * ray.emission.rgb;
		if (ray.pos_and_dist.w > 0) {
			color += albedo * calc_direct_light(ray.pos_and_dist.xyz, ray.normal_and_id.xyz, ray.albedo.rgb, cur_pixel, 1);
		}
	}
	return color / NUM_REFLECTIONS;
}


void main() {
	const vec2 cur_pixel = vec2(gl_LaunchIDEXT.xy);
	const vec2 bottom_right = vec2(gl_LaunchSizeEXT.xy - 1);

	const vec2 uv = (cur_pixel / bottom_right) * 2.0 - 1.0;

	const float aspect = float(gl_LaunchSizeEXT.x) / float(gl_LaunchSizeEXT.y);
	
	const float max_depth = 20000.0;

	vec3 origin = (push.iview * vec4(0,0,0,1)).xyz;
	vec3 direction = calc_ray_dir(uv, aspect);
#if 1
	vec3 out_color;
	

	// scene,flags,cull mask, hit, stride, miss, origin, t0, dir, t1, payload location
	
	traceRayEXT(scene, gl_RayFlagsOpaqueEXT, 0xff, 1, 1, 1, origin, 0.0, direction, max_depth, 0);
	if (ray.pos_and_dist.w > 0) {
		out_color = ray.emission.rgb;
	
		vec3 p = ray.pos_and_dist.xyz;
		vec3 n = ray.normal_and_id.xyz;
		vec3 albedo = ray.albedo.rgb;
		float roughness = ray.albedo.a;

		out_color += calc_direct_light(p, n, albedo, cur_pixel, NUM_SHADOW_SAMPLES);

		out_color += calc_bounced_light(p, n, direction, albedo, roughness, cur_pixel);
	} else {
		out_color = push.background.rgb;
	}
	
	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(out_color,1.0));
#else
	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(direction,1.0));
#endif
}
</RayGenShader>
