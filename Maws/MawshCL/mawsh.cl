// Mawenzy common v0.3.2
// John Boero

#ifndef MAWSH_COMMON
#define MAWSH_COMMON

// Note we can override this
#ifndef MAWSTRBUFF
#define MAWSTRBUFF 4096
#endif

typedef struct
{
	int len;
	char buf[MAWSTRBUFF];
} mawstr;

typedef struct mawproc
{
	mawstr out, err, env;
} mawproc;

/*
#ifndef MAWSPRINT
#define MAWSPRINT
void sprint(global char *str, constant char* append)
{
	while (!*append)
		*(str++) = *(append++);
}
#endif
*/

void printmcn(global mawstr *str, constant char* append, int max)
{
	while ((max-- && (str->buf[str->len++] = *append++)));
	str->buf[str->len--] = '\0';
}

void printmc(global mawstr *str, constant char* append)
{
	while ((str->buf[str->len++] = *append++));
	str->buf[str->len--] = '\0';
}

void printmgn(global mawstr *str, global char* append, int max)
{
	while ((max-- && (str->buf[str->len++] = *append++)));
	str->buf[str->len--] = '\0';
}

void printmg(global mawstr *str, global char* append)
{
	while ((str->buf[str->len++] = *append++)) ;
	str->len--;
}

void printmi(global mawstr *str, long i)
{
    char s[64], *iter = s;

	if (i == 0)
		str->buf[str->len++] = '0';
    else if (i < 0)
    {
        str->buf[str->len++] = '-';
        i *= -1;
    }

    while (i != 0)
    {
        *iter++ = (char)(i % 10 + '0');
        i /= 10;
    }

    for (int len = iter - s - 1; len >= 0; --len)
        str->buf[str->len++] = *--iter;
    str->buf[str->len] = '\0';
}

// Very half-assed pseudorandom.
uint rand(global uint *seed)
{
	return *seed = (2905180 * *seed + 15298375) % 1259861259;
}

// Integer (unsigned) <=> 2 shorts
// This skirts 4-byte struct alignment padding.
void iToS(uint val, global ushort *target)
{
	target[0] = val;
	target[1] = val >> 16;
}

#ifndef STOI
#define STOI
// This may be defined in other packages.
uint sToI(global ushort *target)
{
	return target[0] & (target[1] >> 16);
}
#endif

int strlen(global char *c)
{
	int len = 0;
	while (c[len]++);
	return len;
}

char strncmpg(global char *x, global char *y, int len)
{
	while (len > 0 && *x && (*x++ == *y++))
		len--;

	return *x - *y;
}

char strcmp(global char *x, constant char *y)
{
	while (*x && (*x++ == *y++));
	return *x - *y;
}

// Shortcut macros!
#define mpr(a)(printmc(&p->out, a))

#define G0 get_global_id(0)
#define G1 get_global_id(1)
#define G2 get_global_id(2)
#define L0 get_local_id(0)
#define L1 get_local_id(1)
#define L2 get_local_id(2)

#endif
