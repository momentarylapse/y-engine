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

layout (local_size_x = 16, local_size_y = 16) in;


float rand2d(vec2 p) {
	return fract(sin(dot(p ,vec2(12.9898,78.233))) * 43758.5453);
}

int rgb_to_bin(vec3 c) {
	float luminance = dot(c.rgb, vec3(0.2125, 0.7154, 0.0721));
	if (luminance < 0.001)
		return 0;
	float llum = clamp((log2(luminance) + 10.0) / 20.0, 0.0, 1.0);
	return int(llum * 254.0 + 1.0);
}

void main() {
	float u = rand2d(vec2(gl_GlobalInvocationID.xy));
	float v = rand2d(vec2(gl_GlobalInvocationID.xy));
	ivec2 i = ivec2(u * width, v * height);
	
	//vec4 c = vec4(u,v,1,1);
	vec4 c = texelFetch(tex0, i, 0);
	int bin = rgb_to_bin(c.rgb);
	atomicAdd(hist[bin], 1);
	//hist[bin] += 1;
}

</ComputeShader>