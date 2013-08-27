#ifndef TGAHEADER_H
#define TGAHEADER_H

#pragma pack(push)
#pragma pack(1)
struct TGAHEADER
{
	u8  idlength;
	u8  colourmaptype;
	u8  datatypecode;
	
	u16 colourmaporigin;
	u16 colourmaplength;
	u8  colourmapdepth;

	u16 x_origin;
	u16 y_origin;
	u16 width;
	u16 height;
	u8  bitsperpixel;
	u8  imagedescriptor;
};
#pragma pack(pop)

#endif