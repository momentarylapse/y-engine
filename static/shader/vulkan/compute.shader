<Layout>
	version = 430
	bindings = [[image,buffer]]
	pushsize = 4
</Layout>
<ComputeShader>

layout(push_constant) uniform PushConstants {
	vec4 cam_pos;
	int num_triangles;
} push;
layout(set=0, binding=0, rgba8) uniform writeonly image2D image;
//layout(rgba8,location=0) uniform writeonly image2D tex;
layout(binding=1) uniform Triangles { vec4 vertex[65536]; };
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
	bool hit = false;
	TriaHitData thd;
	for (int i=0; i<push.num_triangles; i++) {
		vec3 a = vertex[i*3].xyz;
		vec3 b = vertex[i*3+1].xyz;
		vec3 c = vertex[i*3+2].xyz;
		if (trace_tria(p0, dir, a, b, c, thd)) {
			hit = true;
			hd.thd = thd;
		}
	}
	return hit;
}

void main() {
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
		
	vec2 r = vec2(gl_GlobalInvocationID.xy) / imageSize(image);
	//vec3 cam_pos = vec3(0,0,-500);
	vec3 dir = normalize(vec3(r.x - 0.5, 0.5 - r.y, 1));
	
	vec3 sun_dir = normalize(vec3(0.2,0.1,1));
	
	HitData hd;
	if (trace(push.cam_pos.xyz, dir, hd)) {
		//imageStore(image, storePos, vec4(hd.thd.p/100,1));
		float light_visibility = 0.0;
		
		HitData hd2;
		int N = 5;
		for (int i=0; i<N; i++)
			if (!trace(hd.thd.p + hd.thd.n * 0.1, -normalize(sun_dir + 0.05 * rand3d(hd.thd.p)), hd2))
				light_visibility += 0.2;//1.0 / N;
		
		float f = max(-dot(hd.thd.n, sun_dir), 0.2) * light_visibility;
		imageStore(image, storePos, vec4(f,f,f,1));
	} else {
		// background
		imageStore(image, storePos, vec4(0.2,0,0,1));
	}
}
</ComputeShader>
