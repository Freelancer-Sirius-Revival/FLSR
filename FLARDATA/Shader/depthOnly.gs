//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on "Parallel-Split Shadow Maps on Programmable GPUs" GPU Gems 3 (2005)

#version 330
const int g_iNumSplits = $NUM_SPLITS;

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

flat in int splitIndex[];

in vec2 inTexCoords[];
uniform mat4 mvpMat[g_iNumSplits];

out vec2 outTexCoords;

void main()
{
	for(int iVertex = 0; iVertex < 3; iVertex++)
	{
	  int iSplit = splitIndex[iVertex];
	  gl_Layer = iSplit;
	  gl_Position = mvpMat[iSplit]*gl_in[iVertex].gl_Position;
	  outTexCoords = inTexCoords[iVertex];
	  EmitVertex();
	}
	EndPrimitive();
}