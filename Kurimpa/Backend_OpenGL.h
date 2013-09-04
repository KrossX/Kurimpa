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

class OGLShader
{
	u32 ID;
	bool created;
	bool CheckError();

public:
	u32 GetID() { return ID; };
	bool Create(u32 type);
	bool CompileFromBuffer(const char *buffer, const char *defines);
	bool CompileFromFile(const char *path, const char *defines);
	bool Delete();

	OGLShader() { ID = 0; created = false; }
};

class OGLProgram
{
	u32 ID;
	bool created;
	bool CheckError();

public:
	void BindAttribLocation(int index, const char *name);
	u32 GetUniformLocation(const char *name);
	bool Create();
	bool AttachShader(OGLShader shader);
	bool Link();
	bool Use();
	bool Delete();

	OGLProgram() { ID = 0; created = false; }
};


class Backend_OpenGL
{
	OGLProgram fbprog; // Framebuffer main shader
	u32 fbid; // Framebuffer id
	u32 fbcolor; // Framebuffer color texture
	int fbwidth, fbheight; // Framebuffer resolution
	u32 quadva, quadvb, quaduv; // Quad vertex array, buffer and tex coords
	bool fbfiltering; // Linear or nearest

	float aspectr; // Desired aspect ratio;
	RECT viewport; // Viewport size, padding (aspect ratio)
	RECT oldrect;
	LONG oldstyle;
	int scwidth, scheight; // Screensize

	HGLRC hTempGLCtx;   // Temporal OpenGL Context
	HGLRC hRenderCtx;   // Rendering Context
	HDC hDeviceCtx;     // Device Context
	HWND hWindow;       // Window Handle

protected:
	void UpdateScreenAspect();
	void SetAspectRatio(float ratio);
	void SetScreensize(float width, float height);
	void SetFullscreen();
	void SetWindowed(int width, int height);
	float GetWidth() { return (float)fbwidth; };
	float GetHeight() { return (float)fbheight; };
	HWND GetHWindow() { return hWindow; };
	HDC  GetDeviceCtx() { return hDeviceCtx; };

	bool CreateContext(HWND hWnd); // return error code
	
	bool PrepareDisplayQuad();
	void DrawDisplayQuad();

	void SetFBfiltering(bool linear);
	bool InitFramebuffer(int width, int height);
	void BindFramebuffer();
	void SetFramebufferSize(int width, int height);
	void EndFrame();

	bool Init(HWND hWin);
	void Shutdown();
	void TakeScreenshot();

public:
	static bool GLfail(const char* msg);
	static bool GLfail(const char* msg, u32 err);
	void UpdateTitle(const char* title);

	Backend_OpenGL();
};

#endif