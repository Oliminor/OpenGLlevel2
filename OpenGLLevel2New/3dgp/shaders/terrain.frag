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

// Fog
uniform vec3 fogColour;
uniform vec3 fogColour2;
in float fogFactor;
in float fogFactor2;

// Water
in float waterDepth;		// water depth (positive for underwater, negative for the shore)

// Shore
uniform sampler2D textureBed;
uniform sampler2D textureShore;

// Shadow Map
in vec4 shadowCoord;
uniform sampler2DShadow shadowMap;
uniform int useShadowMap;

// Normal Map
uniform sampler2D textureNormal;
in mat3 matrixTangent;
uniform int useNormalMap;
vec3 normalNew;

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
	// Rim Light Effect
	vec3 N = normalize(normal);
	vec3 V = normalize(-vec3(position));
	float rim = 1 - dot (V, N);
	float finalColor = smoothstep(0.8, 1.0, rim);

	// Normal calculation
	if (useNormalMap == 1)
	{
		normalNew = 2.0 * texture(textureNormal, texCoord0 * vec2(scaleX, scaleY)).xyz - vec3(1.0, 1.0, 1.0);
		normalNew = normalize(matrixTangent * normalNew);
	}
	else
		normalNew = normal;

	outColor = color;

	// Calculation of the shadow
	vec4 shadowCoordNew = shadowCoord;

	float textureProjectionCalc = 0;

	for (int i = -1; i <= 1; i++)
	{
		shadowCoordNew[0] += 0.025 * i;
		shadowCoordNew[0] -= 0.025 * i;
		shadowCoordNew[2] += 0.025 * i;
		shadowCoordNew[2] -= 0.025 * i;
		textureProjectionCalc += textureProj(shadowMap, shadowCoordNew);
	}

	textureProjectionCalc /= 3;

	float shadow = 1.0;
	if (shadowCoord.w > 0)	// if shadowCoord.w < 0 fragment is out of the Light POV
		shadow = 0.5 + 0.5 * textureProjectionCalc;
	
	for (int i = 0; i < lightPoint.length; i++)
	{
		if (lightPoint[i].on == 1)	outColor += PointLight(lightPoint[i]);
	}

	for (int i = 0; i < lightSpot.length; i++)
	{
		if (lightSpot[i].on == 1)	outColor += SpotLight(lightSpot[i]);
	}

	// shoreline multitexturing
	float isAboveWater = clamp(-waterDepth, 0, 1); 
	outColor *= mix(texture(textureBed, texCoord0), texture(textureShore, texCoord0), isAboveWater);

	outColor *= texture(texture0, texCoord0 * vec2(scaleX, scaleY));

	outColor = mix(vec4(fogColour, 1), outColor, fogFactor); // Fog
	outColor += mix(vec4(fogColour2 ,1), outColor, fogFactor2); // Fog
	if (useShadowMap == 1) outColor *= shadow; // Shadow
	outColor.rgb += vec3(finalColor) * materialDiffuse.rgb; // Rim light
}
