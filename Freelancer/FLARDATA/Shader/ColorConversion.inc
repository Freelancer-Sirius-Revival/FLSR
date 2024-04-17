//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//Color conversion include shader for FLAR by Schmackbolzen

vec3 ToLinearAccurate(vec3 inColor)
{
	vec3 treshold=step(vec3(0.04045),inColor);
	return mix(inColor / 12.92,pow ((inColor + 0.055) / 1.055, vec3(2.4)),treshold);
}

vec3 ToGammaCorrectedAccurate(vec3 inColor)
{
	vec3 treshold=step(vec3(0.0031308),inColor);
	return mix(inColor * 12.92,pow (inColor, vec3(1.0/2.4)) * 1.055 - 0.055,treshold);
}

vec3 ToLinearFast(vec3 inColor)
{	
	return pow(inColor,vec3(2.2));
}

vec3 ToGammaCorrectedFast(vec3 inColor)
{
	return pow(inColor.rgb,1./vec3(2.2));
}

$SRGB_CONVERSION_DEFINE
#ifdef ALWAYS_USE_FAST_SRGB_CONVERSION
vec3 ToLinear(vec3 inColor)
{	
	return ToLinearFast(inColor);
}

vec3 ToGammaCorrected(vec3 inColor)
{
	return ToGammaCorrectedFast(inColor);
}
#endif

#ifdef ALWAYS_USE_ACCURATE_SRGB_CONVERSION
vec3 ToLinear(vec3 inColor)
{	
	return ToLinearAccurate(inColor);
}

vec3 ToGammaCorrected(vec3 inColor)
{
	return ToGammaCorrectedAccurate(inColor);
}
#endif

//Works on gamma compressed and linear values (note that the values are not identical)
//Values are from https://en.wikipedia.org/wiki/Luma_(video)
float ToLuma(vec3 color)
{    
	return dot(color, vec3(0.2126, 0.7152f, 0.0722f));
}