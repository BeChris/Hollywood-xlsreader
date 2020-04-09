#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <hollywood/plugin.h>

#include "purefuncs.h"

#if defined(HW_MACOS) || defined(HW_IOS)
#undef vsnprintf
#endif

extern hwPluginAPI *hwcl;

int validate(const char *s)
{
	return hwcl->UnicodeBase->validate(s);
}

int getnextchar(const char *s, int *idx)
{
	return hwcl->UnicodeBase->getnextchar(s, idx);
}

void composechar(char *s, int ch)
{
	hwcl->UnicodeBase->composechar(s, ch);
}

void *pure_malloc(size_t size)
{
	return hwcl->CRTBase->malloc(size);
}

void *pure_realloc(void *memblock, size_t size)
{
	return hwcl->CRTBase->realloc(memblock, size);
}

void *pure_calloc(size_t num, size_t size)
{
	return hwcl->CRTBase->calloc(num, size);
}

void pure_free(void *ptr)
{
	hwcl->CRTBase->free(ptr);
}

#ifndef HW_LINUX
char *pure_strdup(const char *str)
{
	return hwcl->CRTBase->strdup(str);
}
#endif

int pure_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
	return hwcl->CRTBase->vsnprintf(buffer, count, format, argptr);
}

int pure_snprintf(char *buffer, size_t count, const char *format, ...)
{
	va_list args;
	int r;

	va_start(args, format);
	r = pure_vsnprintf(buffer, count, format, args);
	va_end(args);

	return r;
}

int pure_printf(const char *format, ...)
{
	va_list args;
	int r;

	va_start(args, format);
	r = hwcl->CRTBase->vprintf(format, args);
	va_end(args);

//	  hwcl->SysBase->hwos_Delay(2000);

	return r;
}

int pure_sscanf(const char *str, const char *ctrl, ...)
{
	va_list args;
	int r;

	va_start(args, ctrl);
	r = hwcl->CRTBase->vsscanf(str, ctrl, args);
	va_end(args);

	return r;
}

#ifdef toupper
#undef toupper
#endif

int pure_toupper(int c)
{
	return hwcl->CRTBase->toupper(c);
}

void pure_qsort(void *array, size_t num, size_t size, int (*cmpfunc)(const void *arg1, const void *arg2))
{
	hwcl->CRTBase->qsort(array, num, size, cmpfunc);
}

double pure_strtod(const char *s, char **tp)
{
	return hwcl->CRTBase->strtod(s, tp);
}

long pure_strtol(const char *str, char **tail, int base)
{
	return hwcl->CRTBase->strtol(str, tail, base);
}

char *pure_strcat(char *dst, const char *src)
{
	char *bak = dst;

	while (*dst) dst++;

	while(*src)
	{
		*dst = *src;
		dst++;
		src++;
	} 
		
	*dst = 0;
	
	return bak;
}

size_t pure_strlen(const char *src)
{
	int len = 0;

	if(!src) return 0;
		
	while(*src++) len++;
	
	return len;
}

int pure_gettimeofday(void *tv, void *rest)
{
	return hwcl->CRTBase->gettimeofday(tv, rest);
}

int hwos_Close(APTR fh)
{
	return hwcl->DOSBase->hw_FClose(fh);
}

ULONG hwos_Seek(APTR fh, ULONG pos, int mode)
{
	return hwcl->DOSBase->hw_FSeek(fh, pos, mode);
}

int hwos_FRead(APTR fh, APTR block, ULONG blocklen)
{
	return hwcl->DOSBase->hw_FRead(fh, block, blocklen);
}

#if defined(HW_AMIGAOS3) || defined(HW_WARPOS) || defined(HW_AROS) || defined(HW_WIN32)

long int pure_lrint(double x)
{
	return hwcl->CRTBase->lrint(x);
}

long pure_lrintf(float x)
{
	return pure_lrint(x);
}

double pure_rint(double x)
{
	return pure_lrint(x);
}

#endif

#if defined(HW_AMIGAOS3) || defined(HW_WARPOS) || defined(HW_WIN32)
double cbrt(double x)
{
	if(x == 0) return 0;

	if(x > 0.0) {
		return(pow(x, (double) 1 / (double) 3));
	} else {
		return(-pow(-x, (double) 1 / (double) 3));
	}
}

float cbrtf(float x)
{
	return (float) cbrt(x);
}
#endif

#if defined(HW_AMIGAOS3) || defined(HW_WARPOS)
float sinf(float x)
{
	return (float) sin(x);
}

float cosf(float x)
{
	return (float) cos(x);
}

float powf(float x, float y)
{
	return (float) pow(x, y);
}

float sqrtf(float x)
{
	return (float) sqrt(x);
}

float expf(float x)
{
	return (float) exp(x);
}

float log10f(float x)
{
	return (float) log10(x);
}
#endif

#if defined(__VBCC__) || defined(HW_WIN32)

int strcasecmp(const char *s1, const char *s2)
{
	return hwcl->CRTBase->stricmp(s1, s2);
}

int strncasecmp(const char *s1, const char *s2, int n)
{
	return hwcl->CRTBase->strnicmp(s1, s2, n);
}
#endif

#ifdef __VBCC__

float fabsf(float x)
{
	return (x < 0) ? -x : x;
}

#endif

