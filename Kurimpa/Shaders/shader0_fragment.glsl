#ifdef VIEW_VRAM
	in vec2 pos_vram;
#else
	in vec2 pos_rect;
#endif

uniform ivec4 DisplaySize;
uniform ivec4 DisplayOffset;
uniform vec2 WindowSize;

uniform sampler2DRect Sampler;

// Ouput data
out vec3 color;

#ifdef COLOR24
uint GetTexel24(vec2 pos)
{
	ivec2 ipos = ivec2(pos);
	vec4 texel = texelFetch(Sampler, ipos);
	uvec4 utex = uvec4(texel * 31.0);
	
	uint value;
	value  = utex.r;
	value |= utex.g <<  5;
	value |= utex.b << 10;
	value |= (utex.a&1u) << 15;

	return value;
}

vec3 GetPixel24(vec2 pos)
{
	uint value, color0, color1, color2;
	
	int curpix = int(pos.x)%2;
		
	if(curpix == 0)
	{
		pos.x -= DisplaySize.x;
		pos.x *= 1.5;
		pos.x += DisplaySize.x;
		
		value  = GetTexel24(pos);
		color0 = value & 0xFFu;
		color1 = (value >> 8) & 0xFFu;
		
		value =  GetTexel24(pos + vec2(1.0, 0.0));
		color2 = value & 0xFFu;
	}
	else
	{
		pos.x -= DisplaySize.x;
		pos.x = (pos.x - 1.0) * 1.5 + 2.0;
		pos.x += DisplaySize.x;
		
		value = GetTexel24(pos);
		color1 = value & 0xFFu;
		color2 = (value >> 8) & 0xFFu;
		
		value = GetTexel24(pos - vec2(1.0, 0.0));
		color0 = (value >> 8) & 0xFFu;
	}
	
	vec3 ucolor;
	ucolor.r = float(color0);
	ucolor.g = float(color1);
	ucolor.b = float(color2);
	
	return ucolor;
}

vec3 BilinearFilter24(vec2 pos)
{
	vec3 color0, colorx, colory, colorxy;
	
	vec2 diff = vec2(fract(pos.x), fract(pos.y));
	vec3 offset = vec3(1.0, 1.0, 0.0);
	
	pos = floor(pos);
	ivec2 ipos = ivec2(pos);
	
	color0  = GetPixel24(pos) / 255.0;
	colorx  = GetPixel24(pos + offset.xz) / 255.0;
	colory  = GetPixel24(pos + offset.zy) / 255.0;
	colorxy = GetPixel24(pos + offset.xy) / 255.0;
	
	colorx = mix(color0, colorx, diff.x);
	colory = mix(colory, colorxy, diff.x);
	color0 = mix(colorx, colory, diff.y);

	return color0;
}

#endif

void main()
{
#ifdef VIEW_VRAM
	color = texture(Sampler, pos_vram).rgb;
#else

	if(pos_rect.y < DisplaySize.y || pos_rect.y > DisplayOffset.w)
	{
		color = vec3(0.0, 0.0, 0.0);  
		return;
	}
	
	#ifdef COLOR24
	color = BilinearFilter24(pos_rect);
	#else
	color = texture(Sampler, pos_rect).rgb;
	#endif
	
#endif
}


 