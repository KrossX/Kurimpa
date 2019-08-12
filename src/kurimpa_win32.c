#include "kurimpa.h"
#include "psemupro.h"

typedef char* (CALLBACK *t_PSEgetLibName)(void);
typedef UINT  (CALLBACK *t_PSEgetLibType)(void);
typedef UINT  (CALLBACK *t_PSEgetLibVersion)(void);
typedef int   (CALLBACK *t_GPUinit)(void);
typedef int   (CALLBACK *t_GPUshutdown)(void);
typedef void  (CALLBACK *t_GPUmakeSnapshot)(void);
typedef int   (CALLBACK *t_GPUopen)(HWND);
typedef int   (CALLBACK *t_GPUclose)(void);
typedef void  (CALLBACK *t_GPUupdateLace)(void);
typedef UINT  (CALLBACK *t_GPUreadStatus)(void);
typedef UINT  (CALLBACK *t_GPUwriteStatus)(UINT);
typedef UINT  (CALLBACK *t_GPUreadData)(void);
typedef void  (CALLBACK *t_GPUreadDataMem)(UINT*, int);
typedef void  (CALLBACK *t_GPUwriteData)(UINT);
typedef void  (CALLBACK *t_GPUwriteDataMem)(UINT*, int);
typedef void  (CALLBACK *t_GPUsetMode)(UINT);
typedef UINT  (CALLBACK *t_GPUgetMode)(void);
typedef int   (CALLBACK *t_GPUdmaChain)(UINT*, UINT);
typedef int   (CALLBACK *t_GPUconfigure)(void);
typedef void  (CALLBACK *t_GPUabout)(void);
typedef int   (CALLBACK *t_GPUtest)(void);
typedef void  (CALLBACK *t_GPUdisplayText)(char*);
typedef void  (CALLBACK *t_GPUdisplayFlags)(UINT);
typedef int   (CALLBACK *t_GPUfreeze)(UINT, void*);
typedef void  (CALLBACK *t_GPUshowScreenPic)(BYTE*);
typedef void  (CALLBACK *t_GPUgetScreenPic)(BYTE*);

t_PSEgetLibName		wrap_PSEgetLibName;
t_PSEgetLibType		wrap_PSEgetLibType;
t_PSEgetLibVersion	wrap_PSEgetLibVersion;
t_GPUinit			wrap_GPUinit;
t_GPUshutdown		wrap_GPUshutdown;
t_GPUmakeSnapshot	wrap_GPUmakeSnapshot;
t_GPUopen			wrap_GPUopen;
t_GPUclose			wrap_GPUclose;
t_GPUupdateLace		wrap_GPUupdateLace;
t_GPUreadStatus		wrap_GPUreadStatus;
t_GPUwriteStatus	wrap_GPUwriteStatus;
t_GPUreadData		wrap_GPUreadData;
t_GPUreadDataMem	wrap_GPUreadDataMem;
t_GPUwriteData		wrap_GPUwriteData;
t_GPUwriteDataMem	wrap_GPUwriteDataMem;
t_GPUsetMode		wrap_GPUsetMode;
t_GPUgetMode		wrap_GPUgetMode;
t_GPUdmaChain		wrap_GPUdmaChain;
t_GPUconfigure		wrap_GPUconfigure;
t_GPUabout			wrap_GPUabout;
t_GPUtest			wrap_GPUtest;
t_GPUdisplayText	wrap_GPUdisplayText;
t_GPUdisplayFlags	wrap_GPUdisplayFlags;
t_GPUfreeze			wrap_GPUfreeze;
t_GPUshowScreenPic	wrap_GPUshowScreenPic;
t_GPUgetScreenPic	wrap_GPUgetScreenPic;

u16 *plugin_mem;
void *window_other;

static void* create_window(void);

static int  video_open(void *handle);
static int  video_close(void);
static void video_update(void);

#include "logger.c"
#include "psxgpu.c"
#include "psemupro.c"
#include "psemupro2.c"

static HANDLE stdout;
static HWND   window;
static HDC    dev_ctx;
static HGLRC  ren_ctx;

WNDPROC oldproc;

GLfloat scr_offx;
GLfloat scr_offy;
GLfloat scr_offy2;
GLfloat scr_w;
GLfloat scr_h;
GLfloat scr_h2;

int wnd_width;
int wnd_height;

int mouse_sx, mouse_sy;
int mouse_dx, mouse_dy;

int delta;
GLfloat zoom = 1;

void video_resize(void)
{
	glViewport(0, 0, wnd_width, wnd_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, wnd_width, 0, wnd_height, -1.0, 1.0);
}

