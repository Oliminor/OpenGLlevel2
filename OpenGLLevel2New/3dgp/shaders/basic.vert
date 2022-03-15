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

// Normal Map
in vec3 aTangent;
in vec3 aBiTangent;
out mat3 matrixTangent;

// Cube Map
out vec3 texCoordCubeMap;

// Shadow Map
uniform mat4 matrixShadow;
out vec4 shadowCoord;




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

	// calculate tangent local system transformation
	vec3 tangent = normalize(mat3(matrixModelView) * aTangent);
	vec3 biTangent = normalize(mat3(matrixModelView) * aBiTangent);
	matrixTangent = mat3(tangent, biTangent, normal);

	// calculate Cube Map
	texCoordCubeMap = inverse(mat3(matrixView)) * mix(-reflect(position.xyz, normal.xyz), normal.xyz, 0.2);

	// calculate shadow coordinate – using the Shadow Matrix
	mat4 matrixModel = inverse(matrixView) * matrixModelView;
	shadowCoord = matrixShadow * matrixModel * vec4(aVertex + aNormal * 0.1, 1);


	// calculate light
	color = vec4(0, 0, 0, 1);
	if (lightAmbient.on == 1)	color += AmbientLight(lightAmbient);
	if (lightAmbient2.on == 1)	color += AmbientLight(lightAmbient2);
	if (lightDir.on == 1)		color += DirectionalLight(lightDir);

	// calculate texture coordinate
	texCoord0 = aTexCoord;

	// Fog calculation
	fogFactor = exp2(-fogDensity * length(position));
	
}
