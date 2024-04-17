//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on https://fabiensanglard.net/lightScattering/index.php which is based on 
//"Volumetric Light Scattering as a Post-Process" GPU Gems 3 (2005) and
//"Light Shaft Rendering" ShadersX3 (2004). 

#version 330
layout (location = 0) in vec4 vertex;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(vertex.xy, 0.0f, 1.0f); 
    vec2 texture = vertex.zw;    
    TexCoords = texture;
}  
