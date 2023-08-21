// Basic single-file ray-tracer in OpenCL using Mawsh.
// Writes a 1024x768 BMP image file to the parameter "bmp"
// Buffer will need to be just over 2.5MB.  Use 3m

// Need this line for Mawsh iToS support
#include <mawsh.cl>

#ifndef TIFFW
#define TIFFW	1024
#endif

#ifndef TIFFH
#define TIFFH	768
#endif

#ifndef MAWSHTIFF
#define MAWSHTIFF

#define NTIFFTAGS	11
#define TIFFDIRBUFF	8

// Struct from BMP file header.
// Note we need to use 16 bit vars (max) for struct padding.
typedef struct
{
	uchar	endian[2];  // 2 byte
	ushort	vers;   // 2 byte
	uint	offset;	// 4 byte
} tiffHeader;

typedef struct
{
	ushort	id;
	ushort	type;
	ushort	count[2];
	ushort	offset[2];
} tiffTag;

typedef struct
{
	ushort	nTags;
	tiffTag	tags[NTIFFTAGS];
	ushort	nextTags[2];
	char	buff[TIFFDIRBUFF];
} tiffDir;

// Struct for BMP file - char3 vec used for RGB data.
typedef struct {
	tiffHeader	header;
    tiffDir     dir;
	uchar4      rgba[TIFFW][TIFFH];
} tiffFile;

typedef enum
{
	WIDTH		= 256,
	HEIGHT		= 257,
	DEPTH		= 258,
	COMP		= 259,
	PHOTOMEC	= 262,
	NAME		= 269,
	STRIPOFF	= 273,
	ORIENT		= 274,
	SAMPLES		= 277,
	ROWS		= 278,
	STRIPBYTE	= 279,
	XRES		= 282,
	YRES		= 283,
	PLANAR		= 284,
	PAGENAME	= 285,

	RESUNIT		= 296,
	EXTRASAMP	= 338,
	SAMPLEFRM	= 339
} tagID;

typedef enum
{
	BYTE	= 1,
	ASCII	= 2,
	USHORT	= 3,
	LONG32	= 4,
	RATION	= 5
} tagType;

void setTag(global tiffTag *t, tagID id, tagType type, uint count, uint off)
{
	t->id = id;
	t->type = type;
	iToS(count, &t->count);
	iToS(off, &t->offset);
}

void initTIFF(global tiffFile *tiff)
{
	// Offset is absolute offset for dir arrays.
	int off = sizeof(tiffHeader) + sizeof(tiffDir) - TIFFDIRBUFF;
	int i, tag = 0;
	global char* writer = tiff->dir.buff;

    // II is "intel" order aka little endian
	tiff->header.endian[0] = tiff->header.endian[1] = 'I';
    tiff->header.vers = 42;
    tiff->header.offset = sizeof(tiffHeader);

    tiff->dir.nTags	= NTIFFTAGS;
	iToS(0, &tiff->dir.nextTags);

	setTag(&tiff->dir.tags[tag++], WIDTH,	USHORT, 1, TIFFW);
	setTag(&tiff->dir.tags[tag++], HEIGHT,	USHORT, 1, TIFFH);
	setTag(&tiff->dir.tags[tag++], DEPTH,	USHORT, 4, off);
	for (i = 0; i < 4; ++i)
	{
		*writer++	= 0x08;
		*writer++	= 0x00;
		off += 2;
	}

	setTag(&tiff->dir.tags[tag++], COMP,		USHORT, 1, 1);
	setTag(&tiff->dir.tags[tag++], PHOTOMEC,	USHORT, 1, 2);
	//setTag(&tiff->dir.tags[tag++], NAME,		ASCII, 0, off);

	// Alignment adds 2 bytes here....
	setTag(&tiff->dir.tags[tag++], STRIPOFF,	LONG32, 1, sizeof(tiffHeader) + sizeof(tiffDir) + 2);
	setTag(&tiff->dir.tags[tag++], ORIENT,		USHORT, 1, 1);
	setTag(&tiff->dir.tags[tag++], SAMPLES,		USHORT, 1, 4);
	setTag(&tiff->dir.tags[tag++], ROWS,		USHORT, 1, 0xFFFF);
	// We cheat and use one strip as it's just raw buffer.
	setTag(&tiff->dir.tags[tag++], STRIPBYTE,	LONG32, 1, TIFFW * TIFFH * 4);

	//setTag(&tiff->dir.tags[tag++], XRES,		RATION, 1, off);
	//setTag(&tiff->dir.tags[tag++], YRES,		RATION, 1, off);
	//setTag(&tiff->dir.tags[tag++], PLANAR,	USHORT, 1, 1);
	//setTag(&tiff->dir.tags[tag++], PAGENAME,	ASCII, 0, off);

	//setTag(&tiff->dir.tags[tag++], RESUNIT,	USHORT, 1, 2);
	setTag(&tiff->dir.tags[tag++], EXTRASAMP,	USHORT, 1, 1);
	//setTag(&tiff->dir.tags[tag++], SAMPLEFRM,	USHORT, 4, off);
}

#endif
