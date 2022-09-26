<Layout>
	version = 430
	bindings = [[image,buffer]]
	pushsize = 4
</Layout>
<ComputeShader>

layout(push_constant) uniform PushConstants { int num_triangles; } push;
layout(set=0, binding=0, rgba8) uniform writeonly image2D image;
//layout(rgba8,location=0) uniform writeonly image2D tex;
layout(binding=1) uniform Triangles { vec4 vertex[65536]; };
layout(local_size_x=16, local_size_y=16) in;

struct TriaHitData {
	vec3 p;
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

	if ((x < 0) || (y < 0) || (z < 0))
		return false;
	
	//hit.p = 
	//thd.hit = true;
	return true;
}

struct HitData {
	int index;
	vec3 p;
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
		}
	}
	return hit;
}

void main() {
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
		
	vec2 r = vec2(gl_GlobalInvocationID.xy) / vec2(800,600);
	vec3 cam_pos = vec3(0,0,-200);
	vec3 dir = normalize(vec3(r-vec2(0.5,0.5),1));
	
	HitData hd;
	if (trace(cam_pos, dir, hd)) {
		imageStore(image, storePos, vec4(1,1,1,1));
	} else {
		float roll = 0;
		float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);
		float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;
		imageStore(image, storePos, vec4(1.0-globalCoef*localCoef, 0.0, 0.0, 0.0));
	}
}
</ComputeShader>
