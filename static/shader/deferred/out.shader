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
#define tex_shadow0  tex5
#define tex_shadow1  tex6

#import lighting

struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;




//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


uniform vec3 eye_pos;
uniform vec2 resolution_scale;


const int NSSAO = 32;
//uniform vec3 ssao_samples[NSSAO];
uniform SSAO {
	vec3 ssao_samples[NSSAO];
};

//float ambient_occlusion() {
//}


void main() {

	vec4 albedo = texture(tex_albedo, in_tex_coord);
	vec4 emission = texture(tex_emission, in_tex_coord);
	vec4 nr = texture(tex_normal, in_tex_coord);
	vec3 n = normalize(nr.xyz);
	float roughness = nr.w;
	float metal = 0;
	vec3 p = texture(tex_pos, in_tex_coord).xyz;
	
	vec3 pv = (matrix.view * vec4(p,1)).xyz;
	vec4 pp = matrix.project * matrix.view * vec4(p,1);
	pp.xyz /= pp.w;
	/*out_color.rgb = vec3((matrix.view * vec4(p,1)).z/1000);
	out_color.rgb = vec3((matrix.view * vec4(p,1)).xyz/1000);
	out_color.a = 1;
	return;*/
	
	//vec3 nv = mat3(matrix.view) * n;
	
	float occlusion = 0;
	float bias = 0.02;//.001;
	float R = 20;
	for (int i=0; i<NSSAO; i++) {
		vec3 dp;// = vec3(_surf_rand3d(p), _surf_rand3d(p + n), _surf_rand3d(p + n*2));
		//dp = normalize(dp * 2 - 1);
		
		/*vec3 randomVec = vec3(_surf_rand3d(pv)*2-1, _surf_rand3d(p + n)*2-1, 0);
		vec3 tangent   = normalize(randomVec - nv * dot(nv, randomVec));
		vec3 bitangent = cross(nv, tangent);
		mat3 TBN       = mat3(tangent, bitangent, nv);
		dp = TBN * ssao_samples[i];*/
		dp = ssao_samples[i];
		if (dot(dp, n) < 0)
			continue;
		
		
		vec4 spv = matrix.view * vec4(p + dp*R,1);
		vec4 spp = matrix.project * spv;
		spp.xyz /= spp.w;
		spp.xyz = spp.xyz * 0.5 + 0.5;
		float aa = 1.0;
		vec2 q_uv = (1-aa)*in_tex_coord + aa*spp.xy /* * resolution_scale + vec2(0,1-resolution_scale.y) */;
		vec3 sq = texture(tex_pos, q_uv).xyz;
		vec3 sqv = (matrix.view * vec4(sq,1)).xyz;
		
		//float prev_depth = sqv.z;//texture(tex4, pp.xy * resolution_scale + vec2(0,1-resolution_scale.y)).r;
		float rangeCheck = smoothstep(0.0, 1.0, R / abs(pv.z - sqv.z));
		occlusion += (sqv.z <= spv.z + bias ? 1.0 : 0.0) * rangeCheck;
		
	
	//out_color.rgb = sqv.xyz/100;//(sqv.xyz - spv.xyz)/100;
	//out_color.rgb = sq/1000;//(sqv.xyz - spv.xyz)/100;
	//out_color.rgb = (spv.xyz - sqv.xyz)/1000;
	//out_color.rgb = vec3(prev_depth/1000);
	//out_color.rg = abs(q_uv - in_tex_coord)*1000;
	//out_color.rg = in_tex_coord;
	//out_color.b = 0;
	
	//out_color.a = 1;
	//out_color.rgb = vec3(sqv.z/1000);
	//out_color.rgb = vec3(clamp((sqv.z - spv.z)/200, 0, 1));
	//out_color.rgb = tangent;
	//return;
	//out_color.rgb = (sqv.xyz - spv.xyz)/R;//vec3((prev_depth - spv.z)/1000+0.5);
	//return;
	
	}
	occlusion /= NSSAO/2;
	out_color.rgb = vec3(clamp(1-occlusion,0,1));
	return;
	
	out_color = emission;
	out_color += perform_lighting(p, n, albedo, emission, metal, roughness, eye_pos) * (1-occlusion);
	//out_color.rgb = pp.xyz;
	
//	out_color.rgb = pow(out_color.rgb, vec3(1.0 / 2.2)); // gamma correction
	// should happen in post-processing stage....
}
</FragmentShader>
