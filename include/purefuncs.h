#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#include <hollywood/types.h>

#ifdef HW_AMIGA

#ifdef __cplusplus
extern "C"
{
#endif

void *pure_malloc(size_t size);
void *pure_realloc(void *memblock, size_t size);
void *pure_calloc(size_t num, size_t size);
void pure_free(void *ptr);
size_t pure_strlen(const char *src);
char *pure_strcat(char *dst, const char *src);
char *pure_strdup(const char *str);
FILE *pure_fopen(char *filename, char *modes);
int pure_fclose(FILE *stream);
int pure_fseek(FILE *fp, long offset, int how);
long pure_ftell(FILE *fp);
int pure_ferror(FILE *fp);
int pure_feof(FILE *fp);
size_t pure_fread(void *buf, size_t objsize, size_t nobjs, FILE *fp);
size_t pure_fwrite(const void *buf, size_t objsize, size_t nobjs, FILE *fp);
int pure_snprintf(char *buffer, size_t count, const char *format, ...);
int pure_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);
int pure_printf(const char *format, ...);
void pure_qsort(void *array, size_t num, size_t size, int (*cmpfunc)(const void *arg1, const void *arg2));
int pure_sscanf(const char *str, const char *ctl, ...);
double pure_strtod(const char *s, char **tp);
long pure_strtol(const char *str, char **tail, int base);
int pure_gettimeofday(void *tv, void *restrict);
int pure_toupper(int c);

#ifdef __cplusplus
}
#endif

#else

#define pure_malloc malloc
#define pure_free free
#define pure_realloc realloc
#define pure_calloc calloc
#define pure_fopen fopen
#define pure_fclose fclose
#define pure_fread fread
#define pure_feof feof
#define pure_ferror ferror
#define pure_fwrite fwrite
#define pure_strlen strlen
#define pure_strcat strcat
#define pure_strtod strtod
#define pure_strtol strtol
#define pure_sscanf sscanf
#define pure_toupper toupper

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

#ifdef __VBCC__
#define pure_snprintf snprintf
#define pure_vsnprintf vsnprintf
#elif defined(_MSC_VER)
#define pure_snprintf _snprintf
#define pure_vsnprintf _vsnprintf
#else
#define pure_snprintf snprintf
#define pure_vsnprintf vsnprintf
#endif

#define pure_printf printf

#ifdef __VBCC__
void pure_qsort(void *array, size_t num, size_t size, int (*cmpfunc)(const void *arg1, const void *arg2));
char *mystrdup(const char *s1);
#define pure_strdup(str) mystrdup(str);
#else
#define pure_qsort qsort
#define pure_strdup(str) strdup(str)
#endif

int pure_gettimeofday(void *tv, void *restrict);

#endif

#ifndef HOLLYWOOD_PLATFORM_H
APTR hwos_Open(STRPTR name, int mode);
int hwos_Close(APTR fh);
ULONG hwos_Seek(APTR fh, ULONG pos, int mode);
int hwos_FRead(APTR fh, APTR block, ULONG blocklen);
int hwos_LoadImage(STRPTR filename, ULONG **retrgb, int *retwidth, int *retheight);
void hwos_TmpNam(STRPTR buf);
int hwos_DeleteFile(STRPTR file);
void hwos_GetCurrentDir(STRPTR buf, int len);
int hwos_AddPart(STRPTR dirname, STRPTR filename, int size);
STRPTR hwos_FindTTFFont(STRPTR name, int weight, int slant, int fileonly, int *retoffset, int *retlen, int *rettmp);
#endif

int hwos_isstreaming(APTR fh);

long int pure_lrint(double x);
long pure_lrintf(float x);
double pure_rint(double x);

void debugstring(const char *s);

int validate(const char *s);
int getnextchar(const char *s, int *idx);
void composechar(char *s, int ch);
