//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//Simple color shader for FLAR by Schmackbolzen

#version 330

in vec3 currentColor;

void main()
{		
	gl_FragColor = vec4(currentColor,1);
}
