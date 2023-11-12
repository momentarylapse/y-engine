<Layout>
	version = 420
	name = basic-interface
</Layout>
<Module>
#import basic-interface

layout(location = 0) in vec3 in_pos;
layout(location = 1) in float in_r;
layout(location = 2) in vec4 in_col;

layout(location = 0) out vec4 out_pos; // view space
layout(location = 1) out float out_r;
layout(location = 2) out vec4 out_col;
</Module>
