#include "estr.h"
#include "wxp.h"

#define __ESC_QT  "\\\""
#define __NESC_QT "\""
#define __ESC_BS  "\\\\"
#define __NESC_BS "\\"

static cu_err_t _capture(int* argc, char*** argv, char** rec, char* ptr) {
    cu_err_t err = CU_OK;
    char** _argv = NULL;
    int _argc = *argc;
    char* wrd = NULL;
    char* _wrd = NULL;
    cu_mem_check(_argv = realloc(*argv, ++_argc * sizeof(char*)));
    cu_mem_check(wrd = strndup(*rec, ptr - *rec));
    *rec = NULL;

    if(strstr(wrd, __ESC_QT)) {
        cu_mem_check(_wrd = estr_rep(wrd, __ESC_QT, __NESC_QT));
        free(wrd);
        wrd = _wrd;
        _wrd = NULL;
    }

    if(strstr(wrd, __ESC_BS)) {
        cu_mem_check(_wrd = estr_rep(wrd, __ESC_BS, __NESC_BS));
        free(wrd);
        wrd = _wrd;
        _wrd = NULL;
    }

    _argv[_argc - 1] = wrd;
    goto _return;
_error:
    free(wrd);
    if(_argc > *argc && _argv) {
        _argv = realloc(*argv, (_argc = *argc) * sizeof(char*)); // rollback
        _argv = *argv;
    }
_return:
    *argc = _argc;
    *argv = _argv;
    return err;
}

cu_err_t wxp(const char* words, int* argc, char*** argv) {
    if(! words || ! argc || ! argv) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    size_t words_len = strlen(words);

    if(words_len == 0) {
        return CU_ERR_EMPTY_STRING;
    }

    int _argc = 0;
    char** _argv = NULL;

    char* ptr = (char*) words, * prev = NULL, * next = NULL, * rec = NULL;
    bool qt = false, qt_esc = false, bs_esc = false;

    while(*ptr) {
        next = ptr + 1;

        switch (*ptr) {
            case '\\':
                bs_esc = ! bs_esc && prev && *prev == '\\';
                break;

            case '"':
                qt_esc = prev && *prev == '\\' && ! bs_esc;
                if(qt_esc) {
                    if(! rec) { rec = prev; }
                    break;
                }
                qt = ! qt;

                if(qt) {
                    if(rec && ptr != rec) {
                        cu_err_check(_capture(&_argc, &_argv, &rec, ptr));
                    }

                    rec = next;
                }
                else {
                    cu_err_check(_capture(&_argc, &_argv, &rec, ptr));
                }
                break;

            case ' ':
                if(! rec || qt) { break; }
                cu_err_check(_capture(&_argc, &_argv, &rec, ptr));
                break;
            
            default:
                if(! rec) { rec = ptr; }
                break;
        }

        prev = ptr++;
    }

    if(rec) {
        cu_err_check(_capture(&_argc, &_argv, &rec, ptr));
    }

    if(qt) { // last quote not closed
        err = CU_ERR_SYNTAX_ERROR;
        goto _error;
    }

    goto _return;
_error:
    cu_list_free(_argv, _argc);
_return:
    *argc = _argc;
    *argv = _argv;
    return err;
}