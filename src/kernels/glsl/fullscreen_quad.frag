uniform sampler2D input_sampler;
varying vec2 vTexcoord;

void main()
{
    gl_FragColor = texture(input_sampler, vTexcoord);
}
