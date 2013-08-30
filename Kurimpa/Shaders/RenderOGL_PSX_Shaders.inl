namespace shader
{
	char *define[] = 
	{
		"#version 140\n",
		"#version 140\n#define COLOR24\n",
		"#version 140\n#define VIEW_VRAM\n"
	};

	char main_vertex[] =
		"precision highp float;\n" \
		"precision highp int;\n" \

		"in vec3 vertexPosition_modelspace;\n" \
		"in vec2 vertexUV;\n" \

		"#ifdef VIEW_VRAM\n" \
		"	out vec2 pos_vram;\n" \
		"#else\n" \
		"	out vec2 pos_rect;\n" \
		"#endif\n" \

		"uniform ivec4 DisplaySize;\n" \
		"uniform ivec4 DisplayOffset;\n" \

		"void main()\n" \
		"{\n" \
		"	gl_Position = vec4(vertexPosition_modelspace, 1);\n" \

		"#ifdef VIEW_VRAM\n" \
		"	pos_vram = vertexUV * vec2(1024.0, 512.0);\n" \
		"#else\n" \
		"	pos_rect = vertexUV * vec2(DisplaySize.zw) + vec2(DisplaySize.xy) - vec2(DisplayOffset.xy);\n" \
		"#endif\n" \
		"}\n";

	char main_frag[] =
		"precision highp float;\n" \
		"precision highp int;\n" \

		"#ifdef VIEW_VRAM\n" \
		"	in vec2 pos_vram;\n" \
		"#else\n" \
		"	in vec2 pos_rect;\n" \
		"#endif\n" \

		"uniform ivec4 DisplaySize;\n" \
		"uniform ivec4 DisplayOffset;\n" \
		"uniform vec2 WindowSize;\n" \

		"uniform usampler2DRect Sampler;\n" \

		"out vec3 color;\n" \

		"#ifdef COLOR24\n" \
		"vec3 GetTexel24(vec2 pos)\n" \
		"{\n" \
		"	ivec2 ipos = ivec2(pos);\n" \
		"	ivec2 offset = ivec2(1, 0);\n" \

		"	uint value, color0, color1, color2;\n" \

		"	int curpix = ipos.x % 2;\n" \

		"	if(curpix == 0)\n" \
		"	{\n" \
		"		ipos.x -= DisplaySize.x;\n" \
				"ipos.x  = (ipos.x*3)/2;\n" \
				"ipos.x += DisplaySize.x;\n" \

		"		value  = texelFetch(Sampler, ipos).r;\n" \
		"		color0 = value & 0xFFu;\n" \
		"		color1 = (value >> 8u) & 0xFFu;\n" \

		"		value =  texelFetch(Sampler, ipos + offset).r;\n" \
		"		color2 = value & 0xFFu;\n" \
		"	}\n" \
		"	else\n" \
		"	{\n" \
		"		ipos.x -= DisplaySize.x;\n" \
		"		ipos.x  = ((ipos.x - 1)*3)/2 + 2;\n" \
		"		ipos.x += DisplaySize.x;\n" \

		"		value = texelFetch(Sampler, ipos).r;\n" \
		"		color1 = value & 0xFFu;\n" \
		"		color2 = (value >> 8u) & 0xFFu;\n" \

		"		value = texelFetch(Sampler, ipos - offset).r;\n" \
		"		color0 = (value >> 8u) & 0xFFu;\n" \
		"	}\n" \

		"	uvec3 ucolor;\n" \
		"	ucolor.r = color0;\n" \
		"	ucolor.g = color1;\n" \
		"	ucolor.b = color2;\n" \

		"	return vec3(ucolor) / 255.0;\n" \
		"}\n" \

		"#else\n" \

		"vec3 GetTexel16(vec2 pos)\n" \
		"{\n" \
		"	uint value = texelFetch(Sampler, ivec2(pos)).r;\n" \

		"	uvec3 ucolor;\n" \
		"	ucolor.r = value & 0x1Fu;\n" \
		"	ucolor.g = (value >> 5u) & 0x1Fu;\n" \
		"	ucolor.b = (value >> 10u) & 0x1Fu;\n" \

		"	return vec3(ucolor) / 31.0;\n" \
		"}\n" \

		"#endif\n" \

		"void main()\n" \
		"{\n" \
		"#ifdef VIEW_VRAM\n" \
		"	color = GetTexel16(pos_vram);\n" \
		"#else\n" \
		"\n" \
		"	if(pos_rect.y < float(DisplaySize.y) || pos_rect.y > float(DisplayOffset.w))\n" \
		"	{\n" \
		"		color = vec3(0.0, 0.0, 0.0);  \n" \
		"		return;\n" \
		"	}\n" \

		"	#ifdef COLOR24\n" \
		"	color = GetTexel24(pos_rect);\n" \
		"	#else\n" \
		"	color = GetTexel16(pos_rect);\n" \
		"	#endif\n" \

		"#endif\n" \
		"}\n";

}