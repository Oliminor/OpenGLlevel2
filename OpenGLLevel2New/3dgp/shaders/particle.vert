#version 330


// Uniforms: Transformation Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Particle-specific Uniforms
uniform vec3 gravity = vec3(0.0, 0.1, 0.0);	// Gravity Acceleration in world coords
uniform float particleLifetime;			// Max Particle Lifetime
uniform float time;					// Animation Time

// Special Vertex Attributes
in vec3 aVelocity;					// Particle initial velocity
in float aStartTime;					// Particle "birth" time
in vec3 aInitialPos;		// Initial Position (source of the fountain)

// Output Variable (sent to Fragment Shader)
out float age;					// age of the particle (0..1)

void main()
{
	float t = mod(time - aStartTime, particleLifetime);
	vec3 initialPos = vec3(aInitialPos[0] + (t * t) * 15, aInitialPos[1], aInitialPos[2]+ (t * t) * 5);
	vec3 pos = initialPos + aVelocity * t * gravity; 
	age = t / particleLifetime;

	// calculate position (normal calculation not applicable here)
	vec4 position = matrixModelView * vec4(pos, 1.0);
	gl_Position = matrixProjection * position;

	float distance = distance(vec3(position), vec3(matrixView));

	gl_PointSize = mix(100 / distance, 10 / distance, age);
}
