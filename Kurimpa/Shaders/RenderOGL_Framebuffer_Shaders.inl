namespace shaders
{
	char fb_define[] =
		"#version 140\n" \

		"precision highp float;\n" \
		"precision highp int;\n";

	char fb_vertex[] =
		"in vec3 QuadPos;\n" \
		"in vec2 QuadUV;\n" \

		"out vec2 pos_fb;\n" \

		"void main()\n" \
		"{\n" \
		"	gl_Position = vec4(QuadPos, 1);\n" \
		"	pos_fb = QuadUV;\n" \
		"}\n";

	char fb_frag[] =
		"in vec2 pos_fb;\n" \
		"uniform sampler2D framebuffer;\n" \

		"out vec3 color;\n" \

		"void main()\n" \
		"{\n" \
		"	color = texture(framebuffer, pos_fb).rgb;\n" \
		"}\n";
}