#ifndef __FOREACH_LINE_H__
#define __FOREACH_LINE_H__
#ifdef __cplusplus
extern "C" {
#endif

char* foreach_line_add(const char* src, const char* beg, const char* end);

char* foreach_line_remove_trailing_space(const char* src);

#ifdef __cplusplus
}
#endif
#endif
