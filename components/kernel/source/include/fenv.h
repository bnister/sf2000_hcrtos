#ifndef _FENV_H
#define _FENV_H

#ifdef __cplusplus
extern "C" {
#endif

#define FE_ALL_EXCEPT 0
#define FE_TONEAREST  0

int fegetround(void);
int fesetround(int);

#ifdef __cplusplus
}
#endif
#endif

