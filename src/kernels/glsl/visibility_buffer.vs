uniform mat4 g_ViewProjection;
varying vec2 vTexcoord;

void main()
{
    vTexcoord = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = g_ViewProjection * vec4(vTexcoord * 2.0 - 1.0, 0.0, 1.0);
}
