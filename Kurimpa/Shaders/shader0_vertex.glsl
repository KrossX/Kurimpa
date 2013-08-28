precision highp float;
precision highp int;

in vec3 vertexPosition_modelspace;
in vec2 vertexUV;

#ifdef VIEW_VRAM
	out vec2 pos_vram;
#else
	out vec2 pos_rect;
#endif

uniform ivec4 DisplaySize;
uniform ivec4 DisplayOffset;  

void main()
{
	gl_Position = vec4(vertexPosition_modelspace,1);

#ifdef VIEW_VRAM
	pos_vram = vertexUV * vec2(1024.0, 512.0);
#else
	pos_rect = vertexUV * vec2(DisplaySize.zw) + vec2(DisplaySize.xy) - vec2(DisplayOffset.xy);
#endif
}

