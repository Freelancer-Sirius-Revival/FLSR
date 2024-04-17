//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Depth only shader for FLAR by Schmackbolzen

#version 330

uniform sampler2D texture;
uniform float alphaTestValue;
in vec2 outTexCoords;

void main()
{
	if (alphaTestValue > 0)
	{
		if (texture2D(texture, outTexCoords).a < alphaTestValue)
			discard;
	}
	gl_FragColor = vec4(1,0,0,1);
}