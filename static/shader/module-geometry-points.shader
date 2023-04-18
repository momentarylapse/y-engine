<Layout>
	name = geometry-points
</Layout>
<Module>


layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(location=0) in vec4 in_pos[];
layout(location=1) in float in_r[];
layout(location=2) in vec4 in_color[];

layout(location=0) out vec4 out_pos;
layout(location=1) out vec3 out_n;
layout(location=2) out vec2 out_uv;
layout(location=3) out vec4 out_color;

#ifdef vulkan
const vec4 source_uv = vec4(0,1,0,1);
#else
uniform float target_width = 1024, target_height = 768;
uniform float line_width = 4;
uniform vec4 source_uv = vec4(0,1,0,1);
#endif

void main() {
        /*float w0 = gl_in[0].gl_Position.w;
        float w1 = gl_in[1].gl_Position.w;
        vec2 d = (gl_in[1].gl_Position.xy / w1 - gl_in[0].gl_Position.xy / w0);
        d = vec2(d.x * target_width/2, d.y * target_height/2);
        d = vec2(d.y, -d.x) / length(d) * line_width/2;
        d = vec2(d.x / target_width*2, d.y / target_height*2);*/
	float dx = in_r[0] * 0.6;
	float dy = in_r[0];
	// TODO correct scaling!

        gl_Position = gl_in[0].gl_Position + vec4(-dx,-dy,0,0);
	out_n = vec3(0,0,-1);
        out_uv = vec2(source_uv[0], source_uv[2]);
        out_color = in_color[0];
        EmitVertex();

        gl_Position = gl_in[0].gl_Position + vec4(dx,-dy,0,0);
	out_n = vec3(0,0,-1);
        out_uv = vec2(source_uv[1], source_uv[2]);
        out_color = in_color[0];
        EmitVertex();
    
        gl_Position = gl_in[0].gl_Position + vec4(-dx,dy,0,0);
	out_n = vec3(0,0,-1);
        out_uv = vec2(source_uv[0], source_uv[3]);
        out_color = in_color[0];
        EmitVertex();

        gl_Position = gl_in[0].gl_Position + vec4(dx,dy,0,0);
	out_n = vec3(0,0,-1);
        out_uv = vec2(source_uv[1], source_uv[3]);
        out_color = in_color[0];
        EmitVertex();
    
        EndPrimitive();
}

</Module>

