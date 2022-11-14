<Layout>
	name = geometry-lines
</Layout>
<Module>


layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

layout(location=0) in vec4 in_color[];
layout(location=0) out vec4 out_color;

uniform float target_width = 1024, target_height = 768;
uniform float line_width = 4;

void main() {
        float w0 = gl_in[0].gl_Position.w;
        float w1 = gl_in[1].gl_Position.w;
        vec2 d = (gl_in[1].gl_Position.xy / w1 - gl_in[0].gl_Position.xy / w0);
        d = vec2(d.x * target_width/2, d.y * target_height/2);
        d = vec2(d.y, -d.x) / length(d) * line_width/2;
        d = vec2(d.x / target_width*2, d.y / target_height*2);

        gl_Position = gl_in[0].gl_Position - vec4(d*w0,0,0);
        out_color = in_color[0];
        EmitVertex();

        gl_Position = gl_in[1].gl_Position - vec4(d*w1,0,0);
        out_color = in_color[1];
        EmitVertex();
    
        gl_Position = gl_in[0].gl_Position + vec4(d*w0,0,0);
        out_color = in_color[0];
        EmitVertex();

        gl_Position = gl_in[1].gl_Position + vec4(d*w1,0,0);
        out_color = in_color[1];
        EmitVertex();
    
        EndPrimitive();
}

</Module>

