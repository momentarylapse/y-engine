<Layout>
	version = 430
	extensions = GL_EXT_buffer_reference2,GL_EXT_scalar_block_layout
	bindings = [[image,buffer,buffer]]
	pushsize = 96
</Layout>
<ComputeShader>

struct Vertex {
	vec3 p;
	vec3 n;
	vec2 uv;
};
layout(buffer_reference, scalar) readonly buffer XVertices { Vertex v[256]; };

struct Mesh {
	mat4 matrix;
	vec4 albedo;
	vec4 emission;
	XVertices vertices;
	XVertices indices_dummy;
	int num_triangles, _a, _b, _c;
};

layout(push_constant, std140) uniform PushConstants {
	mat4 iview;
	vec4 background;
	int num_triangles;
	int num_lights;
	int num_meshes;
	int _a;
	int out_width;
	int out_height;
	float out_ratio;
	int _b;
} push;



struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};

layout(binding=0, rgba16f) uniform writeonly image2D image;
layout(binding=1, std430) uniform MeshData { Mesh mesh[256]; };
layout(binding=2) uniform LightData { Light light[32]; };
layout(local_size_x=16, local_size_y=16) in;

float rand(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}

vec3 rand3d(vec3 p) {
	return vec3(rand(p), rand(p + vec3(123.2, 41.41, 0.31134)), rand(2*p + vec3(1,2,3))) * 2 - 1;
}

struct TriaHitData {
	vec3 p, n;
	float f, g;
	float t;
};

bool trace_tria(vec3 p0, vec3 dir, vec3 a, vec3 b, vec3 c, out TriaHitData thd) {
	vec3 u0 = dir;
	vec3 v0 = cross(dir, p0);
	vec3 u1 = b - a;
	vec3 v1 = cross(b, a);
	vec3 u2 = c - b;
	vec3 v2 = cross(c, b);
	vec3 u3 = a - c;
	vec3 v3 = cross(a, c);
	float x = dot(u0, v1) + dot(u1, v0);
	float y = dot(u0, v2) + dot(u2, v0);
	float z = dot(u0, v3) + dot(u3, v0);

	if (x*y < 0 || x*z < 0)
	//if ((x < 0) || (y < 0) || (z < 0))
		return false;
	
	//hit.p = 
	vec3 n = normalize(cross(u1, u2));
	if (dot(n, dir) > 0)
		n = - n;
	vec4 pp = vec4(-cross(n, v0) + dot(n, a) * dir, dot(n, dir));
	vec3 p = pp.xyz / pp.w;
	float t = dot(p - p0, dir);
	if (t <= 0)
		return false;
	
	thd.n = n;
	thd.p = p;
	thd.t = t;
	return true;
}

struct HitData {
	int index; // tria
	int mesh;
	TriaHitData thd;
};

bool trace(vec3 p0, vec3 dir, out HitData hd) {
	hd.thd.t = 1000000;
	bool hit = false;
	TriaHitData thd;
	for (int k=0; k<push.num_meshes; k++) {
		Mesh m = mesh[k];
		for (int i=0; i<m.num_triangles; i++) {
			vec3 a = (m.matrix * vec4(m.vertices.v[i*3].p, 1)).xyz;
			vec3 b = (m.matrix * vec4(m.vertices.v[i*3+1].p, 1)).xyz;
			vec3 c = (m.matrix * vec4(m.vertices.v[i*3+2].p, 1)).xyz;
			if (trace_tria(p0, dir, a, b, c, thd)) {
				if (thd.t < hd.thd.t) {
					hit = true;
					vec3 n = (m.matrix * vec4(m.vertices.v[i*3].n, 0)).xyz;
					if (dot(n, dir) > 0)
						n = -n;
					thd.n = n;
					hd.thd = thd;
					hd.index = i;
					hd.mesh = k;
				}
			}
		}
	}
	return hit;
}

vec3 get_emission(int index) {
	return mesh[index].emission.rgb;
}

vec3 get_albedo(int index) {
	return mesh[index].albedo.rgb;
}

float get_roughness(int index) {
	return mesh[index].albedo.a;
}

float calc_light_visibility(vec3 p, vec3 sun_dir, int N) {
	HitData hd_temp;
	float light_visibility = 0.0;
	for (int i=0; i<N; i++)
		if (!trace(p, -normalize(sun_dir + 0.02 * rand3d(p + vec3(i,2*i,3*i))), hd_temp))
			light_visibility += 1.0 / N;
		else if (i == 8 && light_visibility == 0.0)
			break;
	return light_visibility;
}

vec3 calc_direct_light(vec3 albedo, vec3 p, vec3 n, int N) {
	vec3 color = vec3(0);

	for (int i=0; i<push.num_lights; i++) {
		if (light[i].radius < 0) {
			// directional
			vec3 L = light[i].dir.xyz;
			float light_visibility = calc_light_visibility(p + n * 0.1, L, N);
			float f = max(-dot(n, L), 0.1) * light_visibility;
			color += f * albedo * light[i].color.rgb;
		}
	}
	return color;
}

vec3 calc_bounced_light(vec3 p, vec3 n, vec3 eye_dir, vec3 albedo, float roughness) {
	int N = 50;
	
	vec3 refl = reflect(eye_dir, n);
	
	HitData hd;
	vec3 color = vec3(0);
	for (int i=0; i<N; i++) {
		vec3 dir = mix(refl, normalize(n + 0.7 * rand3d(p + vec3(i,2*i,3*i))), roughness);
		if (trace(p, dir, hd)) {
			color += albedo * calc_direct_light(get_albedo(hd.mesh), hd.thd.p, hd.thd.n, 1) / N;
			color += get_emission(hd.mesh) / N;
		}
	}
	return color;
}

void main() {
	ivec2 store_pos = ivec2(gl_GlobalInvocationID.xy);
		
	vec2 r = vec2(gl_GlobalInvocationID.xy) / vec2(push.out_width, push.out_height);
	vec3 dir = normalize(vec3((r.x - 0.5) * push.out_ratio, 0.5 - r.y, 1));
	dir = (push.iview * vec4(dir,0)).xyz;
	vec3 cam_pos = (push.iview * vec4(0,0,0,1)).xyz;

	HitData hd;
	if (trace(cam_pos, dir, hd)) {
		vec3 albedo = get_albedo(hd.mesh);
		
		vec3 color = get_emission(hd.mesh);
		
		// direct sunlight
		color += calc_direct_light(albedo, hd.thd.p, hd.thd.n, 20);
		
		color += calc_bounced_light(hd.thd.p + hd.thd.n * 0.1, hd.thd.n, dir, albedo, get_roughness(hd.mesh));
		
		imageStore(image, store_pos, vec4(color,1));
	} else {
		// background
		imageStore(image, store_pos, push.background);
	}
}
</ComputeShader>
