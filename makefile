#NMAKE

#CC_FLAGS = /c /Ox /Os /GR- /ML /LD /nologo /W3
CC_FLAGS = /c /Od /GR- /ML /LD /nologo /W3
LINKER_FLAGS = /SUBSYSTEM:WINDOWS /NOLOGO /DLL /DEF:src/kurimpa.def

all:
	@cl $(CC_FLAGS) src/kurimpa_win32.c -Fogpukurimpa.obj
	@link gpukurimpa.obj user32.lib gdi32.lib opengl32.lib $(LINKER_FLAGS)
	@del *.obj
	@del *.lib
	@del *.exp
	@copy gpukurimpa.dll e:\emulation\epsxe\plugins /Y
