<Layout>
	name = vertex-animated
</Layout>
<Module>

#ifdef vulkan
struct Matrices {
	mat4 model;
	mat4 view;
	mat4 project;
};

struct Material {
	vec4 albedo, emission;
	float roughness, metal;
	int _dummy1, _dummy2;
};

layout(binding = 0) uniform ParameterData {
	Matrices matrix;
	Material material;
	int num_lights;
	int shadow_index;
	int _dummy3, _dummy4;
	mat4 bone_matrix[128];
};
//layout(binding = 2) uniform BoneData {  };
#else
struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;
/*layout(binding = 7)*/ uniform BoneData { mat4 bone_matrix[128]; };
#endif


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in ivec4 in_bone_index;
//layout(location = 3) in float in_bone_index;
layout(location = 4) in vec4 in_bone_weight;

layout(location = 0) out vec4 out_pos; // world space
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

void main() {
	mat4 bm = bone_matrix[in_bone_index.x] * in_bone_weight.x;
	bm += bone_matrix[in_bone_index.y] * in_bone_weight.y;
	bm += bone_matrix[in_bone_index.z] * in_bone_weight.z;
	bm += bone_matrix[in_bone_index.w] * in_bone_weight.w;
	mat4 model = matrix.model * bm;
	
	gl_Position = matrix.project * matrix.view * model * vec4(in_position, 1);
	
	//out_normal = (matrix.view * model * vec4(in_normal, 0)).xyz;
	out_normal = (matrix.view * model * vec4(in_normal, 0)).xyz;
	out_uv = in_uv;
	out_pos = matrix.view * model * vec4(in_position, 1);
}

</Module>
