#ifndef BROTLIH
#define BROTLIH

constant ushort brotli_blocks[20] = {
    1024,   1024,   2048,   2048,
    1024,   1024,   1024,   1024,
    1024,   512,    512,    256,
    128,    128,    256,    128,
    128,    64,     64,     32};

// As we have inconsistent lengths, can't use 3 dimensional array sadly.
typedef struct
{
  // Map buffers directly to spec.
  char
	l4[1024][4],	// 0
	l5[1024][5],	// 4096
	l6[2048][6],	// 9216
	l7[2048][7],	// 21504
	l8[1024][8],	// 35840
	l9[1024][9],	// 44032
	l10[1024][10],	// 53248
	l11[1024][11],	// 63488
	l12[1024][12],	// 74752
	l13[512][13],	// 87040
	l14[512][14],	// 93696
	l15[256][15],	// 100864
	l16[128][16],	// 106752
	l17[128][17],	// 108928
	l18[256][18],	// 113536
	l19[128][19],	// 115968
	l20[128][20],	// 118528
	l21[64][21],	// 119872
	l22[64][22],	// 121280
	l23[32][23];	// 122016
} brotlidict;

global char* broindex(global brotlidict *d, int len, int ix)
{
	switch (len)
	{
		case 4: return d->l4[ix];
		case 5: return d->l5[ix];
		case 6: return d->l6[ix];
		case 7: return d->l7[ix];
		case 8: return d->l8[ix];
		case 9: return d->l9[ix];
		case 10: return d->l10[ix];
		case 11: return d->l11[ix];
		case 12: return d->l12[ix];
		case 13: return d->l13[ix];
		case 14: return d->l14[ix];
		case 15: return d->l15[ix];
		case 16: return d->l16[ix];
		case 17: return d->l17[ix];
		case 18: return d->l18[ix];
		case 19: return d->l19[ix];
		case 20: return d->l20[ix];
		case 21: return d->l21[ix];
		case 22: return d->l22[ix];
		case 23: return d->l23[ix];
	}
    return NULL;
}

#endif