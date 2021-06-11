<Layout>
	bindings = [[buffer,buffer,buffer,sampler,sampler,sampler,sampler,sampler]]
	pushsize = 76
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
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
#version 420
#extension GL_ARB_separate_shader_objects : enable


//layout(binding = 1) uniform sampler2D tex;
layout(binding = 3) uniform sampler2D tex0;//sampler_color;
layout(binding = 4) uniform sampler2D tex1;//sampler_emission;
layout(binding = 5) uniform sampler2D tex2;//sampler_pos;
layout(binding = 6) uniform sampler2D tex3;//sampler_normal;
layout(binding = 7) uniform sampler2D tex4;//sampler_shadow;
layout(binding = 8) uniform sampler2D tex5;
//layout(binding = 7) uniform sampler2DShadow sampler_shadow;

//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


uniform vec4 eye_pos;


struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};
uniform int num_lights;
uniform int shadow_index = 0;

/*layout(binding = 1)*/ uniform LightData {
	Light light[32];
};


float rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}


vec3 light_dir(Light l, vec3 p) {
	if (l.radius < 0)
		return l.dir.xyz;
	return normalize(p - l.pos.xyz);
}


float brightness(Light l, vec3 p, vec3 n) {
	// parallel
	if (l.radius < 0)
		return 1.0f;
	
	
	float d = length(p - l.pos.xyz);
	if (d > l.radius)
		return 0.0;
	float b = min(pow(1.0/d, 2), 1.0);
	
	// spherical
	if (l.theta < 0)
		return b;
	
	float t = acos(dot(l.dir.xyz, normalize(p - l.pos.xyz)));
	float tmax = l.theta;
	return b * (1 - smoothstep(tmax*0.8, tmax, t));
}

// blinn phong
/*float specular(Light l, vec3 p, vec3 n) {
	vec3 view_dir = normalize(p - eye_pos.xyz);
	vec3 half_dir = normalize(light_dir(l, p) + view_dir);

	float spec_angle = max(-dot(half_dir, n), 0.0);
	return pow(spec_angle, material.shininess);
}*/

float shadow_pcf_step(vec3 p, vec2 dd, ivec2 ts) {
	vec2 d = dd / ts * 0.8;
	vec2 tp = p.xy + d;
	float epsilon = 0.004;
	float shadow_z = texture(tex4, p.xy + d).r + epsilon;
	if (tp.x > 0.38 && tp.y > 0.38 && tp.x < 0.62 && tp.y < 0.62)
		shadow_z = texture(tex5, (p.xy - vec2(0.5,0.5))*4 + vec2(0.5,0.5) + d).r + epsilon/4;
	if (p.z > shadow_z)
		return 1.0;
	return 0.0;
}

float shadow_pcf(vec3 p) {
	ivec2 ts = textureSize(tex4, 0);
	float value = 0;//shadow_pcf_step(p, vec2(0,0), ts);
	const float R = 1.8;
	const int N = 3;
	for (int i=0; i<N; i++) {
		float phi = rand3d(p + p*i) * 2 * 3.1415;
		float r = R * sqrt(fract(phi * 235.3545));
		value += shadow_pcf_step(p, r * vec2(cos(phi), sin(phi)), ts);
	}
	//value += shadow_pcf_step(p, vec2( 1.0, 1.1), ts);
	//value += shadow_pcf_step(p, vec2(-0.8, 0.9), ts);
	//value += shadow_pcf_step(p, vec2( 0.7,-0.8), ts);
	//value += shadow_pcf_step(p, vec2(-0.5,-1.1), ts);
	return value / N;//(N+1);
}

vec3 light_add(Light l, vec3 p, vec3 n, vec3 t, bool with_shadow) {
	float shadow = 1.0;
	
	if (with_shadow) {
		vec4 proj = l.proj * vec4(p,1);
		proj.xyz /= proj.w;
		proj.x = (proj.x +1)/2;
		proj.y = (proj.y +1)/2;
		proj.z = (proj.z +1)/2;
	
		if (proj.x > 0.01 && proj.x < 0.99 && proj.y > 0.01 && proj.y < 0.99 && proj.z < 1.0) {
			shadow = 1.0 - shadow_pcf(proj.xyz) * l.harshness;
		}
	}
	
	

	float b = brightness(l, p, n) * shadow;
	float lambert = max(-dot(n, light_dir(l, p)), 0);
	
	float bb = l.harshness * lambert + (1 - l.harshness);
	vec3 col = t * l.color.rgb * bb * b;
	
	// specular
	/*if (lambert > 0 && material.specular*material.shininess > 0) {
		float sp = material.specular * specular(l, p, n);
		col += sp * l.color.rgb * b;
	}*/
	return col;
}


void main() {

	vec4 t = texture(tex0, in_tex_coord);
	vec4 e = texture(tex1, in_tex_coord); // emission
	vec3 n = normalize(texture(tex3, in_tex_coord).xyz);
	vec3 p = texture(tex2, in_tex_coord).xyz;
	
	out_color = e;

	for (int i=0; i<num_lights; i++)
		out_color.rgb += light_add(light[i], p, n, t.rgb, i == shadow_index).rgb;
	
//	out_color.rgb = pow(out_color.rgb, vec3(1.0 / 2.2)); // gamma correction
	// should happen in post-processing stage....
}
</FragmentShader>
