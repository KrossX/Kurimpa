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

typedef unsigned __int8	  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef __int8	 s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef float f32;

union Half
{
	u16 U16;
	u8  U8[2];

	s16 S16;
	s8  S8[2];
};

union Word
{
	f32 F32;
	u32 U32;
	u16 U16[2];
	u8  U8 [4];
	
	s32 S32;
	s16 S16[2];
	s8  S8 [4];
};

union DWord
{
	u64 U64;
	f32 F32[2];
	u32 U32[2];
	u16 U16[4];
	u8  U8 [8];

	s64 S64;
	s32 S32[2];
	s16 S16[4];
	s8  S8 [8];

	Word W[2];
};

union QWord
{
	u64 U64[2];
	f32 F32[4];
	u32 U32[4];
	u16 U16[8];
	u8  U8 [16];

	s64 S64[2];
	s32 S32[4];
	s16 S16[8];
	s8  S8 [16];

	Word W[4];
	DWord DW[2];
};