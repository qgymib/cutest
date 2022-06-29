#ifndef __CUTEST_LIST_H__
#define __CUTEST_LIST_H__

/**
 * @brief List initializer helper
 */
#define TEST_LIST_INITIALIZER       { NULL, NULL, 0 }

#ifdef __cplusplus
extern "C" {
#endif

cutest_list_node_t* cutest_list_begin(const cutest_list_t* handler);
size_t cutest_list_size(const cutest_list_t* handler);
cutest_list_node_t* cutest_list_next(const cutest_list_node_t* node);
void cutest_list_erase(cutest_list_t* handler, cutest_list_node_t* node);
void cutest_list_push_back(cutest_list_t* handler, cutest_list_node_t* node);

#ifdef __cplusplus
}
#endif

#endif