void video_coord_update(void)
{
	scr_w = 1024.0f * zoom;
	scr_h = 1024.0f * zoom;
	
	scr_h2 = 512.0f * zoom;

	scr_offx = (GLfloat)(mouse_dx);
	scr_offy = (GLfloat)(wnd_height - scr_h - mouse_dy);
	
	scr_offy2 = scr_offy + scr_h2;
}

int mouse_buttonheld;

LRESULT CALLBACK video_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_SIZE:
		wnd_width  = LOWORD(lparam);
		wnd_height = HIWORD(lparam);
		video_resize();
		video_coord_update();
		break;
		
	case WM_KEYDOWN:
		if(wparam == '1')
		{
			logger_enabled = 1;
			Beep(500, 100);
		}
		
		if(wparam == '2')
		{
			logger_enabled = 0;
			Beep(200, 100);
		}
		break;
		
	case WM_KEYUP:
		
		break;

	case WM_MOUSEWHEEL:
		delta = (s16)(wparam >> 16) / WHEEL_DELTA;
		zoom = delta > 0 ? zoom * 2.0f : delta < 0 ? zoom * 0.5f : zoom;
		video_coord_update();
		break;

	case WM_LBUTTONDOWN:
		mouse_buttonheld = 1;
		mouse_sx = LOWORD(lparam);
		mouse_sy = HIWORD(lparam);
		break;

	case WM_LBUTTONUP:
		mouse_buttonheld = 0;
		break;

	case WM_MOUSEMOVE:
		if (mouse_buttonheld)
		{
			mouse_dx += LOWORD(lparam) - mouse_sx;
			mouse_dy += HIWORD(lparam) - mouse_sy;

			mouse_sx = LOWORD(lparam);
			mouse_sy = HIWORD(lparam);

			video_coord_update();
		}
		break;
	}

	return CallWindowProc(oldproc, wnd, msg, wparam, lparam);
}

typedef BOOL  (WINAPI wglSwapInterval_t) (int interval);
wglSwapInterval_t *wglSwapInterval;

GLuint main_texture, other_texture;

static
int video_open(void *handle)
{
	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };
	RECT wnd_size = { 0, 0, 1024, 1024 };
	LONG wnd_style;
	int pf;
	BOOL spf;
	
	window = NULL;

	if (handle && IsWindow((HWND)handle))
		window = (HWND)handle;

	if (!window) return -1;
	
	window_other = create_window();
	ShowCursor(TRUE);

	dev_ctx = GetDC(window);
	if (!dev_ctx) return -2;

	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	pf = ChoosePixelFormat(dev_ctx, &pfd);
	if (!pf) return -3;

	spf = SetPixelFormat(dev_ctx, pf, &pfd);
	if (!spf) return -4;

	ren_ctx = wglCreateContext(dev_ctx);
	if (!ren_ctx) return -5;

	wglMakeCurrent(dev_ctx, ren_ctx);
	
	wglSwapInterval = (wglSwapInterval_t*)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapInterval(0);

	glClearColor(0.20f, 0.20f, 0.20f, 1.0f);

	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &main_texture);
	glBindTexture(GL_TEXTURE_2D, main_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glGenTextures(1, &other_texture);
	glBindTexture(GL_TEXTURE_2D, other_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Use WindowLongPtr for 64bit!
	oldproc = (WNDPROC)GetWindowLongA(window, GWL_WNDPROC);
	SetWindowLongA(window, GWL_WNDPROC, (LONG)video_proc);

	wnd_style = GetWindowLongA(window, GWL_STYLE) | WS_OVERLAPPEDWINDOW;
	SetWindowLongA(window, GWL_STYLE, wnd_style);

	AdjustWindowRect(&wnd_size, wnd_style, FALSE);

	wnd_width  = wnd_size.right - wnd_size.left;
	wnd_height = wnd_size.bottom - wnd_size.top;

	SetWindowPos(window, NULL, 0, 0, wnd_width, wnd_height, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOOWNERZORDER);
	return 0;
}

static
int video_close(void)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(ren_ctx);
	ReleaseDC(window, dev_ctx);

	dev_ctx = NULL;
	ren_ctx = NULL;
	return 0;
}

//OGL1.2
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366

