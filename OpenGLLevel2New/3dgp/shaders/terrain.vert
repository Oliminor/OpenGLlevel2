// VERTEX SHADER
#version 330

// Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Materials
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;
 
in vec3 aVertex;
in vec3 aNormal;
in vec2 aTexCoord;
out vec2 texCoord0;

out vec4 color;
out vec4 position;
out vec3 normal;

// Fog
out float fogFactor;
uniform float fogDensity;

// Water
uniform float waterLevel;	// water level (in absolute units)
out float waterDepth;	// water depth (positive for underwater, negative for the shore)

struct AMBIENT
{	
	int on;
	vec3 color;
};
uniform AMBIENT lightAmbient, lightAmbient2;

struct DIRECTIONAL
{	
	int on;
	vec3 direction;
	vec3 diffuse;
	mat4 matrix;
};
uniform DIRECTIONAL lightDir;

vec4 AmbientLight(AMBIENT light)
{
// Calculate Ambient Light
	return vec4(materialAmbient * light.color, 1);
}

vec4 DirectionalLight(DIRECTIONAL light)
{
	// Calculate Directional Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(mat3(light.matrix) * light.direction);
	float NdotL = dot(normal, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * max(NdotL, 0);
	return color;
}


void main(void) 
{
	// calculate position
	position = matrixModelView * vec4(aVertex, 1.0);
	gl_Position = matrixProjection * position;

	// calculate normal
	normal = normalize(mat3(matrixModelView) * aNormal);

	// calculate light
	color = vec4(0, 0, 0, 1);
	if (lightAmbient.on == 1)	color += AmbientLight(lightAmbient);
	if (lightDir.on == 1)		color += DirectionalLight(lightDir);

	// calculate texture coordinate
	texCoord0 = aTexCoord;

		// calculate depth of water
	waterDepth = waterLevel - aVertex.y;

	// calculate the observer's altitude above the observed vertex
	float eyeAlt = dot(-position.xyz, mat3(matrixModelView) * vec3(0, 1 ,0));
	float depthFactor = max(waterDepth, 0) / eyeAlt; 

	// Fog calculation
	fogFactor = exp2(-fogDensity * length(position) * depthFactor);
}
