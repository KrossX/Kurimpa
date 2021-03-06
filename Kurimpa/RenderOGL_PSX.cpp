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
#include "RenderOGL_PSX.h"

#define GLEW_STATIC
#include <gl/glew.h>
#include <gl/wglew.h>

#include <string>
#include <fstream>
#include <vector>

WNDPROC oldWinProc = NULL; // Parent Window Prodedure function
RenderOGL_PSX *ogl_psx = NULL; // Current instance

RenderOGL_PSX::RenderOGL_PSX()
{
	linearfilter = true;
	usevsync = false;
	fullscreen = false;
	show_vram = false;

	psx.vram = NULL;
	psx.width = psx.height = 0;
	psx.ox = psx.oy = 0;
	psx.offx1 = psx.offx2 = 0;
	psx.offy1 = psx.offy2 = 0;

	gl.tVRAM = 0;
	gl.background_uv = 0;
	gl.background_va = 0;
	gl.background_vb = 0;
}

//---------------------------------------------------------------[WINDOWSTUFF]

void RenderOGL_PSX::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static bool kbuff[256];

	switch (msg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		kbuff[wp] = true;

		switch(wp)
		{
		case VK_MULTIPLY: ToggleFiltering(); break;
		case VK_ADD: ToggleVsync(); break;
		case VK_SUBTRACT: ToggleVRAM(); break;
		case VK_RETURN: if(kbuff[VK_MENU]) ToggleFullscreen(); break;
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		switch(wp)
		{
		case VK_SNAPSHOT:
			if(show_vram) TakeVRAMshot();
			else          TakeScreenshot();
			break;
		}

		kbuff[wp] = false;
		break;

	case WM_WINDOWPOSCHANGED:
		//WindowResize();
		break;
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(ogl_psx) ogl_psx->WndProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_CLOSE: // Haxz: Clicking X and Emu not stopping...
		oldWinProc(hWnd, WM_KEYDOWN, VK_ESCAPE, 0);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return oldWinProc(hWnd, message, wParam, lParam);
}

//HWND CreateGLWindow(LPCWSTR title, int width, int height)
//{  
//	WNDCLASS windowClass;
//	HWND hWnd;
//	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
//
//	hInstance = GetModuleHandle(NULL);
//
//	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
//	windowClass.lpfnWndProc = (WNDPROC) WndProc;
//	windowClass.cbClsExtra = 0;
//	windowClass.cbWndExtra = 0;
//	windowClass.hInstance = hInstance;
//	windowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
//	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
//	windowClass.hbrBackground = NULL;
//	windowClass.lpszMenuName = NULL;
//	windowClass.lpszClassName = title;
//
//	if (!RegisterClass(&windowClass))
//	return 0;
//
//	hWnd = CreateWindowEx(dwExStyle, title, title, WS_OVERLAPPEDWINDOW,
//	CW_USEDEFAULT, 0, width, height, NULL, NULL, hInstance, NULL);
//
//	return hWnd;
//} 


//---------------------------------------------------------------[TOGGLES]

void RenderOGL_PSX::ToggleVRAM()
{
	show_vram = !show_vram;

	if(show_vram)
	{
		SetAspectRatio(1024.0f/512.0f);
		SetFramebufferSize(1024, 512);
	}
	else
	{
		SetAspectRatio(640.0f/480.0f);
		SetFramebufferSize(psx.width, psx.height);
	}
}

void RenderOGL_PSX::ToggleFullscreen()
{
	fullscreen = !fullscreen;

	if(fullscreen) SetFullscreen();
	else           SetWindowed(640,480);
}

void RenderOGL_PSX::ToggleFiltering()
{
	linearfilter = !linearfilter;
	SetFBfiltering(linearfilter);
}

void RenderOGL_PSX::ToggleVsync()
{
	usevsync = !usevsync;
	wglSwapIntervalEXT(usevsync ? -1 : 0);
}

//---------------------------------------------------------------[DISPLAY]

void RenderOGL_PSX::SetDisplayOffset(int ox, int oy)
{
	psx.ox = ox; psx.oy = oy;
};

void RenderOGL_PSX::SetDisplayMode(int width, int height)
{
	psx.width = width; psx.height = height;
	if(!show_vram) SetFramebufferSize(width, height);
};

void RenderOGL_PSX::SetPSXoffset(int x1, int x2, int y1, int y2)
{
	psx.offx1 = x1; psx.offx2 = x2; psx.offy1 = y1; psx.offy2 = y2;
};

//---------------------------------------------------------------[CONSTANTS]

const GLuint TEX_INTFORMAT = GL_R16UI;
const GLuint TEX_FORMAT    = GL_RED_INTEGER;
const GLuint TEX_TYPE      = GL_UNSIGNED_SHORT;

//---------------------------------------------------------------[INIT]

bool RenderOGL_PSX::CreateVRAMtexture(GLuint *tex)
{
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_RECTANGLE, *tex);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, TEX_INTFORMAT, 1024, 512, 0, TEX_FORMAT, TEX_TYPE, 0);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	if(GLfail("Create tVRAM")) return false;

	return true;
}

