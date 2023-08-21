#ifndef MAWSHTML
#define MAWSHTML

#ifndef MAWSPRINT
#define MAWSPRINT
void sprint(global char *str, constant char* append)
{
	while (!*append)
		*(str++) = *(append++);
}
#endif

#define htopen()(printmc(&p->out, "<HTML>"))
#define htend()(printmc(&p->out, "</HTML>"))

#endif