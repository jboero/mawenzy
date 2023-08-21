// Basic single-file ray-tracer in OpenCL using Mawsh.
// Writes a 1024x768 BMP image file to the parameter "bmp"
// Buffer will need to be just over 2.5MB.  Use 3m

// Need this line for Mawsh iToS support
#include <mawsh.cl>

#ifndef BMPW
#define BMPW	1024
#endif

#ifndef BMPH
#define BMPH	768
#endif

#ifndef MAWSHBMP
#define MAWSHBMP

// Struct from BMP file header.
// Note we need to use 16 bit vars (max) for struct padding.
typedef struct
{
	uchar	bm[2];			// 2 byte
	ushort	size[2];		// 4 byte
	ushort	reserved[2];	// 4 byte
	ushort	offset[2];		// 4 byte
} bmpHeader;

// Struct from BMP info header.
typedef struct {
	ushort	size[2];			// Header size in bytes (uint)
	ushort	width[2], height[2];// Width and height of image
	ushort	planes;				// Number of colour planes
	ushort	bits;				// Bits per pixel
	ushort	compression[2];		// Compression type
	ushort	imagesize[2];		// Image size in bytes
	ushort	xres[2],yres[2];	// Pixels per meter
	ushort	ncolours[2];		// Number of colours
	ushort	icolors[2];			// Important colours
} bmpInfo;

// Struct for BMP file - char3 vec used for RGB data.
typedef struct {
	bmpHeader		header;
	bmpInfo	 info;

	// Aliginment/padding nightmare
	// Don't use char3 vec, or waste 25% memory and corrupt data.
	uchar4	  argb[BMPW][BMPH];
} bmpFile;

void initBMP(global bmpFile *bmp)
{
	bmp->header.bm[0]		= 'B';
	bmp->header.bm[1]		= 'M';
	iToS(sizeof(bmpFile), bmp->header.size);
	bmp->header.reserved[0] = 0;
	bmp->header.reserved[1] = 0;
	iToS(sizeof(bmpHeader) + sizeof(bmpInfo), bmp->header.offset);

	iToS(sizeof(bmpHeader) + sizeof(bmpInfo) + 2, bmp->info.size);
	iToS(BMPW, bmp->info.width);
	iToS(BMPH, bmp->info.height);
	bmp->info.planes	= 1;
	bmp->info.bits	  = 8;

	iToS(0, bmp->info.compression);
	iToS(BMPW * BMPH * 4, bmp->info.imagesize);
}

#endif