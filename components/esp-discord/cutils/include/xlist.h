#ifndef _CUTILS_XLIST_H_
#define _CUTILS_XLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Linked list
 */
typedef struct xlist* xlist_t;

/**
 * @brief Llinked list node
 */
typedef struct xnode* xnode_t;

/**
 * @brief List node data free handler
 */
typedef void(*xnode_free_handler_t)(void* data);

struct xnode {
    xnode_t prev;  /*<! Previous node */
    xnode_t next;  /*<! Next node */
    void* data;    /*<! Node data */
};

struct xlist {
    xnode_t head;                            /*<! First node */
    xnode_t tail;                            /*<! Last node */
    unsigned int len;                                /*<! List size */
    xnode_free_handler_t data_free_handler;  /*<! Node data free handler */
};

/**
 * @brief List configuration
 */
typedef struct {
    xnode_free_handler_t data_free_handler;   /*<! Node data free handler */
} xlist_config_t;

#define xlist_xeach(list, start_ptr, direction, DATADEF, CODE) \
    __extension__ ({ if(list) { xnode_t xnode = list->start_ptr; while(xnode) { \
        DATADEF; {CODE} xnode = xnode->direction; \
    } } list; })

#define xlist_veach(list, CODE) \
    xlist_xeach(list, head, next, {}, CODE)

#define xlist_veachr(list, CODE) \
    xlist_xeach(list, tail, prev, {}, CODE)

#define xlist_each(data_type, list, CODE) \
    xlist_xeach(list, head, next, data_type xdata = (data_type) xnode->data, CODE)

#define xlist_eachr(data_type, list, CODE) \
    xlist_xeach(list, tail, prev, data_type xdata = (data_type) xnode->data, CODE)

#define xlist_get_tdata(list, index, data_type, data_ptr) \
    __extension__ ({ \
        void* _xdata = NULL; cu_err_t err = xlist_get_data(list, index, &_xdata); \
        if(err == CU_OK) { data_ptr = (data_type) _xdata; } err; \
    })

#define xlist_vadd_to_back(list, data) xlist_add_to_back(list, data, NULL)
#define xlist_vadd_to_front(list, data) xlist_add_to_front(list, data, NULL)
#define xlist_add(list, data, node) xlist_add_to_back(list, data, node)
#define xlist_vadd(list, data)  xlist_add(list, data, NULL)

/**
 * @brief Create new list
 * @param config List configuration (optional)
 * @param list List reference
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
cu_err_t xlist_create(xlist_config_t* config, xlist_t* list);

/**
 * @brief Check current size of the list
 * @param list List
 * @return Number of nodes in the list on success, otherwise:
 *         CU_ERR_INVALID_ARG
 */
int xlist_size(xlist_t list);

/**
 * @brief Add new node with data to the end of the list
 * @param list List
 * @param data Data reference
 * @param node New node reference
 * @return New list size on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
int xlist_add_to_back(xlist_t list, void* data, xnode_t* node);

/**
 * @brief Add new node with data to the front of the list
 * @param list List
 * @param data Data reference
 * @param node New node reference
 * @return New list size on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
int xlist_add_to_front(xlist_t list, void* data, xnode_t* node);

/**
 * @brief Get node by the index
 * @param list List
 * @param index Index
 * @param node Node reference
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NOT_FOUND
 */
cu_err_t xlist_get(xlist_t list, int index, xnode_t* node);

/**
 * @brief Get node data by the index
 * @param list List
 * @param index Index
 * @param node Data reference
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NOT_FOUND
 */
cu_err_t xlist_get_data(xlist_t list, int index, void** data);

/**
 * @brief Remove node from the list
 * @param list List
 * @param node Node
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NOT_FOUND
 */
cu_err_t xlist_remove(xlist_t list, xnode_t node);

/**
 * @brief Remove all occurrences of data in the list
 * @param list List
 * @param data Data reference
 * @return Number of deleted nodes on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NOT_FOUND
 */
int xlist_remove_data(xlist_t list, void* data);

/**
 * @brief Check if list is empty
 * @param list List
 * @return true if list is empty (or NULL)
 */
bool xlist_is_empty(xlist_t list);

/**
 * @brief Remove all nodes from the list
 * @param list List
 * @return Number of deleted nodes on success, otherwise:
 *         CU_ERR_INVALID_ARG
 */
int xlist_flush(xlist_t list);

/**
 * @brief Flush and free the memory occupied by the list
 * @param list List
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG
 */
cu_err_t xlist_destroy(xlist_t list);

#ifdef __cplusplus
}
#endif

#endif