static
void video_update(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, main_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512,
		0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, gpu.vram);

	glBegin(GL_TRIANGLE_STRIP);
		//glColor3f(1.0f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(scr_offx, scr_offy2);

		//glColor3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(scr_offx, scr_offy2 + scr_h2);

		//glColor3f(0.0f, 0.0f, 1.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(scr_offx + scr_w, scr_offy2);

		//glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(scr_offx + scr_w, scr_offy2 + scr_h2);
	glEnd();
	
	glBindTexture(GL_TEXTURE_2D, main_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512,
		0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, plugin_mem);
		
	glBegin(GL_TRIANGLE_STRIP);
		//glColor3f(1.0f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(scr_offx, scr_offy);

		//glColor3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(scr_offx, scr_offy + scr_h2);

		//glColor3f(0.0f, 0.0f, 1.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(scr_offx + scr_w, scr_offy);

		//glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(scr_offx + scr_w, scr_offy + scr_h2);
	glEnd();

	SwapBuffers(dev_ctx);
}

static
void* create_window(void)
{
	char wnd_class[128];
	GetClassName(window, wnd_class, 128);
	
	return CreateWindow(wnd_class, "Other plugin", WS_CHILD | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
		window, NULL, GetModuleHandle(NULL), NULL);
}

int load_plugin(void);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		load_plugin();
	}

	return TRUE;
}

int load_plugin(void)
{
	HINSTANCE plugin = LoadLibrary("plugins\\gpuadgpu.dll");
	if (!plugin) return 1;
	
	wrap_PSEgetLibName    = (t_PSEgetLibName)GetProcAddress(plugin, "PSEgetLibName");
	wrap_PSEgetLibType    = (t_PSEgetLibType)GetProcAddress(plugin, "PSEgetLibType");
	wrap_PSEgetLibVersion = (t_PSEgetLibVersion)GetProcAddress(plugin, "PSEgetLibVersion");
	wrap_GPUinit          = (t_GPUinit)GetProcAddress(plugin, "GPUinit");
	wrap_GPUshutdown      = (t_GPUshutdown)GetProcAddress(plugin, "GPUshutdown");
	wrap_GPUmakeSnapshot  = (t_GPUmakeSnapshot)GetProcAddress(plugin, "GPUmakeSnapshot");
	wrap_GPUopen          = (t_GPUopen)GetProcAddress(plugin, "GPUopen");
	wrap_GPUclose         = (t_GPUclose)GetProcAddress(plugin, "GPUclose");
	wrap_GPUupdateLace    = (t_GPUupdateLace)GetProcAddress(plugin, "GPUupdateLace");
	wrap_GPUreadStatus    = (t_GPUreadStatus)GetProcAddress(plugin, "GPUreadStatus");
	wrap_GPUwriteStatus   = (t_GPUwriteStatus)GetProcAddress(plugin, "GPUwriteStatus");
	wrap_GPUreadData      = (t_GPUreadData)GetProcAddress(plugin, "GPUreadData");
	wrap_GPUreadDataMem   = (t_GPUreadDataMem)GetProcAddress(plugin, "GPUreadDataMem");
	wrap_GPUwriteData     = (t_GPUwriteData)GetProcAddress(plugin, "GPUwriteData");
	wrap_GPUwriteDataMem  = (t_GPUwriteDataMem)GetProcAddress(plugin, "GPUwriteDataMem");
	wrap_GPUsetMode       = (t_GPUsetMode)GetProcAddress(plugin, "GPUsetMode");
	wrap_GPUgetMode       = (t_GPUgetMode)GetProcAddress(plugin, "GPUgetMode");
	wrap_GPUdmaChain      = (t_GPUdmaChain)GetProcAddress(plugin, "GPUdmaChain");
	wrap_GPUconfigure     = (t_GPUconfigure)GetProcAddress(plugin, "GPUconfigure");
	wrap_GPUabout         = (t_GPUabout)GetProcAddress(plugin, "GPUabout");
	wrap_GPUtest          = (t_GPUtest)GetProcAddress(plugin, "GPUtest");
	wrap_GPUdisplayText   = (t_GPUdisplayText)GetProcAddress(plugin, "GPUdisplayText");
	wrap_GPUdisplayFlags  = (t_GPUdisplayFlags)GetProcAddress(plugin, "GPUdisplayFlags");
	wrap_GPUfreeze        = (t_GPUfreeze)GetProcAddress(plugin, "GPUfreeze");
	wrap_GPUshowScreenPic = (t_GPUshowScreenPic)GetProcAddress(plugin, "GPUshowScreenPic");
	wrap_GPUgetScreenPic  = (t_GPUgetScreenPic)GetProcAddress(plugin, "GPUgetScreenPic");
	
	plugin_mem = (u16*)((u8*)wrap_GPUfreeze + 0x27A0D8);
	
	return 0;
}