#ifndef RASTERPSX_H
#define RASTERPSX_H

struct RasterPSX
{
	PSXVRAM *VRAM;
	SGPUSTAT *GPUSTAT;
	DRAWAREA *DA;
	TEXWIN *TW;
	TRANSFER *TR;
	vectk *vertex;

	struct
	{
		bool isTextured;
		bool isRect;
		bool isGouraud;
		bool isMaskForced;
		bool isBlendEnabled;

		bool doModulate;
		bool doDither;
	} Drawing;

	void UpdateDC(u8 DC)
	{
		Drawing.isTextured = !!(DC & DCMD_TEXTURED);
		Drawing.isRect = (DC & DCMD_TYPE) == 0x60; // 0x20 poly, 0x40 line, 0x60 rect
		Drawing.isGouraud = !!(DC & DCMD_GOURAUD);
		Drawing.isMaskForced = !!GPUSTAT->MASKSET;
		Drawing.isBlendEnabled = !!(DC & DCMD_BLENDENABLED);

		Drawing.doModulate = !(DC & DCMD_ENVREPLACE);
		Drawing.doDither = !!GPUSTAT->DITHER && (!Drawing.isRect && Drawing.isGouraud);
	}

	bool doMaskCheck(u16 &x, u16 &y) { return GPUSTAT->MASKCHECK && (VRAM->HALF2[y][x] & 0x8000); }
	bool doMaskCheck(u16 &pix) { return GPUSTAT->MASKCHECK && (pix & 0x8000); }

	void InitPointers(SGPUSTAT *reg, DRAWAREA *da, PSXVRAM *mem, TEXWIN *tw, vectk *vx, TRANSFER *tr)
	{
		GPUSTAT = reg;
		DA = da;
		VRAM = mem;
		TW = tw;
		TR = tr;
		vertex = vx;
	}

	virtual void RasterLine(RENDERTYPE render_mode) {}
	virtual void RasterPoly3(RENDERTYPE render_mode) {}
	virtual void RasterPoly4(RENDERTYPE render_mode) {}
	virtual void RasterRect(RENDERTYPE render_mode, u8 type) {}
	virtual void DrawFill() {}
};

struct RasterPSXSW : public RasterPSX
{
	void RasterLine(RENDERTYPE render_mode);
	void RasterPoly3(RENDERTYPE render_mode);
	void RasterPoly4(RENDERTYPE render_mode);
	void RasterRect(RENDERTYPE render_mode, u8 type);
	void DrawFill();

	inline u16 Blend(u16 &back, u16 &front);

	template<bool textured>
	inline void SetPixel(u32 color, s16 x, s16 y, u16 texel);

	inline u16  GetTexel(u8 tx, u8 ty);
	inline u16  GetTexel3(vectk *v, float s, float t);
};


#endif