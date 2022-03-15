#version 330

// Input Variables (received from Vertex Shader)
in vec4 color;
in vec4 position;
in vec3 normal;
in vec2 texCoord0;

uniform vec3 waterColor;
uniform vec3 skyColor;

in float reflFactor;			// reflection coefficient

// Output Variable (sent down through the Pipeline)
out vec4 outColor;

void main(void) 
{
	// calculate reflection coefficient
	// using Schlick's approximation of Fresnel formula
	float newReflFactor = reflFactor; 
	float cosTheta = dot(normal, normalize(-position.xyz));
	float R0 = 0.02;
	newReflFactor = R0 + (1 - R0) * pow(1.0 - cosTheta, 5);

	outColor = color;

	outColor = mix(vec4(waterColor, 0.2), vec4(skyColor, 1), newReflFactor);
}
