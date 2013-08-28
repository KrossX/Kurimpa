precision highp float;
precision highp int;

#ifdef VIEW_VRAM
	in vec2 pos_vram;
#else
	in vec2 pos_rect;
#endif

uniform ivec4 DisplaySize;
uniform ivec4 DisplayOffset;
uniform vec2 WindowSize;

uniform usampler2DRect Sampler;

// Ouput data
out vec3 color;

#ifdef COLOR24
vec3 GetTexel24(vec2 pos)
{
	ivec2 ipos = ivec2(pos);
	ivec2 offset = ivec2(1, 0);

	uint value, color0, color1, color2;
	
	int curpix = ipos.x % 2;
		
	if(curpix == 0)
	{
		ipos.x -= DisplaySize.x;
		ipos.x  = (ipos.x*3)/2;
		ipos.x += DisplaySize.x;
		
		value  = texelFetch(Sampler, ipos).r;
		color0 = value & 0xFFu;
		color1 = (value >> 8u) & 0xFFu;
		
		value =  texelFetch(Sampler, ipos + offset).r;
		color2 = value & 0xFFu;
	}
	else
	{
		ipos.x -= DisplaySize.x;
		ipos.x  = ((ipos.x - 1)*3)/2 + 2;
		ipos.x += DisplaySize.x;
		
		value = texelFetch(Sampler, ipos).r;
		color1 = value & 0xFFu;
		color2 = (value >> 8u) & 0xFFu;
		
		value = texelFetch(Sampler, ipos - offset).r;
		color0 = (value >> 8u) & 0xFFu;
	}
	
	uvec3 ucolor;
	ucolor.r = color0;
	ucolor.g = color1;
	ucolor.b = color2;
	
	return vec3(ucolor) / 255.0;
}

#else

vec3 GetTexel16(vec2 pos)
{
	uint value = texelFetch(Sampler, ivec2(pos)).r;

	uvec3 ucolor;
	ucolor.r = value & 0x1Fu;
	ucolor.g = (value >> 5u) & 0x1Fu;
	ucolor.b = (value >> 10u) & 0x1Fu;

	return vec3(ucolor) / 31.0;
}

#endif

void main()
{
#ifdef VIEW_VRAM
	color = GetTexel16(pos_vram);
#else

	if(pos_rect.y < float(DisplaySize.y) || pos_rect.y > float(DisplayOffset.w))
	{
		color = vec3(0.0, 0.0, 0.0);  
		return;
	}
	
	#ifdef COLOR24
	color = GetTexel24(pos_rect);
	#else
	color = GetTexel16(pos_rect);
	#endif
	
#endif
}

 