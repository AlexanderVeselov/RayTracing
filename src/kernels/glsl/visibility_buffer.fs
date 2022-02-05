#version 430 core

varying vec2 vTexcoord;

void main()
{
    gl_FragColor = vec4(vTexcoord, 0.0f, 1.0f);
}
