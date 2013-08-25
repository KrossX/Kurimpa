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

bool _DebugOpen()
{
	shaderlog = fopen("kurimpa.shaderlog", "w");

	logfile = fopen("kurimpa.log", "w");
	if(logfile) fprintf(logfile, "DebugOpen\n");

	return !!logfile;
}

void _DebugClose()
{
	if(shaderlog) fclose(shaderlog);

	if(logfile) fprintf(logfile, "DebugClose\n");
	if(logfile) fclose(logfile);
	logfile = NULL;
}

void _DebugShader(const char* message, int length)
{
	if(!shaderlog) return;

	fwrite(message, 1, length - 1, shaderlog);
	fflush(shaderlog);
}

void _DebugFunc(const char* func)
{
	if(!logfile && !_DebugOpen()) return;
	//if(!GetAsyncKeyState(VK_F9)) return;

	fprintf(logfile, "%s\n", func);
	fflush(logfile);
}

void _DebugPrint(const char* func, const char* fmt, ...)
{
	if(!logfile && !_DebugOpen()) return;
	//if(!GetAsyncKeyState(VK_F9)) return;

	va_list args;
	
	fprintf(logfile, "%s : ", func);
	
	va_start(args,fmt);
	vfprintf(logfile, fmt, args);
	va_end(args);
	
	fprintf(logfile, "\n");
	fflush(logfile);
}
