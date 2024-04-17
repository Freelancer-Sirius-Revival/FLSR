//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//PBR bloom downsample shader for FLAR by Schmackbolzen based on code from https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#version 330

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main()
{
	gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);
	texCoord = aTexCoord;
}
