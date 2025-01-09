<Layout>
	version = 430
	extensions = GL_EXT_buffer_reference2,GL_EXT_scalar_block_layout
	bindings = [[buffer,buffer,storage-buffer]]
	pushsize = 96
</Layout>
<ComputeShader>

struct Vertex {
	vec3 p;
	vec3 n;
	vec2 uv;
};
layout(buffer_reference, scalar) readonly buffer XVertices { Vertex v[256]; };

struct Request {
	vec4 p0, p1;
};

struct Mesh {
	mat4 matrix;
	vec4 albedo;
	vec4 emission;
	XVertices vertices;
	XVertices indices_dummy;
	int num_triangles, _a, _b, _c;
};

struct TriaHitData {
	vec3 p, n;
	float f, g;
	float t;
};

struct HitData {
	int index; // tria
	int mesh;
	TriaHitData thd;
};

struct Reply {
	vec4 p, n;
	vec4 fgt;
	ivec4 index;
};

layout(push_constant, std140) uniform PushConstants {
	int num_meshes;
} push;


layout(binding=0, std430) uniform RequestData { Request requests[1024][16]; };
layout(binding=1, std430) uniform MeshData { Mesh mesh[256>>3]; };
layout(binding=2, std430) buffer ReplyData { Reply replies[1024][16]; };
layout(local_size_x=16, local_size_y=16) in;

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

void main() {
	ivec2 store_pos = ivec2(gl_GlobalInvocationID.xy);
	Request req = requests[gl_GlobalInvocationID.x][gl_GlobalInvocationID.y];
	
	HitData hd;
	Reply reply;
	if (trace(req.p0.xyz, req.p1.xyz, hd)) {
		reply.p.xyz = hd.thd.p;
		reply.n.xyz = hd.thd.n;
		reply.fgt = vec4(hd.thd.f, hd.thd.g, hd.thd.t, 0);
		reply.index = ivec4(hd.index, hd.mesh, 0, 0);
	} else {
		reply.index = ivec4(-1);
	}
	replies[gl_GlobalInvocationID.x][gl_GlobalInvocationID.y] = reply;
}
</ComputeShader>
