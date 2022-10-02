<Layout>
	version = 430
	bindings = [[image,buffer,buffer]]
	pushsize = 84
</Layout>
<ComputeShader>

layout(push_constant, std140) uniform PushConstants {
	mat4 iview;
	vec4 background;
	int num_triangles;
} push;
layout(set=0, binding=0, rgba16f) uniform writeonly image2D image;
//layout(rgba8,location=0) uniform writeonly image2D tex;
layout(binding=1, std140) uniform Vertices { vec4 vertex[256]; };
layout(binding=2, std140) uniform Materials { vec4 material[256*2]; };
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
	int index;
	TriaHitData thd;
};

bool trace(vec3 p0, vec3 dir, out HitData hd) {
	hd.thd.t = 1000000;
	bool hit = false;
	TriaHitData thd;
	for (int i=0; i<push.num_triangles; i++) {
		vec3 a = vertex[i*3].xyz;
		vec3 b = vertex[i*3+1].xyz;
		vec3 c = vertex[i*3+2].xyz;
		if (trace_tria(p0, dir, a, b, c, thd)) {
			if (thd.t < hd.thd.t) {
				hit = true;
				hd.thd = thd;
				hd.index = i;
			}
		}
	}
	return hit;
}

vec3 get_emission(int index) {
	return material[index * 2 + 1].rgb;
}

vec3 get_albedo(int index) {
	return material[index * 2].rgb;
}

float calc_light_visibility(vec3 p, vec3 sun_dir) {
	int N = 20;
	
	HitData hd_temp;
	float light_visibility = 0.0;
	for (int i=0; i<N; i++)
		if (!trace(p, -normalize(sun_dir + 0.02 * rand3d(p + vec3(i,2*i,3*i))), hd_temp))
			light_visibility += 1.0 / N;
		else if (i == 8 && light_visibility == 0.0)
			break;
	return light_visibility;
}

vec3 calc_bounced_emission(vec3 p, vec3 n) {
	int N = 50;
	
	HitData hd_temp;
	vec3 color = vec3(0);
	for (int i=0; i<N; i++)
		if (trace(p, normalize(n + 0.7 * rand3d(p + vec3(i,2*i,3*i))), hd_temp))
			color += get_emission(hd_temp.index) / N;	
	return color;
}

void main() {
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
		
	vec2 r = vec2(gl_GlobalInvocationID.xy) / imageSize(image);
	vec3 dir = normalize(vec3(r.x - 0.5, 0.5 - r.y, 1));
	dir = (push.iview * vec4(dir,0)).xyz;
	vec3 cam_pos = (push.iview * vec4(0,0,0,1)).xyz;
	
	vec3 sun_dir = normalize(vec3(0.3,0.05,1));
	vec3 sun_color = vec3(1,1,1);

	HitData hd;
	if (trace(cam_pos, dir, hd)) {
		//imageStore(image, storePos, vec4(hd.thd.p/100,1));
		vec3 albedo = get_albedo(hd.index);
		
		// direct sunlight
		float light_visibility = calc_light_visibility(hd.thd.p + hd.thd.n * 0.1, sun_dir);
		
		vec3 color = get_emission(hd.index);
		float f = max(-dot(hd.thd.n, sun_dir), 0.1) * light_visibility;
		color += f * albedo * sun_color;
		
		color += calc_bounced_emission(hd.thd.p + hd.thd.n * 0.1, hd.thd.n);
		
		imageStore(image, storePos, vec4(color,1));
	} else {
		// background
		imageStore(image, storePos, push.background);
	}
}
</ComputeShader>
