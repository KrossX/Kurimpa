/*  Kurimpa - OGL3 Video Plugin for Emulators
 *  Copyright (C) 2013  KrossX
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "General.h"
#include "Renderer.h"

#define GLEW_STATIC
#include <gl/glew.h>
#include <gl/wglew.h>

#include <string>
#include <fstream>
#include <vector>

extern void *psxmem;

HGLRC g_hrc;// Rendering context
HDC g_hdc;  // Device context
HWND g_hwnd;// Window identifier
WNDPROC g_oldproc;

LONG g_style;
float g_width, g_height;

bool linearfilter = true;
bool usevsync = false;
bool fullscreen = false;
bool show_vram = false;

const GLuint TEX_INTFORMAT = GL_RGB5_A1;
const GLuint TEX_FORMAT    = GL_RGBA;
const GLuint TEX_TYPE      = GL_UNSIGNED_SHORT_1_5_5_5_REV;

#pragma pack(push)
#pragma pack(1)
struct TGAHEADER
{
	u8  idlength;
	u8  colourmaptype;
	u8  datatypecode;
	
	u16 colourmaporigin;
	u16 colourmaplength;
	u8  colourmapdepth;

	u16 x_origin;
	u16 y_origin;
	u16 width;
	u16 height;
	u8  bitsperpixel;
	u8  imagedescriptor;
};
#pragma pack(pop)

void TakeScreenshot()
{
	TGAHEADER header;

	FILE *dump = NULL;
	char filename[18];

	for(int number = 0; number < 999 ; number++)
	{
		sprintf_s(filename, "Kurimpa-%03da.tga", number);
		fopen_s(&dump, filename, "rb");
		if(!dump) break;
	}

	fopen_s(&dump, filename, "wb");

	if(dump)
	{
		u32 size = (u32)(g_width * g_height) * 3;
		u8 *buffer = new u8[size];
		glReadPixels(0, 0, (int)g_width, (int)g_height, GL_BGR, GL_UNSIGNED_BYTE, buffer);
		
		memset(&header, 0, sizeof(TGAHEADER));
		header.datatypecode = 0x02; // Uncompressed RGB
		header.width  = (u16)g_width;
		header.height = (u16)g_height;
		header.bitsperpixel = 24; // RGB

		fwrite(&header, 1, sizeof(TGAHEADER), dump);
		fwrite(buffer, 1, size, dump);
		fclose(dump);

		delete[] buffer;
	}

	filename[11] = 'b';
	fopen_s(&dump, filename, "wb");

	if(dump)
	{
		u32 size = 1024 * 512 * 3;
		u8 *buffer = new u8[size];

		memset(&header, 0, sizeof(TGAHEADER));
		header.datatypecode = 0x02; // Uncompressed RGB
		header.width  = 1024;
		header.height = 512;
		header.bitsperpixel = 24; // RGBA

		u16 *pix = (u16*)psxmem;

		for(int y = 0; y < 512; y++)
		for(int x = 0; x < 1024; x++)
		{
			u32 pos = x+ y * 1024;
			Pixel16 pix16(pix[pos]);

			pos = (x + (511 - y) * 1024) * 3;

			buffer[pos + 2] = (pix16.pix.R * 0xFF) / 0x1F;
			buffer[pos + 1] = (pix16.pix.G * 0xFF) / 0x1F;
			buffer[pos + 0] = (pix16.pix.B * 0xFF) / 0x1F;
			//buffer[pos + 3] = pix16.pix.A ? 0xFF : 0x00;
		}

		fwrite(&header, 1, sizeof(TGAHEADER), dump);
		fwrite(buffer, 1, size, dump);
		fclose(dump);

		delete[] buffer;
	}

	
}

void WindowResize(int width, int height)
{
	g_width = width * 1.0f;
	g_height = height * 1.0f;

	int width43 = (height * 4) / 3;
	int padleft = (width - width43) / 2;

	glViewport(padleft, 0, width43, height);
}

// Context creation source copy pasta' yum~
// http://www.swiftless.com/tutorials/opengl4/1-opengl-window.html

bool CreateContext(HWND hWnd) 
{
	g_hwnd = hWnd;
	g_hdc = GetDC(hWnd);

	PIXELFORMATDESCRIPTOR pfd; // Create a new PIXELFORMATDESCRIPTOR (PFD)  
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); // Clear our  PFD  
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); // Set the size of the PFD to the size of the class  
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; // Enable double buffering, opengl support and drawing to a window  
	pfd.iPixelType = PFD_TYPE_RGBA; // Set our application to use RGBA pixels  
	pfd.cColorBits = 32; // Give us 32 bits of color information (the higher, the more colors)  
	pfd.cDepthBits = 32; // Give us 32 bits of depth information (the higher, the more depth levels)  
	pfd.iLayerType = PFD_MAIN_PLANE; // Set the layer of the PFD

	int nPixelFormat = ChoosePixelFormat(g_hdc, &pfd); // Check if our PFD is valid and get a pixel format back  
	if (nPixelFormat == 0) // If it fails  
	return false;
  
	BOOL bResult = SetPixelFormat(g_hdc, nPixelFormat, &pfd); // Try and set the pixel format based on our PFD  
	if (!bResult) // If it fails  
	return false;

	HGLRC tempOpenGLContext = wglCreateContext(g_hdc); // Create an OpenGL 2.1 context for our device context  
	wglMakeCurrent(g_hdc, tempOpenGLContext); // Make the OpenGL 2.1 context current and active  

	glewExperimental = true;
	GLenum error = glewInit(); // Enable GLEW  
	if (error != GLEW_OK) // If GLEW fails  
	return false;

	int attributes[] = {
	WGL_CONTEXT_MAJOR_VERSION_ARB, 3, // Set the MAJOR version of OpenGL to 3
	WGL_CONTEXT_MINOR_VERSION_ARB, 3, // Set the MINOR version of OpenGL to 3
	WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	0  
	};

	if (wglewIsSupported("WGL_ARB_create_context") == 1) // If the OpenGL 3.x context creation extension is available  
	{ 
		g_hrc = wglCreateContextAttribsARB(g_hdc, NULL, attributes); // Create and OpenGL 3.x context based on the given attributes  
		wglMakeCurrent(NULL, NULL); // Remove the temporary context from being active  
		wglDeleteContext(tempOpenGLContext); // Delete the temporary OpenGL 2.1 context
		wglMakeCurrent(g_hdc, g_hrc); // Make our OpenGL 3.0 context current  
	}
	else
		return false;

	int glVersion[2] = {-1, -1}; // Set some default values for the version  
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); // Get back the OpenGL MAJOR version we are using  
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); // Get back the OpenGL MAJOR version we are using  

	printf("Kurimpa -> Using OpenGL v%d.%d\n", glVersion[0], glVersion[1]);

	return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	static bool kbuff[256];

	switch (message)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		kbuff[wParam] = true;

		switch(wParam)
		{
		case VK_MULTIPLY:
			linearfilter = !linearfilter;
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, linearfilter ? GL_LINEAR : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, linearfilter ? GL_LINEAR : GL_NEAREST);
			break;

		case VK_F9:
			usevsync = !usevsync;
			wglSwapIntervalEXT(usevsync ? 1 : 0);
			break;

		case VK_F11:
			show_vram = !show_vram;
			break;

		case VK_RETURN:

			if(kbuff[VK_MENU])
			{
				fullscreen = !fullscreen;

				if(fullscreen)
				{
					int width = GetSystemMetrics(SM_CXSCREEN);
					int height = GetSystemMetrics(SM_CYSCREEN);

					LONG newstyle = g_style & ~(WS_CAPTION | WS_BORDER);

					SetWindowLong(g_hwnd, GWL_STYLE, newstyle);
					SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_ASYNCWINDOWPOS);
				}
				else
				{
					RECT size; 
					size.top = 0; size.bottom = 480;
					size.left = 0; size.right = 640;

					SetWindowLong(g_hwnd, GWL_STYLE, g_style);
					AdjustWindowRect(&size, g_style, FALSE);
					SetWindowPos(g_hwnd, HWND_TOP, 200, 200, size.right - size.left, size.bottom - size.top, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);
				}
			}

			break;
		
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		switch(wParam)
		{
		case VK_SNAPSHOT:
			TakeScreenshot();
			break;
		}

		kbuff[wParam] = false;
		break;

	case WM_WINDOWPOSCHANGED:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			WindowResize(rect.right, rect.bottom);
		}
		break;

	case WM_CLOSE: // Haxz: Clicking X and Emu not stopping...
		g_oldproc(hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return g_oldproc(hWnd, message, wParam, lParam);
}

HWND CreateGLWindow(LPCWSTR title, int width, int height)
{  
	WNDCLASS windowClass;
	HWND hWnd;
	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	hInstance = GetModuleHandle(NULL);

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = (WNDPROC) WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = title;

	if (!RegisterClass(&windowClass))
	return 0;

	hWnd = CreateWindowEx(dwExStyle, title, title, WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT, 0, width, height, NULL, NULL, hInstance, NULL);

	return hWnd;
} 

char* ReadFileToBuffer(const char *filename, const char *insert)
{
	using namespace std;

	char *buffer = NULL;
	ifstream file(filename, ios::in | ios::binary | ios::ate);

	if(file.is_open())
	{
		int filesize = (int)file.tellg();

		if(filesize > 0)
		{
			int def_len = insert ? def_len = strlen(insert) : 0;
			int sizebuff = filesize + def_len;
			
			buffer = new char[sizebuff + 1];
			if(insert) memcpy(buffer, insert, def_len);

			file.seekg(0, ios::beg);
			file.read(&buffer[def_len], filesize);
			buffer[sizebuff] = 0x00;
		}

		file.close();
	}

	return buffer;
}

void CheckShaderError(GLuint ShaderID)
{
	GLint Result = GL_FALSE;
	int InfoLogLength = 0;

	glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if(Result == GL_FALSE && InfoLogLength > 0)
	{
		char *message = new char[InfoLogLength];
		glGetShaderInfoLog(ShaderID, InfoLogLength, NULL, message);
		DebugShader(message, InfoLogLength);
		delete[] message;
	}
}

void CheckProgramError(GLuint ProgramID)
{
	GLint Result = GL_FALSE;
	int InfoLogLength = 0;

	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if(Result == GL_FALSE && InfoLogLength > 0)
	{
		char *message = new char[InfoLogLength];
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, message);
		DebugShader(message, InfoLogLength);
		delete[] message;
	}
}

GLuint CompileShader(const char *shader_path, unsigned int type, const char* defines)
{
	GLuint ShaderID = glCreateShader(type);

	const GLchar *glbuff = ReadFileToBuffer(shader_path, defines);
	glShaderSource(ShaderID, 1, &glbuff, NULL);
	glCompileShader(ShaderID);
	
	CheckShaderError(ShaderID);
	
	if(glbuff) delete[] glbuff;
	
	return ShaderID;
}

GLuint LoadShaders(const char *vertex_file_path, const char *fragment_file_path, const char* defines)
{
	GLuint VertexShaderID = CompileShader(vertex_file_path, GL_VERTEX_SHADER, defines);
	GLuint FragmentShaderID = CompileShader(fragment_file_path, GL_FRAGMENT_SHADER, defines);

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	CheckProgramError(ProgramID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}


// An array of 3 vectors which represents 3 vertices
static const GLfloat g_vertex_quad[] = 
{
   -1.0f, -1.0f, 0.0f,
   -1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
};

static const GLfloat g_uv_quad[] = 
{
   0.0f, 1.0f,
   0.0f, 0.0f,
   1.0f, 1.0f,
   1.0f, 0.0f,
};

static inline void GetTexture(GLuint &tex)
{
	if(tex) glDeleteTextures(1, &tex);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE, tex);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, TEX_INTFORMAT, 1024, 512, 0, TEX_FORMAT, TEX_TYPE, 0);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_RECTANGLE, GL_TEXTURE_BORDER_COLOR, color);
	
	u32 err = glGetError();
	 if (err != GL_NO_ERROR)
	{
		if (err == GL_INVALID_ENUM)      printf("OpenGL error: Invalid enum!\n");
		if (err == GL_INVALID_VALUE)     printf("OpenGL error: Invalid value!\n");
		if (err == GL_INVALID_OPERATION) printf("OpenGL error: Invalid operation!\n");
		if (err == GL_STACK_OVERFLOW)    printf("OpenGL error: Stack overflow!\n");
		if (err == GL_STACK_UNDERFLOW)   printf("OpenGL error: Stack underflow!\n");
		if (err == GL_OUT_OF_MEMORY)     printf("OpenGL error: Not enough memory!\n");
	}
	
	glActiveTexture(GL_TEXTURE0);
}

void  RenderOGL::PrepareQuad()
{
	GetTexture(tVRAM_ID);
	
	glGenBuffers(1, &vb_background);
	glBindBuffer(GL_ARRAY_BUFFER, vb_background);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_quad), g_vertex_quad, GL_STATIC_DRAW);

	glGenBuffers(1, &uv_background);
	glBindBuffer(GL_ARRAY_BUFFER, uv_background);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_quad), g_uv_quad, GL_STATIC_DRAW);
}

void  RenderOGL::DrawBackground()
{
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vb_background);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uv_background);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
 
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Starting from vertex 0; 3 vertices total -> 1 triangle
 
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

bool RenderOGL::Init(HWND hWin)
{
	//HWND hWin = CreateGLWindow(L"Kurimpa", 640, 480);

	if (!hWin || !CreateContext(hWin))
	{ 
		//destroy stuff
		return false;
	}

	origin_y = origin_x = 0;
	width = 320; height = 240;

	g_hwnd = hWin;

	LONG proc = GetWindowLong(hWin, GWL_WNDPROC);
	g_oldproc = (proc && (WNDPROC)proc != WndProc)? (WNDPROC)proc : DefWindowProc;
	SetWindowLong(hWin, GWL_WNDPROC, (LONG)WndProc);

	g_style = GetWindowLong(hWin, GWL_STYLE);

	RECT size; 
	size.top = 0; size.bottom = 480;
	size.left = 0; size.right = 640;

	AdjustWindowRect(&size, g_style, FALSE);

	SetWindowTextA(g_hwnd, "Kurimpa");
	SetWindowPos(g_hwnd, HWND_TOP, 200, 200, size.right - size.left, size.bottom - size.top, SWP_ASYNCWINDOWPOS | SWP_NOMOVE);

	wglSwapIntervalEXT(usevsync ? 1 : 0);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	
	PrepareQuad();

	char *PROG_DEFINE[PROG_SIZE] = {"#version 330 core\n", "#version 330 core\n#define COLOR24\n", "#version 330 core\n#define VIEW_VRAM\n"};

	for(int i = 0; i < PROG_SIZE; i++)
	{
		Prog[i].ProgramID = LoadShaders("shader0_vertex.glsl", "shader0_fragment.glsl", PROG_DEFINE[i]);
		Prog[i].SamplerID = glGetUniformLocation(Prog[i].ProgramID, "Sampler");
		
		Prog[i].u_WindowSize  = glGetUniformLocation(Prog[i].ProgramID,"WindowSize");
		Prog[i].u_DisplaySize = glGetUniformLocation(Prog[i].ProgramID,"DisplaySize");
		Prog[i].u_DisplayOffset = glGetUniformLocation(Prog[i].ProgramID,"DisplayOffset");

		glUniform1i(Prog[i].SamplerID, 0);
		glBindSampler(0, Prog[i].SamplerID);
	}

	return true;
}

void RenderOGL::Present(bool is24bpp, bool disabled)
{
	if(disabled && !show_vram)
	{
		glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
		SwapBuffers(g_hdc);
		return;
	}

	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 1024, 512, TEX_FORMAT, TEX_TYPE, psxmem);
	
	if(show_vram)
	{
		glUseProgram(Prog[PROG_VRAM].ProgramID);
		glUniform2f(Prog[PROG_VRAM].u_WindowSize, g_width, g_height);
		glUniform4i(Prog[PROG_VRAM].u_DisplaySize, 0, 0, 1024, 512);
		glUniform4i(Prog[PROG_VRAM].u_DisplayOffset, 0, 0, 0, 0);
	}
	else
	{
		u8 program = is24bpp ? PROG_FB24 : PROG_FB16;

		glUseProgram(Prog[program].ProgramID);
		glUniform2f(Prog[program].u_WindowSize, g_width, g_height);
		glUniform4i(Prog[program].u_DisplaySize, origin_x, origin_y, width -1, height -1);
		glUniform4i(Prog[program].u_DisplayOffset, offset_x1, offset_y1, offset_x2, offset_y2);
	}
	
	DrawBackground();
	SwapBuffers(g_hdc);
}

void RenderOGL::UpdateTitle(const char* title)
{
	if(title)
		SetWindowTextA(g_hwnd, title);
}

void RenderOGL::Close()
{
	glDeleteBuffers(1, &vb_background);
	glDeleteBuffers(1, &uv_background);
	
	for(int i = 0; i < PROG_SIZE; i++)
		glDeleteProgram(Prog[i].ProgramID);

	glDeleteTextures(1, &tVRAM_ID);
	glDeleteVertexArrays(1, &VertexArrayID);


	wglMakeCurrent(g_hdc, 0);	// Remove the rendering context from our device context
	wglDeleteContext(g_hrc);	// Delete our rendering context
	ReleaseDC(g_hwnd, g_hdc);	// Release the device context from our window
}