#include "Shaders\RenderOGL_PSX_Shaders.inl"

bool RenderOGL_PSX::Init(HWND hWin, u8 *psxvram)
{
	if(!hWin)
	{
		printf("Kurimpa -> Error! Invalid window handle.\n");
		return false;
	};

	//HWND hWin = CreateGLWindow(L"Kurimpa", 640, 480);

	if(!Backend_OpenGL::Init(hWin))
		return false;

	LONG proc = GetWindowLong(hWin, GWL_WNDPROC);
	oldWinProc = (proc && (WNDPROC)proc != WindowProc)? (WNDPROC)proc : DefWindowProc;
	SetWindowLong(hWin, GWL_WNDPROC, (LONG)WindowProc);

	if(!InitFramebuffer(640, 480))
	{
		printf("Kurimpa -> Error! Could not init framebuffer.\n");
		return false;
	}

	SetWindowTextA(hWin, "Kurimpa");
	SetWindowed(640,480);
	
	if(!CreateVRAMtexture(&gl.tVRAM)) return false;
	if(!PrepareDisplayQuad()) return false;

	for(int i = 0; i < PROG_SIZE; i++)
		Prog[i].LoadShadersFromBuff(shader::main_vertex, shader::main_frag, shader::define[i]);

	// This are constant for VRAM
	Prog[PROG_VRAM].Use();
	Prog[PROG_VRAM].SetDisplaySize(0, 0, 1024, 512);
	Prog[PROG_VRAM].SetDisplayOffset(0, 0, 0, 0);

	ogl_psx = this;
	psx.vram = psxvram;

	SetAspectRatio(4.0f/3.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	wglSwapIntervalEXT(usevsync ? -1 : 0);

	while(glGetError() != GL_NO_ERROR) {}; // Clean up errors.

	return true;
}

void RenderOGL_PSX::Shutdown()
{
	psx.vram = NULL;
	ogl_psx = NULL;

	if(oldWinProc && oldWinProc != DefWindowProc)
		SetWindowLong(GetHWindow(), GWL_WNDPROC, (LONG)oldWinProc);

	oldWinProc = NULL;

	// Shaders...
	for(int i = 0; i < PROG_SIZE; i++)
		Prog[i].Delete();

	// Buffers...
	glDeleteBuffers(1, &gl.background_va);
	glDeleteBuffers(1, &gl.background_uv);

	glDeleteTextures(1, &gl.tVRAM);
	glDeleteVertexArrays(1, &gl.background_va);


	Backend_OpenGL::Shutdown();
}

//---------------------------------------------------------------[DRAWING]

void RenderOGL_PSX::Present(bool is24bpp, bool disabled)
{
	if(disabled && !show_vram)
	{
		glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
		SwapBuffers(GetDeviceCtx());
		return;
	}

	BindFramebuffer();

	if(show_vram)
	{
		Prog[PROG_VRAM].Use();
		glViewport(0, 0, 1024, 512);
	}
	else
	{
		u8 prog = is24bpp ? PROG_FB24 : PROG_FB16;

		Prog[prog].Use();
		Prog[prog].SetDisplaySize(psx.ox, psx.oy, psx.width - 1, psx.height - 1);
		Prog[prog].SetDisplayOffset(psx.offx1, psx.offy1, psx.offx2, psx.offy2);
		glViewport(0, 0, psx.width, psx.height);
	}

	// Select and update VRAM texture
	glBindTexture(GL_TEXTURE_RECTANGLE, gl.tVRAM);
	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 1024, 512, TEX_FORMAT, TEX_TYPE, psx.vram);

	DrawDisplayQuad();
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	EndFrame();
}

