#include "xlist.h"

cu_err_t xlist_create(xlist_config_t* config, xlist_t* list) {
    if(! list) {
        return CU_ERR_INVALID_ARG;
    }

    xlist_t _list = NULL;
    cu_mem_checkr(_list = cu_tctor(xlist_t, struct xlist));

    if(config) {
        _list->data_free_handler = config->data_free_handler;
    }

    *list = _list;
    return CU_OK;
}

int xlist_size(xlist_t list) {
    return ! list ? CU_ERR_INVALID_ARG : (int) list->len;
}

#define _xlist_chain_(CHAINER)            \
    if(! list) {                          \
        return CU_ERR_INVALID_ARG;        \
    }                                     \
    xnode_t _node = NULL;                 \
    cu_mem_checkr(_node = cu_tctor(       \
        xnode_t,                          \
        struct xnode,                     \
        .data = data                      \
    ));                                   \
    list->len++;                          \
    if(! list->head) {                    \
        list->head = list->tail = _node;  \
    } else { CHAINER }                    \
    if(node) { *node = _node; }           \
    return list->len;

int xlist_add_to_back(xlist_t list, void* data, xnode_t* node) {
    _xlist_chain_({
        list->tail->next = _node;
        _node->prev = list->tail;
        list->tail = _node;
    });
}

int xlist_add_to_front(xlist_t list, void* data, xnode_t* node) {
    _xlist_chain_({
        list->head->prev = _node;
        _node->next = list->head;
        list->head = _node;
    });
}

#define _xlist_getter_(list, index, ptr, SETTER)  \
    if(! list || ! ptr || index < 0) {            \
        return CU_ERR_INVALID_ARG;                \
    }                                             \
    if(list->len == 0) {                          \
        return CU_ERR_NOT_FOUND;                  \
    }                                             \
    int i;                                        \
    if((unsigned int) index <= (list->len - 1) / 2) {     \
        i = 0;                                    \
        xlist_veach(list, {                       \
            if(i++ == index) {                    \
                { SETTER }                        \
                return CU_OK;                     \
            }                                     \
        });                                       \
    } else {                                      \
        i = list->len - 1;                        \
        xlist_veachr(list, {                      \
            if(i-- == index) {                    \
                { SETTER }                        \
                return CU_OK;                     \
            }                                     \
        });                                       \
    }                                             \
    return CU_ERR_NOT_FOUND;

cu_err_t xlist_get(xlist_t list, int index, xnode_t* node) {
    _xlist_getter_(list, index, node, {
        *node = xnode;
    });
}

cu_err_t xlist_get_data(xlist_t list, int index, void** data) {
    _xlist_getter_(list, index, data, {
        *data = xnode->data;
    });
}

/**
 *  @brief Call this function only when sure that node belongs to list
 */
static void _xlist_popfree(xlist_t list, xnode_t node) {
    if(node->prev) { node->prev->next = node->next; }
    else { list->head = node->next; }
    if(node->next) { node->next->prev = node->prev; }
    else { list->tail = node->prev; }
    node->next = node->prev = NULL;
    if(list->data_free_handler) { list->data_free_handler(node->data); }
    node->data = NULL;
    free(node);
    list->len--;
}

cu_err_t xlist_remove(xlist_t list, xnode_t node) {
    if(! list || ! node) {
        return CU_ERR_INVALID_ARG;
    }

    xlist_veach(list, {
        if(xnode == node) {
            _xlist_popfree(list, xnode);
            return CU_OK;
        }
    });

    return CU_ERR_NOT_FOUND;
}

int xlist_remove_data(xlist_t list, void* data) {
    if(! list) {
        return CU_ERR_INVALID_ARG;
    }

    xnode_t next = NULL;
    int cnt = 0;

    xlist_veach(list, {
        if(data == xnode->data) {
            next = xnode->next;
            _xlist_popfree(list, xnode);
            cnt++;
            xnode = next;
            continue;
        }
    });

    return cnt > 0 ? cnt : CU_ERR_NOT_FOUND;
}

bool xlist_is_empty(xlist_t list) {
    return ! list ? true : list->len == 0;
}

int xlist_flush(xlist_t list) {
    if(! list) {
        return CU_ERR_INVALID_ARG;
    }

    int cnt = 0;

    while(list->len > 0) {
        _xlist_popfree(list, list->head);
        cnt++;
    }

    return cnt;
}

cu_err_t xlist_destroy(xlist_t list) {
    if(! list) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err;
    if((err = xlist_flush(list)) < 0) {
        return err;
    }

    free(list);
    return CU_OK;
}