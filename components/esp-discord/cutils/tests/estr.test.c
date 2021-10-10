#include "estr.h"
#include "cutils.h"
#include <assert.h>

static void test_estr_eq() {
    assert( estr_eq("a", "a"));
    assert( estr_eq(" ", " "));
    assert( estr_eq("", ""));
    assert(!estr_eq(" ", ""));
    assert(!estr_eq(NULL, "b"));
    assert(!estr_eq("a", NULL));
    assert(!estr_eq(NULL, NULL));
}

static void test_estrn_eq() {
    assert( estrn_eq("a", "a", 1));
    assert( estrn_eq("a", "ab", 1));
    assert( estrn_eq(" ", " ",1));
    assert( estrn_eq("", "", 0));
    assert( estrn_eq("", "", 1));
    assert(!estrn_eq(" ", "", 1));
    assert(!estrn_eq(NULL, "b", 1));
    assert(!estrn_eq("a", NULL, 1));
    assert(!estrn_eq(NULL, NULL, 0));
}

static void test_estr_sw() {
    assert( estr_sw("+react", "+react"));
    assert( estr_sw("+react to this", "+react"));
    assert(!estr_sw("+react", "+react to this"));
    assert(!estr_sw(NULL, "+react"));
    assert(!estr_sw("+react", NULL));
    assert(!estr_sw(NULL, NULL));
    assert(!estr_sw("", "+react"));
    assert(!estr_sw("+react", ""));
    assert(!estr_sw("", ""));
    assert(!estr_sw("+react", "   "));
    assert(!estr_sw("  ", "+react"));

    assert(!estr_sw_chr(NULL, 'a'));
    assert( estr_sw_chr("ab", 'a'));
    assert(!estr_sw_chr("ab", 'b'));
}

static void test_estr_ew() {
    assert( estr_ew("+react", "+react"));
    assert( estr_ew("+react", "ct"));
    assert( estr_ew("a", "a"));
    assert( estr_ew("abcd", "cd"));
    assert(!estr_ew("+react", "this to +react"));
    assert(!estr_ew(NULL, "+react"));
    assert(!estr_ew("+react", NULL));
    assert(!estr_ew(NULL, NULL));
    assert(!estr_ew("", "+react"));
    assert(!estr_ew("+react", ""));
    assert(!estr_ew("", ""));
    assert(!estr_ew("+react", "   "));
    assert(!estr_ew("  ", "+react"));

    assert(!estr_ew_chr(NULL, 'a'));
    assert( estr_ew_chr("ab", 'b'));
    assert(!estr_ew_chr("ab", 'a'));
}

static void test_estrn_is_digit_only() {
    assert( estrn_is_digit_only("123", 3));
    assert( estrn_is_digit_only("", 0));
    assert( estrn_is_digit_only("1", 1));
    assert( estrn_is_digit_only("123", 2));
    assert( estrn_is_digit_only("123", 5));
    assert(!estrn_is_digit_only("a", 1));
    assert( estrn_is_digit_only("23ab", 2));
    assert(!estrn_is_digit_only("abcs", 2));
    assert( estrn_is_digit_only("", 3));
}

static void test_estrn_chrcnt() {
    assert(estrn_chrcnt(".34.f.s.", '.', 8) == 4);
    assert(estrn_chrcnt(".34.f.s.", '.', 3) == 1);
    assert(estrn_chrcnt(".34.f.s.", '.', 5) == 2);
}

