<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 450
#extension GL_ARB_separate_shader_objects : enable



layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_tex_coord;

void main() {
	gl_Position = mat.proj * mat.view * mat.model * vec4(in_position, 1.0);
	out_pos = mat.model * vec4(in_position, 1.0);
	out_normal = (mat.model * vec4(in_normal, 0.0)).xyz;
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 450
#extension GL_ARB_separate_shader_objects : enable


const float shininess = 20.0;
const float gloss = 0.2;

/*struct Light {
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};*/
layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;

layout(push_constant) uniform PushConstants {
	mat4 mat_model;
	vec4 emission_factor;
	vec4 eye_pos;
	vec4 xxx;
	//Light light;
};

layout(binding = 1) uniform Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
} light;

layout(binding = 2) uniform Fog {
	vec4 color;
	float distance;
} fog;

layout(binding = 3) uniform sampler2D tex;
layout(binding = 4) uniform sampler2D tex_mat;
layout(binding = 5) uniform sampler2D tex_glow;
layout(binding = 6) uniform sampler2D sampler_shadow;

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;


vec3 light_dir(vec3 p) {
	if (light.radius < 0)
		return light.dir.xyz;
	return normalize(light.pos.xyz - p);
}


float brightness(vec3 p, vec3 n) {
	// parallel
	if (light.radius < 0)
		return 1.0f;
	
	
	float d = length(p - light.pos.xyz);
	if (d > light.radius)
		return 0.0;
	float b = min(pow(1.0/d, 2), 1.0);
	
	// spherical
	if (light.theta < 0)
		return b;
	
	float t = acos(dot(light.dir.xyz, normalize(p - light.pos.xyz)));
	float tmax = light.theta;
	return b * (1 - smoothstep(tmax*0.8, tmax, t));
}

// blinn phong
float specular(vec3 p, vec3 n) {
	vec3 view_dir = normalize(p - eye_pos.xyz);
	vec3 half_dir = normalize(light_dir(p) - view_dir);

	float spec_angle = max(dot(half_dir, n), 0.0);
	return pow(spec_angle, shininess);
}

float shadow_pcf_step(vec3 p, vec2 dd, ivec2 ts) {
	vec2 d = dd / ts * 0.8;
	float shadow_z = texture(sampler_shadow, p.xy + d).r + 0.001;
	if (p.z > shadow_z)
		return 1.0;
	return 0.0;
}

float shadow_pcf(vec3 p) {
	ivec2 ts = textureSize(sampler_shadow, 0);
	float value = shadow_pcf_step(p, vec2(0,0), ts) * 2;
	value += shadow_pcf_step(p, vec2( 1.0, 1.1), ts);
	value += shadow_pcf_step(p, vec2(-0.8, 0.9), ts);
	value += shadow_pcf_step(p, vec2( 0.7,-0.8), ts);
	value += shadow_pcf_step(p, vec2(-0.5,-1.1), ts);
	return value / 6.0;
}

void main() {
	vec3 n = normalize(in_normal);
	vec3 d = normalize(eye_pos.xyz - in_pos.xyz/in_pos.w);
	vec4 t = texture(tex, in_tex_coord);
	//t = texture(sampler_shadow, in_tex_coord);
	//t.r = pow(t.r, 10);
	vec3 p = in_pos.xyz / in_pos.w;
	
	out_color = texture(tex_glow, in_tex_coord) * emission_factor;
	
	float reflectivity = 1-((1-xxx.x) * (1-exp(-pow(dot(d, n),2) * 100)));



	vec4 proj = light.proj * vec4(p,1);
	proj.xyz /= proj.w;
	proj.x = (proj.x +1)/2;
	proj.y = (proj.y +1)/2;
	
	float shadow = 1.0;
	if (proj.x > 0.01 && proj.x < 0.99 && proj.y > 0.01 && proj.y < 0.99) {
		//float shadow = shadow2d(sampler_shadow, in_tex_coord).r;
		
//		proj.xyz += 0.001;
//		shadow = texture(sampler_shadow, proj.xyz);
		
		shadow = 1.0 - shadow_pcf(proj.xyz) * light.harshness;
	}
	
	float b = brightness(p, n) * shadow;
	float lambert = max(dot(n, light_dir(p)), 0);
	
	float bb = light.harshness * lambert + (1 - light.harshness);
	out_color += t * light.color * bb * b;
	
	// specular
	if (lambert > 0) {
		float sp = gloss * specular(p, n);
		out_color += sp * light.color * b;
	}
	
	float distance = length(p - eye_pos.xyz);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	out_color.rgb = pow(out_color.rgb, vec3(1.0 / 2.2)); // gamma correction
	// should happen in post-processing stage....
}
</FragmentShader>
