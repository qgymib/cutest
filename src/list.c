#include "cutest.h"

static void _test_list_set_once(cutest_list_t* handler, cutest_list_node_t* node)
{
    handler->head = node;
    handler->tail = node;
    node->p_after = NULL;
    node->p_before = NULL;
    handler->size = 1;
}

/**
 * @brief Insert a node to the tail of the list.TEST_LIST_INITIALIZER
 * @warning the node must not exist in any list.
 * @param [in] handler  Pointer to list
 * @param [in] node     Pointer to a new node
 */
void cutest_list_push_back(cutest_list_t* handler, cutest_list_node_t* node)
{
    if (handler->head == NULL)
    {
        _test_list_set_once(handler, node);
        return;
    }

    node->p_after = NULL;
    node->p_before = handler->tail;
    handler->tail->p_after = node;
    handler->tail = node;
    handler->size++;
}

/**
 * @brief Get the last node.
 * @param [in] handler  Pointer to list
 * @return              The first node
 */
cutest_list_node_t* cutest_list_begin(const cutest_list_t* handler)
{
    return handler->head;
}

/**
 * @brief Get next node.
 * @param [in] node     Current node
 * @return              The next node
 */
cutest_list_node_t* cutest_list_next(const cutest_list_node_t* node)
{
    return node->p_after;
}

/**
 * @brief Get the number of nodes in the list.
 * @param [in] handler  Pointer to list
 * @return              The number of nodes
 */
size_t cutest_list_size(const cutest_list_t* handler)
{
    return handler->size;
}

/**
 * @brief Delete a exist node
 * @warning The node must already in the list.
 * @param [in] handler  Pointer to list
 * @param [in] node     The node you want to delete
 */
void cutest_list_erase(cutest_list_t* handler, cutest_list_node_t* node)
{
    handler->size--;

    if (handler->head == node && handler->tail == node)
    {
        handler->head = NULL;
        handler->tail = NULL;
        return;
    }

    if (handler->head == node)
    {
        node->p_after->p_before = NULL;
        handler->head = node->p_after;
        return;
    }

    if (handler->tail == node)
    {
        node->p_before->p_after = NULL;
        handler->tail = node->p_before;
        return;
    }

    node->p_before->p_after = node->p_after;
    node->p_after->p_before = node->p_before;
    return;
}
