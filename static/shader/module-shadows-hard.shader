<Layout>
	version = 420
	name = shadows
</Layout>
<Module>

// import basic-data first!


// amount of shadow
float _shadow_sample_z(vec3 p, ivec2 ts) {
	vec2 tp = p.xy;
	float epsilon = 0.004;
	if (tp.x > 0.39 && tp.y > 0.39 && tp.x < 0.61 && tp.y < 0.61)
		return texture(tex_shadow0, (p.xy - vec2(0.5,0.5))*4 + vec2(0.5,0.5)).r + epsilon;
	if (tp.x > 0.05 && tp.y > 0.05 && tp.x < 0.95 && tp.y < 0.95)
		return texture(tex_shadow1, p.xy).r + epsilon;
	return 1.0;
}

float _shadow_hard(vec3 p) {
	ivec2 ts = textureSize(tex_shadow0, 0);
	float z = _shadow_sample_z(p, ts);
	if (z < p.z)
		return 1;
	return 0;
}

vec3 _light_proj(Light l, vec3 p) {
	vec4 proj = shadow_proj[0] * vec4(p,1);
	proj.xyz /= proj.w;
	proj.x = (proj.x +1)/2;
	proj.y = (proj.y +1)/2;
#ifdef vulkan
	proj.y = 1 - proj.y;
#endif
	//proj.z = (proj.z +1)/2;
	return proj.xyz;
}

float _shadow_factor(Light l, vec3 p) {
	vec3 proj = _light_proj(l, p);
	
	if (proj.x > 0.01 && proj.x < 0.99 && proj.y > 0.01 && proj.y < 0.99 && proj.z < 1.0)
		return 1.0 - _shadow_hard(proj) * l.harshness;
	
	return 1.0;
}

</Module>
