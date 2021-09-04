<Layout>
	bindings = [[buffer,buffer,buffer,sampler,sampler,sampler,sampler,sampler]]
	pushsize = 76
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_tex_coord;

void main() {
	gl_Position = vec4(in_position, 1.0);
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
//#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 1) uniform sampler2D tex;
layout(binding = 3) uniform sampler2D tex0;//sampler_color;
layout(binding = 4) uniform sampler2D tex1;//sampler_emission;
layout(binding = 5) uniform sampler2D tex2;//sampler_pos;
layout(binding = 6) uniform sampler2D tex3;//sampler_normal;
layout(binding = 7) uniform sampler2D tex4;//sampler_z;
layout(binding = 8) uniform sampler2D tex5;//sampler_shadow;
layout(binding = 9) uniform sampler2D tex6;
//layout(binding = 7) uniform sampler2DShadow sampler_shadow;

#define tex_albedo   tex0
#define tex_emission tex1
#define tex_pos      tex2
#define tex_normal   tex3
#define tex_z        tex4
#define tex_shadow0  tex5
#define tex_shadow1  tex6

#import lighting

struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;




//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


uniform vec3 eye_pos = vec3(0,0,0);
uniform vec2 resolution_scale;


const int NSSAO = 16;
//uniform vec3 ssao_samples[NSSAO];
uniform SSAO {
	vec3 ssao_samples[NSSAO];
};
uniform float ambient_occlusion_bias = 0.02;
uniform float ambient_occlusion_radius = 10;

vec2 project_onto_texture(vec3 pv) {
	vec4 pp = matrix.project * vec4(pv,1);
	pp.xyz /= pp.w;
	pp.xyz = pp.xyz * 0.5 + 0.5;
	return pp.xy * resolution_scale + vec2(0,1-resolution_scale.y);
}

vec3 project_onto_z(vec3 pv) {
	vec2 q_uv = project_onto_texture(pv);
	return texture(tex_pos, q_uv).xyz;
}

float get_ambient_occlusion(vec3 p, vec3 n) {
	if (ambient_occlusion_radius <= 0)
		return 0;

	vec3 pv = p;
	//vec4 pp = matrix.project * vec4(p,1);
	//pp.xyz /= pp.w;
	
	vec3 nv = n;
	
	vec3 randomVec = vec3(_surf_rand3d(pv)*2-1, _surf_rand3d(p + n)*2-1, 0);
	vec3 tangent   = normalize(randomVec - nv * dot(nv, randomVec));
	vec3 bitangent = cross(nv, tangent);
	mat3 TBN       = mat3(tangent, bitangent, nv);
	
	float occlusion = 0;
	float bias = ambient_occlusion_bias;
	float R = ambient_occlusion_radius;
	float count = 0;
	for (int i=0; i<NSSAO; i++) {
		vec3 dp = TBN * ssao_samples[i];
		//dp = ssao_samples[i];
		if (dot(dp, n) < 0.3)
			continue;
		
		
		vec3 spv = p + dp*R;
		vec3 sqv = project_onto_z(spv);
		
		float rangeCheck = smoothstep(0.0, 1.0, R / abs(pv.z - sqv.z));
		occlusion += (sqv.z <= spv.z + bias ? 1.0 : 0.0) * rangeCheck;
		
		count += 1.0;
	}
	return occlusion / count;
}


void main() {

	vec4 albedo = texture(tex_albedo, in_tex_coord);
	vec4 em = texture(tex_emission, in_tex_coord);
	vec3 emission = em.rgb;
	vec4 nr = texture(tex_normal, in_tex_coord);
	vec3 n = normalize(nr.xyz);
	float roughness = nr.w;
	float metal = em.a;
	vec3 p = texture(tex_pos, in_tex_coord).xyz;
	
	if (albedo.a < 0) {
		discard;
		return;
	}
	
	if (metal > 0.9 && roughness < 0.2) {
		vec3 dir = reflect(normalize(p), n);
		dir /= length(dir.xy);
		
		vec3 q = p + n*0.5;
		/*if (n.z < -0.9) {
			out_color = vec4(1,0,0,1);
			return;
		}*/
		for (float t=0.0; t<2000; t+=0.1) {
			q = p + n*0.5 + dir * t;
			vec2 quv = project_onto_texture(q);
			if (quv.x < 0 || quv.x > resolution_scale.x || quv.y < 1-resolution_scale.y || quv.y > 1) {
				out_color = vec4(1,0,1,1);
				return;
			}
			vec3 qq = project_onto_z(q);
			if (qq.z < q.z) {
				//out_color = vec4(1,1,0,1) * t;
				out_color = texture(tex_albedo, quv) * min(sqrt(t) * 0.2, 2);
				return;
			}
		}
		out_color = vec4(1,0,0,1);
		return;
	}
	
	float occlusion = get_ambient_occlusion(p, n);
	
	out_color = perform_lighting(p, n, albedo, vec4(emission, 1), metal, roughness, occlusion, eye_pos);
	out_color.rgb *= (1-occlusion);
	//out_color.rgb = pp.xyz;
	//out_color.rgb = vec3(1-occlusion);
	
//	out_color.rgb = pow(out_color.rgb, vec3(1.0 / 2.2)); // gamma correction
	// should happen in post-processing stage....
}
</FragmentShader>
