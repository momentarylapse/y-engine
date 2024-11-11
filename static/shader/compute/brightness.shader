<Layout>
	version = 430
</Layout>
<ComputeShader>

uniform int width;
uniform int height;

layout(binding=0) uniform sampler2D tex0;

layout(std430, binding=1) buffer Histogram {
	int hist[256];
};

shared int hist_g[256];

layout (local_size_x = 16, local_size_y = 16) in;

int rgb_to_bin(vec3 c) {
	float luminance = dot(c.rgb, vec3(0.2125, 0.7154, 0.0721));
	if (luminance < 0.001)
		return 0;
	float llum = clamp((log2(luminance) + 10.0) / 20.0, 0.0, 1.0);
	return int(llum * 254.0 + 1.0);
}

void main() {
	hist_g[gl_LocalInvocationIndex] = 0;
	//groupMemoryBarrier();
	barrier();

	if ((gl_GlobalInvocationID.x < width) && (gl_GlobalInvocationID.y < height)) {
		vec4 c = texelFetch(tex0, ivec2(gl_GlobalInvocationID.xy), 0);
		int bin = rgb_to_bin(c.rgb);
		atomicAdd(hist_g[bin], 1);
	}
	
	//groupMemoryBarrier();
	barrier();
	
	atomicAdd(hist[gl_LocalInvocationIndex], hist_g[gl_LocalInvocationIndex]);
}

</ComputeShader>