static void test_estr_split() {
    char** _pcs;
    size_t _len;

    _pcs = estr_split("a,b,,c", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 1 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "b") && estr_eq(_pcs[2], "c")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split(",,d", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estr_eq(_pcs[0], "d"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("a,,bc,d,,", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "bc") && estr_eq(_pcs[2], "d")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("da", ',', &_len);
    assert(_len == 1 && estr_eq(_pcs[0], "da"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("d,", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estr_eq(_pcs[0], "d"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("jan.feb.mar", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estr_eq(_pcs[0], "jan") && estr_eq(_pcs[1], "feb") && estr_eq(_pcs[2], "mar")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("..jan..feb..mar..", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estr_eq(_pcs[0], "jan") && estr_eq(_pcs[1], "feb") && estr_eq(_pcs[2], "mar")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("......", '.', &_len);
    assert(_len == 0 && !_pcs);
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("     a     bc  d    ", ' ', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "bc") && estr_eq(_pcs[2], "d")
    );
    cu_list_tfree(_pcs, size_t, _len);
}

static void test_estr_cat() {
    char* res;
    
    res = estr_cat("a", "b", "c");
    assert(estr_eq(res, "abc"));
    free(res);

    res = estr_cat("", "b", " ");
    assert(estr_eq(res, "b "));
    free(res);

    res = estr_cat("a", "b", NULL, "c"); // c will be ignored
    assert(estr_eq(res, "ab"));
    free(res);
}

static void test_estr_url_encode() {
    char* res;

    res = estr_url_encode("👍");
    assert(estr_eq(res, "%f0%9f%91%8d"));
    free(res);
}

static void test_estr_rep() {
    char* tmp;
    tmp = estr_rep("aaabbbccc", "bbb", "ddd");
    assert(estr_eq(tmp, "aaadddccc"));
    free(tmp);
    tmp = estr_rep("aaa\\\"x\\\"d", "\\\"", "\""); // aaa\"x\"d -> aaa"x"d
    assert(estr_eq(tmp, "aaa\"x\"d"));
    free(tmp);
    tmp = estr_rep("abc", "d", "x");
    assert(estr_eq(tmp, "abc"));
    free(tmp);
    tmp = estr_rep(NULL, "b", "x");
    assert(!tmp);
    tmp = estr_rep("abc", NULL, "x");
    assert(!tmp);
    tmp = estr_rep("abc", "b", NULL);
    assert(!tmp);
}

static void test_estr_trim() {
    assert(estr_is_trimmed("ddd"));
    assert(estr_is_trimmed(""));
    assert(!estr_is_trimmed(" ddd"));
    assert(!estr_is_trimmed("ddd "));
    assert(!estr_is_trimmed(" ddd "));
    assert(!estr_is_trimmed("ddd    "));
    assert(!estr_is_trimmed("   ddd"));
    assert(!estr_is_trimmed("   ddd "));
}

static void test_escaping() {
    assert(!estr_contains_unescaped_chr(NULL, '\"'));
    assert(!estr_contains_unescaped_chr("abc", '\"'));
    assert(!estr_contains_unescaped_chr("\\\"", '\"'));
    assert(estr_contains_unescaped_chr("asfd\"test\"", '\"'));
}

static void test_empty() {
    assert(estr_is_empty_ws(NULL));
    assert(estr_is_empty_ws(""));
    assert(estr_is_empty_ws(" "));
    assert(! estr_is_empty_ws("  d   "));
    assert(estr_is_empty_ws("                    "));
    assert(! estr_is_empty_ws("    d"));
    assert(! estr_is_empty_ws("g    "));
}

static void test_repeat() {
    char* str = NULL;
    assert(! estr_repeat_chr('a', 0));
    assert((str = estr_repeat_chr('x', 5)));
    assert(estr_eq(str, "xxxxx"));
    free(str);
}

static void test_ws_contains() {
    assert(! estr_contains_ws("asdf"));
    assert(estr_contains_ws("as df"));
    assert(estr_contains_ws("asd    f"));
}

static void test_validation() {
    assert(estr_validate(NULL, NULL) == CU_ERR_INVALID_ARG);
    assert(estr_validate("", NULL) == CU_ERR_INVALID_ARG);
    assert(estr_validate(NULL, &(estr_validation_t){}) == CU_ERR_INVALID_ARG);
    assert(estr_validate("", &(estr_validation_t){
    }) == CU_OK);
    assert(estr_validate("", &(estr_validation_t){
        .length = true
    }) == CU_OK);
    assert(estr_validate("", &(estr_validation_t){
        .length = true,
        .minlen = 1
    }) == CU_ERR_ESTR_INVALID_OUT_OF_BOUNDS);
    assert(estr_validate("a", &(estr_validation_t){
        .length = true,
        .minlen = 1
    }) == CU_OK);
    assert(estr_validate("aa", &(estr_validation_t){
        .length = true,
        .minlen = 1
    }) == CU_ERR_ESTR_INVALID_OUT_OF_BOUNDS); // maxlen is same as minlen
    assert(estr_validate("a", &(estr_validation_t){
        .length = true,
        .minlen = 1,
        .maxlen = 3
    }) == CU_OK);
    assert(estr_validate("aa", &(estr_validation_t){
        .length = true,
        .minlen = 1,
        .maxlen = 3
    }) == CU_OK);
    assert(estr_validate("aaa", &(estr_validation_t){
        .length = true,
        .minlen = 1,
        .maxlen = 3
    }) == CU_OK);
    assert(estr_validate("aaaa", &(estr_validation_t){
        .length = true,
        .minlen = 1,
        .maxlen = 3
    }) == CU_ERR_ESTR_INVALID_OUT_OF_BOUNDS);
    assert(estr_validate("", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_OK);
    assert(estr_validate("a", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_OK);
    assert(estr_validate("abc", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_OK);
    assert(estr_validate("ab c", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
    assert(estr_validate(" abc", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
    assert(estr_validate("abc ", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
    assert(estr_validate("a bc", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
    assert(estr_validate("ab    c", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
    assert(estr_validate("  ", &(estr_validation_t){
        .no_whitespace = true
    }) == CU_ERR_ESTR_INVALID_WHITESPACE);
}

int main() {
    test_estr_eq();
    test_estrn_eq();
    test_estr_sw();
    test_estr_ew();
    test_estrn_is_digit_only();
    test_estrn_chrcnt();
    test_estr_split();
    test_estr_cat();
    test_estr_url_encode();
    test_estr_rep();
    test_estr_trim();
    test_escaping();
    test_empty();
    test_repeat();
    test_ws_contains();
    test_validation();

    return 0;
}