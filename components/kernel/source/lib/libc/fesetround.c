#include <fenv.h>

/* __fesetround wrapper for arch independent argument check */

int __fesetround(int r)
{
	return 0;
}

int fesetround(int r)
{
	if (r != FE_TONEAREST
#ifdef FE_DOWNWARD
		&& r != FE_DOWNWARD
#endif
#ifdef FE_UPWARD
		&& r != FE_UPWARD
#endif
#ifdef FE_TOWARDZERO
		&& r != FE_TOWARDZERO
#endif
	)
		return -1;
	return __fesetround(r);
}

int fegetround(void)
{
	return FE_TONEAREST;
}
