#version 330

// Input Variables (received from Vertex Shader)
in vec2 texCoord0;

// Uniform: The Texture
uniform sampler2D texture0;
uniform vec2 resolution = vec2(800, 600);

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


void main(void) 
{
      outColor = texture(texture0, texCoord0);

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
