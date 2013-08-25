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

uvec3 GetPixel24(vec2 pos)
{
	uvec3 ucolor;
	uint value, color0, color1, color2;
	
	uint curpix = int(pos.x)%2;
	
	if(curpix == 0)
	{
		pos.x -= DisplaySize.x;
		pos.x *= 1.5;
		pos.x += DisplaySize.x;
		
		value = texture(Sampler, pos).r;
		color0 = value & 0xFF;
		color1 = (value >> 8) & 0xFF;
		
		value = texture(Sampler, pos + vec2(1.0, 0.0)).r;
		color2 = value & 0xFF;
	}
	else
	{
		pos.x -= DisplaySize.x;
		pos.x = (pos.x - 1.0) * 1.5 + 2.0;
		pos.x += DisplaySize.x;
		
		value = texture(Sampler, pos).r;
		color1 = value & 0xFF;
		color2 = (value >> 8) & 0xFF;
		
		value = texture(Sampler, pos - vec2(1.0, 0.0)).r;
		color0 = (value >> 8) & 0xFF;
	}
	
	ucolor.r = color0;
	ucolor.g = color1;
	ucolor.b = color2;
	
	return ucolor;
}

vec3 BilinearFilter24(vec2 pos)
{
	vec3 color, colorx, colory, colorxy;
	
	vec3 offset = vec3(1.0, 1.0, 0.0);
	vec2 diff = vec2(fract(pos.x), fract(pos.y));
	
	pos = floor(pos);

	color   = GetPixel24(pos) / 255.0;
	colorx  = GetPixel24(pos + offset.xz) / 255.0;
	colory  = GetPixel24(pos + offset.zy) / 255.0;
	colorxy = GetPixel24(pos + offset.xy) / 255.0;
	
	colorx = mix(color, colorx, diff.x);
	colory = mix(colory, colorxy, diff.x);
	color  = mix(colorx, colory, diff.y);

	return color;
}

#else

uvec3 GetPixel16(vec2 pos)
{
	uvec3 ucolor;
	uint value = texture(Sampler, pos).r;
		
	ucolor.r = value & 0x1F;
	ucolor.g = (value >> 5) & 0x1F;
	ucolor.b = (value >> 10) & 0x1F;
	
	return ucolor;
}

vec3 BilinearFilter16(vec2 pos)
{
	vec3 color, colorx, colory, colorxy;
	
	vec3 offset = vec3(1.0, 1.0, 0.0);
	vec2 diff = vec2(fract(pos.x), fract(pos.y));
	
	pos = floor(pos);

	color   = GetPixel16(pos) / 31.0;
	colorx  = GetPixel16(pos + offset.xz) / 31.0;
	colory  = GetPixel16(pos + offset.zy) / 31.0;
	colorxy = GetPixel16(pos + offset.xy) / 31.0;
	
	colorx = mix(color, colorx, diff.x);
	colory = mix(colory, colorxy, diff.x);
	color  = mix(colorx, colory, diff.y);
	
	return color;
}

#endif


void main()
{
#ifdef VIEW_VRAM
	color = BilinearFilter16(pos_vram);
#else

	if(pos_rect.y < DisplaySize.y || pos_rect.y > DisplayOffset.w)	
	{
		color = vec3(0.0, 0.0, 0.0); 
		return;
	}
	


	#ifdef COLOR24
	color = BilinearFilter24(pos_rect);
	#else
	color = BilinearFilter16(pos_rect);
	#endif
	
#endif
}


 