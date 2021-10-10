#include <stdlib.h>
#include <assert.h>

#include "estr.h"
#include "wxp.h"

int main() {
    char** argv = NULL;
    int argc;
    
    assert(wxp(NULL, &argc, &argv) == CU_ERR_INVALID_ARG);
    assert(wxp("a b", NULL, &argv) == CU_ERR_INVALID_ARG);
    assert(wxp("a b", &argc, NULL) == CU_ERR_INVALID_ARG);
    assert(wxp("", &argc, &argv) == CU_ERR_EMPTY_STRING);

    assert(wxp("test a b c", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b") && estr_eq(argv[3], "c"));
    cu_list_free(argv, argc);

    assert(wxp("a", &argc, &argv) == CU_OK);
    assert(argc == 1);
    assert(argv && estr_eq(argv[0], "a"));
    cu_list_free(argv, argc);
    assert(wxp("ab \"", &argc, &argv) == CU_ERR_SYNTAX_ERROR);

    assert(wxp("\"ab\\\"c\" \"\\\\\" d", &argc, &argv) == CU_OK);
    assert(argc == 3);
    assert(argv && estr_eq(argv[0], "ab\"c") && estr_eq(argv[1], "\\") && 
        estr_eq(argv[2], "d"));
    cu_list_free(argv, argc);

    assert(wxp("a\\\\\\\\b d\"e f\"g h", &argc, &argv) == CU_OK);
    assert(argc == 5);
    assert(argv && estr_eq(argv[0], "a\\\\b") && estr_eq(argv[1], "d") && 
        estr_eq(argv[2], "e f") &&
        estr_eq(argv[3], "g") && 
        estr_eq(argv[4], "h"));
    cu_list_free(argv, argc);

    assert(wxp("a\\\\\\\"b c d", &argc, &argv) == CU_OK);
    assert(argc == 3);
    assert(argv && estr_eq(argv[0], "a\\\"b") && estr_eq(argv[1], "c") && 
        estr_eq(argv[2], "d"));
    cu_list_free(argv, argc);

    assert(wxp("a\"b\"\" c d", &argc, &argv) == CU_ERR_SYNTAX_ERROR);
    assert(wxp("a b\"", &argc, &argv) == CU_ERR_SYNTAX_ERROR);

    assert(wxp("test ab \"c d\" e", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "ab") &&
        estr_eq(argv[2], "c d") && estr_eq(argv[3], "e"));
    cu_list_free(argv, argc);
    assert(wxp("test a \"b c\"    \"\"   d", &argc, &argv) == CU_OK);
    assert(argc == 5);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "")  && estr_eq(argv[4], "d"));
    cu_list_free(argv, argc);
    assert(wxp("  \" a   b c \"  d e \"f\" ", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], " a   b c ") && 
        estr_eq(argv[1], "d") && estr_eq(argv[2], "e") && estr_eq(argv[3], "f"));
    cu_list_free(argv, argc);
    assert(wxp("test a \"b c\"    \"\"   d \"\"", &argc, &argv) == CU_OK);
    assert(argc == 6);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") &&
        estr_eq(argv[3], "") &&
        estr_eq(argv[4], "d") &&
        estr_eq(argv[5], "")
        );
    cu_list_free(argv, argc);
    assert(wxp("test a \"b c\"    \"\"   d \"x\"", &argc, &argv) == CU_OK);
    assert(argc == 6);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") &&
        estr_eq(argv[3], "") &&
        estr_eq(argv[4], "d") && estr_eq(argv[5], "x"));
    cu_list_free(argv, argc);
    assert(wxp("\"\"   test a \"b c\"    \"\"   \"d\"  ", &argc, &argv) == CU_OK);
    assert(argc == 6);
    assert(argv && estr_eq(argv[0], "") && estr_eq(argv[1], "test") && estr_eq(argv[2], "a") && 
        estr_eq(argv[3], "b c") && estr_eq(argv[4], "") && estr_eq(argv[5], "d"));
    cu_list_free(argv, argc);
    assert(wxp("a \"b \\\"c\\\" d\"", &argc, &argv) == CU_OK); // a "b \"c\" d"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "b \"c\" d"));
    cu_list_free(argv, argc);
    assert(wxp("a \\\"c\\\"", &argc, &argv) == CU_OK); // a \"c\"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "\"c\""));
    cu_list_free(argv, argc);
    assert(wxp("\\\"a\\\" b", &argc, &argv) == CU_OK); // \"a\" b
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "\"a\"") && estr_eq(argv[1], "b"));
    cu_list_free(argv, argc);

    assert(wxp("a \"\\\"b\\\"\"", &argc, &argv) == CU_OK);
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "\"b\""));
    cu_list_free(argv, argc);

    return 0;
}