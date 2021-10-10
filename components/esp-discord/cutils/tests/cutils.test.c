#include "cutils.h"
#include "estr.h"
#include <assert.h>

struct test_struct {
    int num;
    char chr;
    char* str;
};

typedef struct test_struct test_struct_t;
typedef test_struct_t* test_struct_handle_t;

static char** create_list(int* len);

int main() {
    // testing constants

    assert(CU_OK == 0);
    assert(CU_FAIL == -1);

    // testing attrs

    test_struct_t* sptr = cu_ctor(test_struct_t, 
        .num = 6,
        .chr = 'x',
        .str = "hello world"
    );

    assert(sptr);
    assert(sptr->num == 6);
    assert(sptr->chr == 'x');
    assert(estr_eq(sptr->str, "hello world"));
    free(sptr);

    // testing attrs

    test_struct_handle_t hndl = cu_tctor(test_struct_handle_t, test_struct_t, 
        .chr = 'a',
        .str = "hello"
    );

    assert(hndl);
    assert(hndl->chr == 'a');
    assert(estr_eq(hndl->str, "hello"));
    free(hndl);

    // testing blank ctor

    sptr = cu_ctor(test_struct_t);
    assert(sptr);
    free(sptr);

    // testing blank tctor

    hndl = cu_tctor(test_struct_handle_t, test_struct_t);
    assert(hndl);
    free(hndl);

    // test freeing list

    int len;
    char** list = NULL;

    list = create_list(&len);
    assert(list);
    assert(len == 2);
    cu_list_tfreex(list, int, len, free);
    assert(!list);
    assert(len == 0);

    list = create_list(&len);
    assert(list);
    assert(len == 2);
    cu_list_tfree(list, int, len);
    assert(!list);
    assert(len == 0);

    list = create_list(&len);
    assert(list);
    assert(len == 2);
    cu_list_freex(list, len, free);
    assert(!list);
    assert(len == 0);

    list = create_list(&len);
    assert(list);
    assert(len == 2);
    cu_list_free(list, len);
    assert(!list);
    assert(len == 0);

    return 0;
}

static char** create_list(int* len) {
    *len = 2;
    char** list = calloc(*len, sizeof(char*));
    list[0] = strdup("a");
    list[1] = strdup("b");
    return list;
}