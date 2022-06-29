#ifndef __CUTEST_PRINT_H__
#define __CUTEST_PRINT_H__
#ifdef __cplusplus
extern "C" {
#endif

int _cutest_color_printf(cutest_print_color_t color, const char* fmt, ...);
int cutest_color_fprintf(cutest_print_color_t color, FILE* stream, const char* fmt, ...);
int cutest_color_vfprintf(cutest_print_color_t color, FILE* stream, const char* fmt, va_list ap);

#ifdef __cplusplus
}
#endif
#endif
