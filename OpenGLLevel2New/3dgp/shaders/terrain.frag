// FRAGMENT SHADER
#version 330

in vec4 color;
out vec4 outColor;

// Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Materials
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;

in vec4 position;
in vec3 normal;

in vec2 texCoord0;

uniform sampler2D texture0;

// Fog
uniform vec3 fogColour;
in float fogFactor;

// Water
uniform vec3 waterColor;
in float waterDepth;		// water depth (positive for underwater, negative for the shore)

// Shore
uniform sampler2D textureBed;
uniform sampler2D textureShore;

void main(void) 
{
	outColor = color;

	// shoreline multitexturing
	float isAboveWater = clamp(-waterDepth, 0, 1); 
	outColor *= mix(texture(textureBed, texCoord0), texture(textureShore, texCoord0), isAboveWater);

	outColor *= texture(texture0, texCoord0);

	outColor = mix(vec4(fogColour, 1), outColor, fogFactor); // Fog
}
