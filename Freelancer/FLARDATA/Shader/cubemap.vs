//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on https://learnopengl.com/PBR/IBL/Specular-IBL

#version 330
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 projectionMat;
uniform mat4 viewMat;

void main()
{
    WorldPos = aPos;  
    gl_Position =  projectionMat * viewMat * vec4(WorldPos, 1.0);
}