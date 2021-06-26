<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>

struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
/*layout(binding = 0)*/ uniform Matrix matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_pos_world;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_normal;

void main() {
	//float a = gl_InstanceID*0.5;
	//float b = gl_InstanceID*1.1;
	out_pos_world = matrix.model * vec4(in_position, 1.0);// + vec4(cos(a)*cos(b),sin(a)*cos(b),sin(b),0) * 300 * pow(gl_InstanceID, 0.4);
	gl_Position = matrix.project * matrix.view * out_pos_world;
	out_normal = (matrix.model * vec4(in_normal, 0.0)).xyz;
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>

#import surface

struct Material { vec4 albedo, emission; float roughness, metal; };
uniform Material material;


layout(binding = 3) uniform sampler2D tex0;
layout(binding = 4) uniform sampler2D tex1;//_mat;
layout(binding = 5) uniform sampler2D tex2;//_glow;


void main() {
	vec3 n = normalize(in_normal);
	vec4 di = texture(tex0, in_uv) * material.albedo;
	vec4 em = texture(tex2, in_uv) * material.emission;
	
	surface_out(n, di, em, material.metal, material.roughness);
}

</FragmentShader>
