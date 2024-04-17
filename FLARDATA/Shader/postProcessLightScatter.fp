//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on https://fabiensanglard.net/lightScattering/index.php which is based on 
//"Volumetric Light Scattering as a Post-Process" GPU Gems 3 (2005) and
//"Light Shaft Rendering" ShadersX3 (2004). 

#version 330
#define MAX_NUM_LIGHTS $MAX_NUM_LIGHTS
in  vec2  TexCoords; 

uniform float exposure;
uniform float decay;
uniform float density;
uniform float weight;
uniform vec2 lightPositionOnScreen[MAX_NUM_LIGHTS];
uniform sampler2D myTexture;
uniform int numActiveLights;

const int NUM_SAMPLES = 75;

void main()
{	
	vec4 result = vec4(0);
	
	for (int lightIndex = 0; lightIndex < MAX_NUM_LIGHTS && lightIndex < numActiveLights; lightIndex++ )
	{
		vec2 deltaTextCoord = vec2( TexCoords.st - lightPositionOnScreen[lightIndex].xy );
		vec2 textCoord = TexCoords.st;
		deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;
		float illuminationDecay = 1.0;
		
		for(int i=0; i < NUM_SAMPLES ; i++)
		{
				textCoord -= deltaTextCoord;				
				vec4 sample = texture2D(myTexture, textCoord );				
				sample *= illuminationDecay * weight;
				result += sample;				
				illuminationDecay *= decay;
		}		
	}
	
	result=min(result,vec4(NUM_SAMPLES*weight));	
	result *= exposure;	
	gl_FragColor = result;
}
