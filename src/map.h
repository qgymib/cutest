#ifndef __CUTEST_MAP_H__
#define __CUTEST_MAP_H__

#include "cutest.h"

/**
 * @brief Map initializer helper
 * @param [in] fn       Compare function
 * @param [in] arg      User defined argument
 */
#define CUTEST_MAP_INIT(fn, arg)      { NULL, { fn, arg }, 0 }

#ifdef __cplusplus
extern "C" {
#endif

cutest_map_node_t* cutest_map_begin(const cutest_map_t* handler);
cutest_map_node_t* cutest_map_next(const cutest_map_node_t* node);
int cutest_map_insert(cutest_map_t* handler, cutest_map_node_t* node);

#ifdef __cplusplus
}
#endif

#endif
