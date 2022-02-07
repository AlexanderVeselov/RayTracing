varying vec2 vTexcoord;

void main()
{
    vTexcoord = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(vTexcoord * 2.0 - 1.0, 0.0, 1.0);
}
