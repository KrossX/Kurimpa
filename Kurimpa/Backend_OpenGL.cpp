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
#include "Backend_OpenGL.h"

#define GLEW_STATIC
#include <gl/glew.h>
#include <gl/wglew.h>

#include <string>
#include <fstream>
#include <vector>

//---------------------------------------------------------------[CONSTANTS]

const GLuint VER_MAJOR = 3;
const GLuint VER_MINOR = 1;

//---------------------------------------------------------------[CONTEXT]

void Backend_OpenGL::Shutdown()
{
	wglMakeCurrent(NULL, NULL);

	if(hTempGLCtx) wglDeleteContext(hTempGLCtx);
	if(hRenderCtx) wglDeleteContext(hRenderCtx);
	if(hDeviceCtx) ReleaseDC(hWindow, hDeviceCtx);

	hTempGLCtx = NULL;
	hRenderCtx = NULL;
	hDeviceCtx = NULL;
}

bool Backend_OpenGL::CreateContext(HWND hWnd)
{
	hWindow = hWnd;
	hDeviceCtx = GetDC(hWindow);
	if(!hDeviceCtx) return false;

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	//pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int nPixelFormat = ChoosePixelFormat(hDeviceCtx, &pfd);
	if (nPixelFormat == 0) return false;

	BOOL bResult = SetPixelFormat(hDeviceCtx, nPixelFormat, &pfd);
	if (!bResult) return false;

	hTempGLCtx = wglCreateContext(hDeviceCtx);
	if(!hTempGLCtx) return false;

	wglMakeCurrent(hDeviceCtx, hTempGLCtx);

	glewExperimental = GL_TRUE;
	GLenum error = glewInit();
	if (GLfail("Glew Init", error)) return false;


	if(wglewIsSupported("WGL_ARB_create_context") != 1)
		return false;

	int attributes[] = {
	WGL_CONTEXT_MAJOR_VERSION_ARB, VER_MAJOR,
	WGL_CONTEXT_MINOR_VERSION_ARB, VER_MINOR,
	WGL_CONTEXT_FLAGS_ARB ,  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 0};
	
	hRenderCtx = wglCreateContextAttribsARB(hDeviceCtx, NULL, attributes);
	if(!hRenderCtx) return false;

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hTempGLCtx);
	hTempGLCtx = NULL;

	wglMakeCurrent(hDeviceCtx, hRenderCtx);

	int glVersion[2] = {-1, -1};
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	const GLubyte* glVendorStr   = glGetString(GL_VENDOR);
	const GLubyte* glRendererStr = glGetString(GL_RENDERER);
	const GLubyte* glVersionStr  = glGetString(GL_VERSION);

	printf("\n%s\n%s\n%s\n\n", glVendorStr, glRendererStr, glVersionStr);

	if(glVersion[0] < VER_MAJOR && glVersion[1] < VER_MINOR)
		return false;

	// Yay! All is fine now... supposedly.
	return true;
}

bool Backend_OpenGL::Init(HWND hWin)
{
	if(!hWin || !CreateContext(hWin))
	{
		GLfail("OGL Init");
		Shutdown();
		return false;
	}

	hWindow = hWin;
	oldstyle = GetWindowLong(hWin, GWL_STYLE);
	
	oldrect.top = 200;
	oldrect.left = 200;

	return true;
}

void Backend_OpenGL::UpdateTitle(const char* title)
{
	if(title)
		SetWindowTextA(hWindow, title);
}

//---------------------------------------------------------------[MISC]

void Backend_OpenGL::SetScreensize(float w, float h)
{
	width = w;
	height = h;
}

void Backend_OpenGL::SetFullscreen()
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	
	GetWindowRect(hWindow, &oldrect);
	LONG newstyle = oldstyle & ~(WS_CAPTION | WS_BORDER);

	SetWindowLong(hWindow, GWL_STYLE, newstyle);
	SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, width, height, 0);
}

void Backend_OpenGL::SetWindowed(int width, int height)
{
	RECT size; 
	size.top = 0; size.bottom = height;
	size.left = 0; size.right = width;

	SetWindowLong(hWindow, GWL_STYLE, oldstyle);
	AdjustWindowRect(&size, oldstyle, FALSE);

	width = size.right - size.left;
	height = size.bottom - size.top;

	SetWindowPos(hWindow, HWND_TOP, oldrect.left, oldrect.top, width, height, 0);
}

//---------------------------------------------------------------[ERRORCHECK]

bool Backend_OpenGL::GLfail(const char* msg, u32 err)
{
	bool failed = err != GL_NO_ERROR;
	
	if(failed)
	{
		const char *errstr = (char*)glewGetErrorString(err);
		const int buffsize = strlen(msg) + strlen(errstr) + 128;
		char *buffer = new char[buffsize];

		if(buffer)
		{
			char *errenum;

			switch(err)
			{
			case GL_NO_ERROR:
				errenum = "GL_NO_ERROR"; break;
			case GL_INVALID_ENUM:
				errenum = "GL_INVALID_ENUM"; break;
			case GL_INVALID_VALUE:
				errenum = "GL_INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:
				errenum = "GL_INVALID_OPERATION"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				errenum = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
			case GL_OUT_OF_MEMORY:
				errenum = "GL_OUT_OF_MEMORY"; break;
			case GL_STACK_UNDERFLOW:
				errenum = "GL_STACK_UNDERFLOW"; break;
			case GL_STACK_OVERFLOW:
				errenum = "GL_STACK_OVERFLOW"; break;
			default:
				errenum = "UNKNOWN"; break;
			};


			sprintf_s(buffer, buffsize, "Kurimpa -> %s\nOGL: %s (0x%02X)\n%s", msg, errstr, err, errenum);
			MessageBoxA(NULL, buffer, "Kurimpa OpenGL Error!", MB_OK | MB_ICONERROR);
			delete[] buffer;
		}
	}
	
	return failed;
}

