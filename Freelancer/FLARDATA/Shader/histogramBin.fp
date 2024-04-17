//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//Simple histogram shader for FLAR by Schmackbolzen

#version 330
#include "ColorConversion.inc"
#define NUM_LIMITS $NUM_LIMITS
in  vec2  TexCoords;

uniform sampler2D scene;
uniform vec2 limits[NUM_LIMITS];
uniform int index;

void main()
{ 
  float luma = clamp(ToLuma(texture2D(scene, TexCoords).rgb),limits[0].x,limits[NUM_LIMITS-1].x);
  vec2 currentLimits=limits[index];  
  bool withinLimits;   
  withinLimits = (luma >= currentLimits.x) && (luma < currentLimits.y);
  if (!withinLimits)
	  discard;  
  gl_FragDepth= 0;
}