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

#pragma once

void DebugClose();

void _DebugFunc(const char* func);
void _DebugPrint(const char* func, const char* fmt, ...);
void _DebugShader(const char* message, int length);

#define KDEBUG 0

#if KDEBUG > 0

	#define DebugPrint(...)	_DebugPrint(__FUNCTION__, __VA_ARGS__)
	#define DebugFunc()		_DebugFunc(__FUNCTION__)
	#define DebugShader		_DebugShader

#else

	#define DebugPrint(...)
	#define DebugFunc()
	#define DebugShader(...)

#endif