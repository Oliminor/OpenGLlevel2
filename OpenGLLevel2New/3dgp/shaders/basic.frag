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

// UV scale
uniform float scaleX;
uniform float scaleY;

// Animated textures 
uniform float time;
uniform float speedX;
uniform float speedY;

// Fog
uniform vec3 fogColour;
in float fogFactor;

// Normal Map
uniform sampler2D textureNormal;
in mat3 matrixTangent;
uniform int useNormalMap;
vec3 normalNew;

// Cube Map
in vec3 texCoordCubeMap;
uniform samplerCube textureCubeMap;
uniform samplerCube textureCubeMap2;
uniform samplerCube textureCubeMap3;
uniform float reflectionPower;
uniform int useCubeMap;

// Shadow Map
in vec4 shadowCoord;
uniform sampler2DShadow shadowMap;
uniform int useShadowMap;

//Opacity
uniform float opacity;


struct SPOT{
	int on;
	float att_quadratic;
	vec3 position;
	vec3 diffuse;
	vec3 specular;
	vec3 direction;
	float cutoff;
	float attenuation;
	mat4 matrix;
};
uniform SPOT lightSpot[10];

struct POINT
{
	int on;
	float att_quadratic;
	vec3 position;
	vec3 diffuse;
	vec3 specular;
	mat4 matrix;
};
uniform POINT lightPoint[5];

vec4 SpotLight(SPOT light)
{
	// Calculate Point Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(light.matrix * vec4(light.position, 1) - position).xyz;
	
	float NdotL = dot(normalNew, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * max(NdotL, 0);

	vec3 V = normalize(-position.xyz);
	vec3 R = reflect(-L, normalNew);
	float RdotV = dot(R, V);

	if(NdotL > 0&& RdotV > 0)color += vec4(materialSpecular * light.specular * pow(RdotV, shininess), 1);

	// Attenuated
	float dist = length(light.matrix * vec4(light.position, 1) -position);
	float att = 1 / (light.att_quadratic * dist * dist);

	// Calculate Spot Light part
	vec3 direction = normalize(mat3(light.matrix) * light.direction);
	float spot = dot(-L, direction);
	float angleAlpha = acos(spot);
	float cutAngle = radians(clamp(light.cutoff, 0, 90));
	float factor;

	if (angleAlpha <= cutAngle) factor = pow(spot, light.attenuation);
	else factor = 0;
	
	return (factor * color) * att;
}

vec4 PointLight (POINT light)
{
	// Calculate Point Light
	vec4 color = vec4(0, 0, 0, 0);
	vec3 L = normalize(light.matrix * vec4(light.position, 1) - position).xyz;
	
	float NdotL = dot(normalNew, L);
	if (NdotL > 0)
		color += vec4(materialDiffuse * light.diffuse, 1) * max(NdotL, 0);

	vec3 V = normalize(-position.xyz);
	vec3 R = reflect(-L, normalNew);
	float RdotV = dot(R, V);

	if(NdotL > 0&& RdotV > 0)color += vec4(materialSpecular * light.specular * pow(RdotV, shininess), 1);

	// Attenuated
	float dist = length(light.matrix * vec4(light.position, 1) -position);
	float att = 1 / (light.att_quadratic * dist * dist);

	return color * att;
}

void main(void) 
{
	// Animated texture
	float xScrollValue = speedX * time;
	float yScrollValue = speedY * time;

	// Rim Light Effect
	vec3 N = normalize(normal);
	vec3 V = normalize(-vec3(position));
	float rim = 1 - dot (V, N);
	float finalColor = smoothstep(0.6, 1.0, rim);

	// Normal calculation
	if (useNormalMap == 1)
	{
		normalNew = 2.0 * texture(textureNormal, texCoord0* vec2(scaleX, scaleY)).xyz - vec3(1.0, 1.0, 1.0);
		normalNew = normalize(matrixTangent * normalNew);
	}
	else
		normalNew = normal;

	outColor = color;

	// Calculation of the shadow
	float shadow = 1.0;
	if (shadowCoord.w > 0)	// if shadowCoord.w < 0 fragment is out of the Light POV
		shadow = 0.5 + 0.5 * textureProj(shadowMap, shadowCoord);

	// Cube Map
	if(useCubeMap == 1)
	{
		outColor = mix(outColor * texture(texture0, texCoord0.st), texture(textureCubeMap , texCoordCubeMap), reflectionPower);
		outColor = mix(outColor * texture(texture0, texCoord0.st), texture(textureCubeMap2, texCoordCubeMap), reflectionPower);
		outColor = mix(outColor * texture(texture0, texCoord0.st), texture(textureCubeMap3, texCoordCubeMap), reflectionPower);
	}


	for (int i = 0; i < lightPoint.length; i++)
	{
		if (lightPoint[i].on == 1)	outColor += PointLight(lightPoint[i]);
	}

	for (int i = 0; i < lightSpot.length; i++)
	{
		if (lightSpot[i].on == 1)	outColor += SpotLight(lightSpot[i]);
	}

	outColor *= texture(texture0, texCoord0 * vec2(scaleX, scaleY));
	outColor = vec4(outColor[0], outColor[1], outColor[2], opacity);
	//if (useShadowMap == 1) outColor *= shadow; // Shadow
	outColor = mix(vec4(fogColour, 1), outColor, fogFactor); // Fog
}
