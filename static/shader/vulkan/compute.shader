<Layout>
	version = 430
	bindings = [[image]]
	pushsize = 4
</Layout>
<ComputeShader>

layout(push_constant) uniform PushConstants { float roll; } push;
layout(set=0, binding=0, rgba8) uniform writeonly image2D image;
//layout(rgba8,location=0) uniform writeonly image2D tex;
layout(local_size_x=16, local_size_y=16) in;

void main() {
	float roll = 0;
	ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
	float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);
	float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;
	imageStore(image, storePos, vec4(1.0-globalCoef*localCoef, 0.0, 0.0, 0.0));
	
	/*float x = 0.0;
	for (int i=0;i<10; i++)
		if (i < cos(i))
			x += sin(gl_GlobalInvocationID.x * 0.02);
	imageStore(image, storePos, vec4(x, x, 0, 0.0));*/
	//imageStore(image, storePos, vec4(0, 1, 0, 0.0));
}
</ComputeShader>
