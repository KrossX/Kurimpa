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

FILE *logfile = NULL;
FILE *shaderlog = NULL;

#define BUFFSIZE 1024

void _Print2File(const char* str)
{
	static char buff[BUFFSIZE];
	static u32 counter = 0;

	if(strcmp(str, buff) == 0)
	{
		counter++;
	}
	else
	{
		if(counter) fprintf(logfile, "%s <x%u>\n", buff, counter);
		else        fprintf(logfile, "%s\n", buff);

		memcpy(buff, str, BUFFSIZE);
		fflush(logfile);
		counter = 0;
	}
}

bool DebugOpen()
{
	logfile = fopen("kurimpa.log", "w");
	if(logfile) fprintf(logfile, "DebugOpen\n");
	return logfile != NULL;
}

void DebugClose()
{
	if(shaderlog) fclose(shaderlog);
	shaderlog = NULL;

	if(logfile)
	{
		_Print2File(""); // flush last repeat
		fprintf(logfile, "DebugClose\n");
		fclose(logfile);
		logfile = NULL;
	}
}

void _DebugShader(const char* message, int length)
{
	if(!shaderlog) shaderlog = fopen("kurimpa.shaderlog", "w");
	if(!shaderlog) return;

	fwrite(message, 1, length - 1, shaderlog);
	fflush(shaderlog);
}

void _DebugFunc(const char* func)
{
	if(!logfile) DebugOpen();
	if(!logfile) return;
	if(!GetAsyncKeyState(VK_INSERT)) return;

	char buff[BUFFSIZE];
	sprintf(buff, "%s", func);
	_Print2File(buff);
}

void _DebugPrint(const char* func, const char* fmt, ...)
{
	if(!logfile) DebugOpen();
	if(!logfile) return;
	if(!GetAsyncKeyState(VK_INSERT)) return;

	va_list args;
	char buff[2][BUFFSIZE];

	va_start(args,fmt);
	vsprintf(buff[1], fmt, args);
	va_end(args);

	sprintf(buff[0], "%s : %s", func, buff[1]); 
	_Print2File(buff[0]);
}
