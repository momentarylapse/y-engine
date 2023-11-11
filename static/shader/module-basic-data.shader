<Layout>
	version = 420
	name = basic-data
</Layout>
<Module>

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

struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};



#ifdef vulkan

/*---------------------------------------*\
  Vulkan
\*---------------------------------------*/

layout(binding = 0) uniform ParameterData {
	Matrices matrix;
	Material material;
	int num_lights;
	int shadow_index;
};
layout(binding = 1) uniform LightData {
	Light light[32];
};

layout(binding = 4) uniform sampler2D tex0;
layout(binding = 5) uniform sampler2D tex1;
layout(binding = 6) uniform sampler2D tex2;
layout(binding = 2) uniform sampler2D tex_shadow0;
layout(binding = 3) uniform sampler2D tex_shadow1;
layout(binding = 8) uniform samplerCube tex_cube;

#else

/*---------------------------------------*\
  OpenGL
\*---------------------------------------*/

layout(binding = 0) uniform sampler2D tex0;
layout(binding = 1) uniform sampler2D tex1;
layout(binding = 2) uniform sampler2D tex2;
layout(binding = 3) uniform sampler2D tex3;//sampler_shadow;
layout(binding = 4) uniform sampler2D tex4;//sampler_shadow2;
layout(binding = 5) uniform samplerCube tex_cube;//sampler_shadow2;

#define tex_shadow0 tex3
#define tex_shadow1 tex4


uniform Material material;
uniform Matrices matrix;

uniform int num_lights;
uniform int shadow_index = -1;
layout(std140) uniform LightData {
	Light light[32];
};
//uniform Fog fog;

//uniform vec3 eye_pos;
const vec3 eye_pos = vec3(0,0,0);

#endif


layout(location = 0) in vec4 in_pos; // view space
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_color;
layout(location = 0) out vec4 out_color;


const float PI = 3.141592654;



</Module>
