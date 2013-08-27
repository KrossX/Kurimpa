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

#ifndef BACKEND_OPENGL_H
#define BACKEND_OPENGL_H

class Backend_OpenGL
{
	RECT oldrect;
	LONG oldstyle;
	float width, height; // Screensize

	HGLRC hTempGLCtx;   // Temporal OpenGL Context
	HGLRC hRenderCtx;   // Rendering Context
	HDC hDeviceCtx;     // Device Context
	HWND hWindow;       // Window Handle

protected:
	void SetScreensize(float width, float height);
	void SetFullscreen();
	void SetWindowed(int width, int height);
	float GetWidth() { return width; };
	float GetHeight() { return height; };
	HWND GetHWindow() { return hWindow; };
	HDC  GetDeviceCtx() { return hDeviceCtx; };

	void CheckProgramError(u32 ProgramID);
	u32 CompileShader(const char *shader_path, unsigned int type, const char* defines);
	u32 LoadShaders(const char *vertex_file_path, const char *fragment_file_path, const char* defines);
	
	bool GLfail(const char* msg);
	bool GLfail(const char* msg, u32 err);
	bool CreateContext(HWND hWnd); // return error code

	bool Init(HWND hWin);
	void Shutdown();
	void TakeScreenshot();

public:
	void UpdateTitle(const char* title);
};

#endif