<Layout>
	version = 460
	extensions = GL_NV_ray_tracing,GL_EXT_nonuniform_qualifier
</Layout>


<RayClosestHitShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

layout(location = 0) rayPayloadInNV RayPayload ray;
                     hitAttributeNV vec2 hit_attribs;

void main() {
	const float id = float(gl_InstanceCustomIndexNV);

	ray.pos_and_dist = vec4(0,0,0, gl_HitTNV);
	//ray.color = barycentrics;
	//ray.normal_and_id = vec4(0,0,0, id);
	ray.emission.rgb = vec3(0,0,0);
}
</RayClosestHitShader>

<RayMissShader>

struct RayPayload {
	vec4 pos_and_dist;
	vec4 normal_and_id;
	vec4 albedo;
	vec4 emission;
};

layout(location = 0) rayPayloadInNV RayPayload ray;

void main() {
	//const vec3 bg = vec3(0.312f, 0.596f, 1.0f);
	ray.pos_and_dist = vec4(0,0,0, -1);
	//ray.color = vec3(cos(gl_WorldRayDirectionNV*2)*0.4+0.6);
	//ray.normal_and_id = vec4(0.0f);
}
</RayMissShader>