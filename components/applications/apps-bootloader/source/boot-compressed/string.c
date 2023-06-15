/*
 * arch/mips/boot/compressed/string.c
 *
 * Very small subset of simple string routines
 */

void *memcpy(void *dest, const void *src, unsigned int n)
{
	unsigned int i;
	const char *s = src;
	char *d = dest;

	for (i = 0; i < n; i++)
		d[i] = s[i];
	return dest;
}

void *memset(void *s, int c, unsigned int n)
{
	unsigned int i;
	char *ss = s;

	for (i = 0; i < n; i++)
		ss[i] = c;
	return s;
}
