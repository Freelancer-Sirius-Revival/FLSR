//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//No material shader for FLAR by Schmackbolzen

#version 330

uniform vec3 color;

void main()
{		
	gl_FragColor = vec4(color,1);	
}