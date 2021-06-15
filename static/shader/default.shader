<Layout>
	version = 420
</Layout>
<VertexShader>
layout(column_major) struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 0) out vec4 out_pos; // world space
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_normal;
void main() {
	gl_Position = matrix.project * matrix.view * matrix.model * vec4(in_position, 1);
	out_normal = (matrix.model * vec4(in_normal, 0)).xyz;
	out_uv = in_uv;
	out_pos = matrix.model * vec4(in_position,1);
}
</VertexShader>
<FragmentShader>
#import surface
struct Material { vec4 albedo, emission; float roughness, metal; };
uniform Material material;
uniform sampler2D tex0;
layout(column_major) struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;

void main() {
	// Mesh
	vec3 tmp1 = in_pos.xyz / in_pos.w;
	vec3 tmp2 = normalize(in_normal);
	vec2 tmp3 = in_uv;
	// Material
	vec4 tmp4 = material.albedo;
	float tmp5 = material.roughness;
	float tmp6 = material.metal;
	vec4 tmp7 = material.emission;
	// Texture0
	vec4 tmp8 = texture(tex0, tmp3);
	// Multiply
	vec4 tmp9 = tmp8 * tmp4;
	// SurfaceOutput
	surface_out(tmp2, tmp9, tmp7, tmp6, tmp5);
}
</FragmentShader>
