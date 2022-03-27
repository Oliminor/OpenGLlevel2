#version 330

// Input Variables (received from Vertex Shader)
in vec2 texCoord0;

uniform int mode;

// Uniform: The Texture
uniform sampler2D texture0;
uniform vec2 resolution;

// Output Variable (sent down through the Pipeline)
out vec4 outColor;

// Vignette parameters
const float RADIUS = 0.85;
const float SOFTNESS = 0.45;

// Colour definitions
const vec3 lum = vec3(0.299, 0.587, 0.114);	// B&W filter
const vec3 sepia = vec3(1.2, 1.0, 0.8); 

// Contrast and brightness
uniform float brightness;
uniform float contrast;

// Edge
const float edgeThreshold = 0.25;

// Swirl
uniform float radius = 300.0;
uniform float angle = 0.6;

// Toonish
uniform float threshold = 0.1;
uniform float strength = 1;
const float coolThreshold = 0.20;


void main(void) 
{
      outColor = texture(texture0, texCoord0);

	  // brightness + contrast + vignette
	  if (mode == 1)
	  {
	//https://stackoverflow.com/questions/1506299/applying-brightness-and-contrast-with-opengl-es

	 outColor = mix(outColor * brightness, mix(vec4(0.5,0.5,0.5,1.0), outColor, contrast), 0.5);

	// Vignette

	// Find centre position
	vec2 centre = (gl_FragCoord.xy / resolution.xy) - vec2(0.5);

	// Distance from the centre (between 0 and 1)
	float dist = length(centre);

	// Hermite interpolation to create smooth vignette
	dist = smoothstep(RADIUS, RADIUS-SOFTNESS, dist);

	// mix in the vignette
	outColor.rgb = mix(outColor.rgb, outColor.rgb * dist, 0.5);
	  }

	  // Sepia
	if (mode == 2)
	  {
	// Vignette

    // Find centre position
    vec2 centre = (gl_FragCoord.xy / resolution.xy) - vec2(0.5);

    // Distance from the centre (between 0 and 1)
    float dist = length(centre);

    // Hermite interpolation to create smooth vignette
    dist = smoothstep(RADIUS, RADIUS-SOFTNESS, dist);

    // mix in the vignette
    outColor.rgb = mix(outColor.rgb, outColor.rgb * dist, 0.5);

    // Sepia

    // Find gray scale value using NTSC conversion weights
    float gray = dot(outColor.rgb, lum);

    // mix-in the sepia effect
    outColor.rgb = mix(outColor.rgb, vec3(gray) * sepia, 0.75);
	  }

	 // edge detection + greyscale
	if (mode == 3)
	{
	float s00 = dot(lum, texture(texture0, texCoord0 + vec2(-1,  1) / resolution).rgb);
	float s01 = dot(lum, texture(texture0, texCoord0 + vec2( 0,  1) / resolution).rgb);
	float s02 = dot(lum, texture(texture0, texCoord0 + vec2( 1,  1) / resolution).rgb);
	float s10 = dot(lum, texture(texture0, texCoord0 + vec2(-1,  0) / resolution).rgb);
	float s12 = dot(lum, texture(texture0, texCoord0 + vec2( 1,  0) / resolution).rgb);
	float s20 = dot(lum, texture(texture0, texCoord0 + vec2(-1, -1) / resolution).rgb);
	float s21 = dot(lum, texture(texture0, texCoord0 + vec2( 0, -1) / resolution).rgb);
	float s22 = dot(lum, texture(texture0, texCoord0 + vec2( 1, -1) / resolution).rgb);

	float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
	float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);

	float s = sx *sx + sy * sy;

	if (s > edgeThreshold)
		outColor = vec4(1.0);
	else
	{
		vec3 gray = vec3( dot( vec3(outColor) , vec3( 0.2126 , 0.7152 , 0.0722 ) ) );
		outColor = vec4(gray, 1);
	}
	  }

	// blur
	if (mode == 4)
	{
	const int SIZE = 5;

	vec3 v = vec3(0, 0, 0);
	
	int n = 0;
	for (int k = -SIZE; k <= SIZE; k++)
		for (int j = -SIZE; j <= SIZE; j++)
		{
			v += texture(texture0, texCoord0 + vec2(k, j) / resolution).rgb;
			n++;
		}

	outColor = vec4(v / n, 1);

	  }

	// Swirl
	// https://www.geeks3d.com/20110428/shader-library-swirl-post-processing-filter-in-glsl/
	if (mode == 5)
	{
		 vec2 center = resolution / 2;
		 vec2 tc = texCoord0 * resolution;
		 tc -= center;
		 float dist = length(tc);

		 if (dist < radius) 
		 {
			 float percent = (radius - dist) / radius;
			 float theta = percent * percent * angle * 8.0;
			 float s = sin(theta);
			 float c = cos(theta);
			 tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
		 }

		 tc += center;
		 vec3 color = texture2D(texture0, tc / resolution).rgb;
		 outColor = vec4(color, 1);
	  }
	  
	// Toonish
	// https://pingpoli.medium.com/the-bloom-post-processing-effect-9352fa800caf from here, but I did not like the bloom effect, so kept only the part of it
	if (mode == 6)
	{
    
	float s00 = dot(lum, texture(texture0, texCoord0 + vec2(-1,  1) / resolution).rgb);
	float s01 = dot(lum, texture(texture0, texCoord0 + vec2( 0,  1) / resolution).rgb);
	float s02 = dot(lum, texture(texture0, texCoord0 + vec2( 1,  1) / resolution).rgb);
	float s10 = dot(lum, texture(texture0, texCoord0 + vec2(-1,  0) / resolution).rgb);
	float s12 = dot(lum, texture(texture0, texCoord0 + vec2( 1,  0) / resolution).rgb);
	float s20 = dot(lum, texture(texture0, texCoord0 + vec2(-1, -1) / resolution).rgb);
	float s21 = dot(lum, texture(texture0, texCoord0 + vec2( 0, -1) / resolution).rgb);
	float s22 = dot(lum, texture(texture0, texCoord0 + vec2( 1, -1) / resolution).rgb);

	float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
	float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);

	float s = sx *sx + sy * sy;

	if (s > coolThreshold)
		outColor = vec4( 0.0 , 0.0 , 0.0 , 1.0 );
	else
	{
	vec4 color = texture(texture0, texCoord0);
    float brightness = dot( color.rgb , vec3( 0.2126 , 0.7152 , 0.0722 ) );
    if ( brightness > threshold ) outColor = vec4( strength * color.rgb , 1.0 );
    else outColor = vec4( 0.0 , 0.0 , 0.0 , 1.0 );
	}
	  }

}
