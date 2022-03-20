#version 330

in float age;
uniform sampler2D texture0;
out vec4 outColor;

//Opacity
uniform float opacity;

void main()
{
	outColor = texture(texture0, gl_PointCoord);
	outColor.a = 1 - outColor.r * outColor.g * outColor.b;
	outColor.a *= 1 - age;
	outColor *= vec4(0.4f, 0.3f, 0.1f, opacity) * 2;
}