//---------------------------------------------------------------[SHADER]

bool psxProgram::LoadShadersFromBuff(const char *vbuff, const char *fbuff, const char* defs)
{
	OGLShader fragment, vertex;

	vertex.Create(GL_VERTEX_SHADER);
	vertex.CompileFromBuffer(vbuff, defs);

	fragment.Create(GL_FRAGMENT_SHADER);
	fragment.CompileFromBuffer(fbuff, defs);

	Create();
	
	AttachShader(vertex);
	AttachShader(fragment);

	BindAttribLocation(0, "QuadPos");
	BindAttribLocation(1, "QuadUV");

	Link();

	vertex.Delete();
	fragment.Delete();

	Sampler       = GetUniformLocation("Sampler");
	DisplaySize   = GetUniformLocation("DisplaySize");
	DisplayOffset = GetUniformLocation("DisplayOffset");

	Use();
	glUniform1i(Sampler, 0);
	//glBindSampler(0, Prog[i].SamplerID);

	return true;
}

void psxProgram::SetDisplaySize(int ox, int oy, int width, int height)
{
	glUniform4i(DisplaySize, ox, oy, width, height);
}

void psxProgram::SetDisplayOffset(int offx1, int offy1, int offx2, int offy2)
{
	glUniform4i(DisplayOffset, offx1, offy1,offx2, offy2);
}

//---------------------------------------------------------------[SCREENSHOT]
#include "TGA_Header.h"


void RenderOGL_PSX::TakeVRAMshot()
{
	TGAHEADER header;

	FILE *dump = NULL;
	char filename[32];

	for(int number = 0; number < 999 ; number++)
	{
		sprintf_s(filename, "Kurimpa-VRAM%03d.tga", number);
		fopen_s(&dump, filename, "rb");
		if(dump) { fclose(dump); dump = NULL; }
		else break;
	}

	fopen_s(&dump, filename, "wb");

	if(dump)
	{
		u32 size = 1024 * 512 * 3;
		u8 *buffer = new u8[size];

		memset(&header, 0, sizeof(TGAHEADER));
		header.datatypecode = 0x02; // Uncompressed RGB
		header.width  = 1024;
		header.height = 512;
		header.bitsperpixel = 24; // RGB

		for(int y = 0; y < 512; y++)
		for(int x = 0; x < 1024; x++)
		{
			u32 pos = x+ y * 1024;
			u16 *vram = (u16*)psx.vram;
			RGBA5551 pix16(vram[pos]);

			pos = (x + (511 - y) * 1024) * 3;

			buffer[pos + 2] = (pix16.R * 0xFF) / 0x1F;
			buffer[pos + 1] = (pix16.G * 0xFF) / 0x1F;
			buffer[pos + 0] = (pix16.B * 0xFF) / 0x1F;
			//buffer[pos + 3] = pix16.pix.A ? 0xFF : 0x00;
		}

		fwrite(&header, 1, sizeof(TGAHEADER), dump);
		fwrite(buffer, 1, size, dump);
		fclose(dump);

		delete[] buffer;
	}
}