bool Backend_OpenGL::GLfail(const char* msg)
{
	GLenum err = glGetError();
	return GLfail(msg, err);
}

//---------------------------------------------------------------[SHADERS]

char* ReadFileToBuffer(const char *filename)
{
	using namespace std;

	char *buffer = NULL;
	ifstream file(filename, ios::in | ios::binary | ios::ate);

	if(file.is_open())
	{
		int filesize = (int)file.tellg();

		if(filesize > 0)
		{
			buffer = new char[filesize + 1];

			file.seekg(0, ios::beg);
			file.read(buffer, filesize);
			buffer[filesize] = 0x00;
		}

		file.close();
	}

	return buffer;
}

//---------------------------------------------------------------[OGLShader]

bool OGLShader::CheckError()
{
	GLint Result = GL_TRUE;
	int InfoLogLength = 0;

	glGetShaderiv(ID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if(Result == GL_FALSE && InfoLogLength > 0)
	{
		char *message = new char[InfoLogLength];
		glGetShaderInfoLog(ID, InfoLogLength, NULL, message);
		printf("Shader Error!\n%s", message);
		//DebugShader(message, InfoLogLength);
		delete[] message;
	}

	return true;
}

bool OGLShader::Create(u32 type)
{
	ID = glCreateShader(type);
	created = ID != 0;
	return created;
}

bool OGLShader::CompileFromBuffer(const char *buffer, const char *defines)
{
	if(!buffer) return false;

	size_t buffer_len = strlen(buffer);
	size_t defines_len = defines ? strlen(defines) : 0;

	char *shaderbuff = new char[buffer_len + defines_len + 5];
	memset(&shaderbuff[buffer_len + defines_len - 1], 0x00, 5);
	
	if(defines_len)
		memcpy(shaderbuff, defines, defines_len);

	memcpy(&shaderbuff[defines_len], buffer, buffer_len);

	const GLchar* glbuff = shaderbuff;
	glShaderSource(ID, 1, &glbuff, NULL);
	glCompileShader(ID);

	CheckError();

	delete[] shaderbuff;
	return true;
}

bool OGLShader::CompileFromFile(const char *path, const char *defines)
{
	const char* filebuff = ReadFileToBuffer(path);
	if(!filebuff) return false;
	
	bool result = CompileFromBuffer(filebuff, defines);

	delete[] filebuff;
	return result;
}

bool OGLShader::Delete()
{
	if(!created) return false;
	glDeleteShader(ID);
	ID = NULL;
	created = false;
	return true;
}

//---------------------------------------------------------------[OGLProgram]

bool OGLProgram::CheckError()
{
	GLint Result = GL_FALSE;
	int InfoLogLength = 0;

	glGetProgramiv(ID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if(Result == GL_FALSE && InfoLogLength > 0)
	{
		char *message = new char[InfoLogLength];
		glGetProgramInfoLog(ID, InfoLogLength, NULL, message);
		printf("Program Error!\n%s", message);
		//DebugShader(message, InfoLogLength);
		delete[] message;
	}

	return true;
}

bool OGLProgram::Create()
{
	ID = glCreateProgram();
	created = ID != 0;
	return created;
}

bool OGLProgram::AttachShader(OGLShader shader)
{
	glAttachShader(ID, shader.GetID());
	return true;
}

bool OGLProgram::Link()
{
	glLinkProgram(ID);
	return true;
}

bool OGLProgram::Use()
{
	glUseProgram(ID);
	return true;
}

bool OGLProgram::Delete()
{
	if(!created) return false;
	glDeleteProgram(ID);
	ID = NULL;
	created = false;
	return true;
}

u32 OGLProgram::GetUniformLocation(const char *name)
{
	return glGetUniformLocation(ID, name);
}

void OGLProgram::BindAttribLocation(int index, const char *name)
{
	glBindAttribLocation(ID, index, name);
}

//---------------------------------------------------------------[SCREENSHOT]
#include "TGA_Header.h"

void Backend_OpenGL::TakeScreenshot()
{
	TGAHEADER header;

	FILE *dump = NULL;
	char filename[32];

	for(int number = 0; number < 999 ; number++)
	{
		sprintf_s(filename, "Kurimpa-SS%03d.tga", number);
		fopen_s(&dump, filename, "rb");
		if(dump) { fclose(dump); dump = NULL; }
		else break;
	}

	fopen_s(&dump, filename, "wb");

	if(dump)
	{
		int w = (int)width;
		int h = (int)height;

		u32 size = w * h * 3; // * 4 is using Alpha
		u8 *buffer = new u8[size];
		glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, buffer);

		memset(&header, 0, sizeof(TGAHEADER));
		header.datatypecode = 0x02; // Uncompressed RGB
		header.width  = w;
		header.height = h;
		header.bitsperpixel = 24; // RGB
		
		fwrite(&header, 1, sizeof(TGAHEADER), dump);
		fwrite(buffer, 1, size, dump);

		fclose(dump);

		delete[] buffer;
	}
}

