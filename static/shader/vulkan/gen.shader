<Layout>
	version = 460
	extensions = GL_NV_ray_tracing
	bindings = [[acceleration-structure,image,buffer,buffer]]
</Layout>

<RayGenShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

// packed std140
struct UniformParams {
	// Camera
	vec4 camPos;
	vec4 camDir;
	vec4 camUp;
	vec4 camSide;
	vec4 camNearFarFov;
	mat4 m;
};


layout(set=0, binding=0)         uniform accelerationStructureNV scene;
layout(set=0, binding=1, rgba16f) uniform image2D image;
layout(set=0, binding=2, std140) uniform MoreData {
	mat4 iview;
	vec4 background;
	int num_triangles;
	int num_lights;
} push;
layout(set=0, binding=3, std140) uniform Vertices { float v[]; } vertices;

/*layout(set = 0,      binding = 2)     uniform AppData {
    UniformParams Params;
};*/

layout(location = 0) rayPayloadNV RayPayload ray;

const vec3 light_pos = vec3(-1.7, 1.6, -1.1);
const vec3 light_rad = vec3(0,0,10);
const float light_radius = 0.1; // for shadow

const int NUM_REFLECTIONS = 15;
const int NUM_SHADOW_SAMPLES = 10;


vec3 CalcRayDir(vec2 screenUV, float aspect) {
	vec3 u = (push.iview * vec4(1,0,0,0)).xyz;
	vec3 v = (push.iview * vec4(0,1,0,0)).xyz;
	vec3 dir = (push.iview * vec4(0,0,1,0)).xyz;

	const float planeWidth = 0.7;//tan(Params.camNearFarFov.z * 0.5f);

	u *= (planeWidth * aspect);
	v *= planeWidth;

	const vec3 rayDir = normalize(dir + (u * screenUV.x) - (v * screenUV.y));
	return rayDir;
}

float rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}

vec3 rand_dir(vec3 p) {
	vec3 v = vec3(rand3d(p)*2-1, rand3d(p + vec3(0.43767,0.92546,0.7536))*2-1, rand3d(p + vec3(0.2695,0.976234,0.1567))*2-1);
	return normalize(v);
}

void main() {
	const vec2 curPixel = vec2(gl_LaunchIDNV.xy);
	const vec2 bottomRight = vec2(gl_LaunchSizeNV.xy - 1);

	const vec2 uv = (curPixel / bottomRight) * 2.0 - 1.0;

	const float aspect = float(gl_LaunchSizeNV.x) / float(gl_LaunchSizeNV.y);
	
	const float max_depth = 20000.0;

	vec3 origin = (push.iview * vec4(0,0,0,1)).xyz;
	vec3 direction = CalcRayDir(uv, aspect);
#if 1
	vec3 out_color = push.background.rgb;
	

	// scene,flags,cull mask, hit, stride, miss, origin, t0, dir, t1, payload location
	
	traceNV(scene, gl_RayFlagsOpaqueNV, 0xff, 1, 1, 1, origin, 0.0, direction, max_depth, 0);
	if (ray.pos_and_dist.w > 0) {
		out_color = vec3(1,0,0);//ray.emission.rgb;
	
		#if 0
		vec3 p = ray.pos_and_dist.xyz;
		vec3 n = ray.normal_and_id.xyz;
		vec3 albedo = ray.albedo.rgb;
	
	
		// reflections...
		for (int i=0; i<NUM_REFLECTIONS; i++) {
			vec3 dir = normalize(rand_dir(p + vec3(curPixel,1)*i*0.732538) + n);
			/*vec3 dir = rand_dir(p + vec3(i));
			if (dot(dir, n) < 0)
				dir = -dir;*/
			traceNV(scene, gl_RayFlagsOpaqueNV, 0xff, 1, 1, 1, p + n * 0.01, 0.0, dir, max_depth, 0);
			if (ray.pos_and_dist.w > 0)
				out_color += albedo.rgb * ray.emission.rgb / NUM_REFLECTIONS;
		}
		
	
		// direct lighting
		float light_visibility = 0.0;
		for (int i=0; i<NUM_SHADOW_SAMPLES; i++) {
			vec3 lsp = light_pos + rand_dir(p + vec3(curPixel,1)*i*0.27409435) * light_radius;
			vec3 L = normalize(lsp - p);
			float d = length(lsp - p);
			traceNV(scene, gl_RayFlagsOpaqueNV, 0xff, 0, 1, 0, p + n * 0.01, 0.0, L, d, 0);
			if (ray.pos_and_dist.w < 0)
				light_visibility += 1.0 / NUM_SHADOW_SAMPLES;
		}
		vec3 L = normalize(light_pos - p);
		float d = length(light_pos - p);
		out_color += light_visibility * (albedo.rgb * light_rad) / pow(d, 2) * abs(dot(n, L));
		#endif
	}
	
	imageStore(image, ivec2(gl_LaunchIDNV.xy), vec4(out_color,1.0));
#else
	imageStore(image, ivec2(gl_LaunchIDNV.xy), vec4(direction,1.0));
	//imageStore(image, ivec2(gl_LaunchIDNV.xy), vec4(push.background.rgb,1.0));
#endif
}
</RayGenShader>
