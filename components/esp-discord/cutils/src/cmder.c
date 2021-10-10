#include "cmder.h"
#include "cutils.h"
#include "estr.h"
#include "wxp.h"
#include <getopt.h>
#include <assert.h>

struct cmder_cmd_handle {
	char* name;
    cmder_callback_t callback;
    cmder_handle_t cmder;
    xlist_t opts;
    char* getoopts;
};

struct cmder_handle {
    char* name;
    bool name_as_cmdline_prefix;
    void* context;
    size_t cmdline_max_len;
    xlist_t cmds;
};

typedef enum {
    CMDER_OPT_FLAG,
    CMDER_OPT_OPTIONAL_ARG,
    CMDER_OPT_MANDATORY_ARG
} cmder_opt_type_t;

typedef struct {
    cmder_opt_handle_t* flags;  /*<! Options */
    uint16_t flags_len;         /*<! Options length */
    cmder_opt_handle_t* oargs;  /*<! Optional args */
    uint16_t oargs_len;         /*<! Optional args length */
    cmder_opt_handle_t* margs;  /*<! Mandatory args */
    uint16_t margs_len;         /*<! Mandatory args length */
} cmder_gopts_t;

static void _opt_free(cmder_opt_handle_t opt) {
    if(!opt)
        return;

    free(opt->description);
    free(opt);
}

static void _cmd_free(cmder_cmd_handle_t cmd) {
    if(!cmd)
        return;

    cmd->cmder = NULL;
    cmd->callback = NULL;
    free(cmd->name);
    cmd->name = NULL;
    free(cmd->getoopts);
    cmd->getoopts = NULL;
    xlist_destroy(cmd->opts);
    cmd->opts = NULL;
    free(cmd);
}

static void _optval_free(cmder_optval_t* optval) {
    if(!optval)
        return;
    
    optval->opt = NULL;
    optval->state = false;
    optval->val = NULL; // don't free, it's a reference to argv item
    free(optval);
}

static void _cmdval_free(cmder_cmdval_t* cmdval) {
    if(!cmdval)
        return;
    
    cmdval->cmder = NULL;
    cmdval->context = NULL;
    xlist_destroy(cmdval->optvals);
    cmdval->optvals = NULL;
    xlist_destroy(cmdval->extra_args);
    cmdval->extra_args = NULL;
    free(cmdval);
}

static void _free_gopts(cmder_gopts_t* gopts) {
    if(!gopts) {
        return;
    }

    // don't free individual opts
    free(gopts->flags);
    gopts->flags = NULL;
    gopts->flags_len = 0;
    free(gopts->oargs);
    gopts->oargs = NULL;
    gopts->oargs_len = 0;
    free(gopts->margs);
    gopts->margs = NULL;
    gopts->margs_len = 0;
    free(gopts);
}

static cu_err_t _validate_name(const char* name, unsigned int maxlen) {
    return estr_validate(name, &(estr_validation_t) {
        .length = true,
        .minlen = 1,
        .maxlen = maxlen,
        .no_whitespace = true
    });
}

static void _xlist_cmd_free(void* data) {
    _cmd_free((cmder_cmd_handle_t) data);
}

static void _xlist_opt_free(void* data) {
    _opt_free((cmder_opt_handle_t) data);
}

static void _xlist_optval_free(void* data) {
    _optval_free((cmder_optval_t*) data);
}

cu_err_t cmder_create(cmder_t* config, cmder_handle_t* out_handle) {
    if(!config || !out_handle)
        return CU_ERR_INVALID_ARG;

    cu_err_t err = CU_OK;
    cmder_handle_t cmder = NULL;
    char* _name = NULL;

    bool validate_name = config->name_as_cmdline_prefix || config->name;

    if(validate_name && (err = _validate_name(config->name, CMDER_NAME_MAX_LENGTH)) != CU_OK) {
        return err;
    }

    if(config->name) {
        cu_mem_check(_name = strdup(config->name));
    }
    
    cu_mem_check(cmder = cu_tctor(cmder_handle_t, struct cmder_handle,
        .name = _name,
        .name_as_cmdline_prefix = config->name_as_cmdline_prefix,
        .context = config->context,
        .cmdline_max_len = config->cmdline_max_len > 0 ? config->cmdline_max_len : CMDER_DEFAULT_CMDLINE_MAX_LEN
    ));

    _name = NULL;

    cu_err_check(xlist_create(&(xlist_config_t){ .data_free_handler = &_xlist_cmd_free }, &cmder->cmds));

    goto _return;
_error:
    free(_name);
    cmder_destroy(cmder);
    cmder = NULL;
_return:
    *out_handle = cmder;
    return err;
}

cu_err_t cmder_getoopts(cmder_cmd_handle_t cmd, char** out_getoopts) {
    if(!cmd || !out_getoopts) {
        return CU_ERR_INVALID_ARG;
    }

    if(xlist_is_empty(cmd->opts)) {
        return CU_ERR_CMDER_NO_OPTS;
    }

    uint16_t len = 1;

    xlist_each(cmder_opt_handle_t, cmd->opts, {
        len += xdata->is_arg ? 2 : 1;
    });

    char* getoopts = NULL;
    cu_mem_checkr(getoopts = malloc(len + 1));

    getoopts[0] = ':';
    char* ptr = getoopts + 1;

    xlist_each(cmder_opt_handle_t, cmd->opts, {
        *ptr++ = xdata->name;
        if(xdata->is_arg) *ptr++ = ':';
    });

    getoopts[len] = '\0';
    *out_getoopts = getoopts;

    return CU_OK;
}

cu_err_t cmder_args(const char* cmdline, int* out_argc, char*** out_argv) {
    if(!cmdline || !out_argc || !out_argv) {
        return CU_ERR_INVALID_ARG;
    }

    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    cu_err_t err;
    char** argv = NULL;
    int argc;

    if((err = wxp(cmdline, &argc, &argv)) == CU_OK) {
        *out_argc = argc;
        *out_argv = argv;
    }

    return err;
}

static cu_err_t _getoopts_recalc(cmder_cmd_handle_t cmd) {
    cu_err_t err = CU_OK;
    char* getoopts = NULL;
    cu_err_checkr(cmder_getoopts(cmd, &getoopts));
    free(cmd->getoopts); // free old
    cmd->getoopts = getoopts;

    return err;
}

cu_err_t cmder_get_cmd_by_name(cmder_handle_t cmder, const char* cmd_name, cmder_cmd_handle_t* out_cmd_handle) {
    if(!cmder || !cmd_name) {
        return CU_ERR_INVALID_ARG;
    }

    if(strlen(cmd_name) <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    xlist_each(cmder_cmd_handle_t, cmder->cmds, {
        if(estr_eq(cmd_name, xdata->name)) {
            if(out_cmd_handle) {
                *out_cmd_handle = xdata;
            }
            
            return CU_OK;
        }
    });

    return CU_ERR_NOT_FOUND;
}

cu_err_t cmder_add_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd, cmder_cmd_handle_t* out_cmd) {
    if(!cmder || !cmd || !cmd->callback || !cmd->name)
        return CU_ERR_INVALID_ARG;

    cu_err_t err = CU_OK;
    cmder_cmd_handle_t _cmd = NULL;
    char* _name = NULL;

    if((err = _validate_name(cmd->name, CMDER_CMD_NAME_MAX_LENGTH)) != CU_OK) {
        return err;
    }
    
    if(cmder_get_cmd_by_name(cmder, cmd->name, NULL) == CU_OK) // already exist
        return CU_ERR_CMDER_CMD_EXIST;
    
    cu_mem_check(_name = strdup(cmd->name));

    cu_mem_check(_cmd = cu_tctor(cmder_cmd_handle_t, struct cmder_cmd_handle,
        .cmder = cmder,
        .name = _name,
        .callback = cmd->callback
    ));

    _name = NULL;

    cu_err_check(xlist_create(&(xlist_config_t){ .data_free_handler = &_xlist_opt_free }, &_cmd->opts));
    cu_err_negative_check(xlist_vadd(cmder->cmds, _cmd));

    goto _return;
_error:
    free(_name);
    _cmd_free(_cmd);
    _cmd = NULL;
_return:
    if(out_cmd) { *out_cmd = _cmd; }

    return err;
}

cu_err_t cmder_add_vcmd(cmder_handle_t cmder, cmder_cmd_t* cmd) {
    return cmder_add_cmd(cmder, cmd, NULL);
}

static cmder_opt_handle_t _get_opt_by_name(cmder_cmd_handle_t cmd, char name) {
    if(!cmd) {
        return NULL;
    }

    xlist_each(cmder_opt_handle_t, cmd->opts, {
        if(xdata->name == name) {
            return xdata;
        }
    });

    return NULL;
}

cu_err_t cmder_add_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt, cmder_opt_handle_t* out_opt) {
    if(!cmd || !cmd->cmder || !opt)
        return CU_ERR_INVALID_ARG;

    cu_err_t err = CU_OK;
    cmder_opt_handle_t _opt = NULL;
    char* _desc = NULL;

    if(!estr_is_alnum(opt->name)) {
        return CU_ERR_CMDER_INVALID_OPT_NAME;
    }

    if(_get_opt_by_name(cmd, opt->name)) // already exist
        return CU_ERR_CMDER_OPT_EXIST;
    
    if(opt->description) {
        cu_mem_check(_desc = strdup(opt->description));
    }

    cu_mem_check(_opt = cu_tctor(cmder_opt_handle_t, cmder_opt_t,
        .name = opt->name,
        .is_arg = opt->is_arg,
        .is_optional = opt->is_optional,
        .description = _desc
    ));

    _desc = NULL;

    cu_err_negative_check(xlist_vadd(cmd->opts, _opt));
    cu_err_check(_getoopts_recalc(cmd));

    goto _return;
_error:
    free(_desc);
    _opt_free(_opt);
    _opt = NULL;
_return:
    if(out_opt) { *out_opt = _opt; }

    return err;
}

cu_err_t cmder_add_vopt(cmder_cmd_handle_t cmd, cmder_opt_t* opt) {
    return cmder_add_opt(cmd, opt, NULL);
}

/**
 * @brief Calculate how much is there options with specific type.
 *        Set param "out_opts" to NULL if you want just to calculate
 *        without memory allocation for options array
 * 
 * @param cmd command
 * @param type option type
 * @param out_opts options array (if NULL - no memory will be alocated. just count mode)
 * @param out_len options array length
 * 
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
static cu_err_t _cmd_options(cmder_cmd_handle_t cmd, cmder_opt_type_t type, cmder_opt_handle_t** out_opts, uint16_t* out_len) {
    if(!cmd) {
        return CU_ERR_INVALID_ARG;
    }

    uint16_t len = 0;

    xlist_each(cmder_opt_handle_t, cmd->opts, {
        if(xdata->is_arg) {
            if(xdata->is_optional && type == CMDER_OPT_OPTIONAL_ARG) {
                len++;
            } else if(!xdata->is_optional && type == CMDER_OPT_MANDATORY_ARG) {
                len++;
            }
        } else if(type == CMDER_OPT_FLAG) {
            len++;
        }
    });

    if(len == 0 || !out_opts) {
        if(out_opts) { *out_opts = NULL; }
        if(out_len) { *out_len = len; }
        
        return CU_OK;
    }

    cmder_opt_handle_t* opts = NULL;
    cu_mem_checkr(opts = malloc(len * sizeof(cmder_opt_handle_t)));

    uint16_t j = 0;
    xlist_each(cmder_opt_handle_t, cmd->opts, {
        if(xdata->is_arg) {
            if(xdata->is_optional && type == CMDER_OPT_OPTIONAL_ARG) {
                opts[j++] = xdata;
            } else if(!xdata->is_optional && type == CMDER_OPT_MANDATORY_ARG) {
                opts[j++] = xdata;
            }
        } else if(type == CMDER_OPT_FLAG) {
            opts[j++] = xdata;
        }
    });

    *out_opts = opts;

    if(out_len) {
        *out_len = len;
    }

    return CU_OK;
}

static cu_err_t _cmd_gopts(cmder_cmd_handle_t cmd, cmder_gopts_t** out_gopts, bool just_lengths) {
    if(!cmd || !out_gopts) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    cmder_gopts_t* gopts = NULL;
    cu_mem_check(gopts = cu_ctor(cmder_gopts_t));

    cu_err_check(_cmd_options(cmd, CMDER_OPT_FLAG, just_lengths ? NULL : &gopts->flags, &gopts->flags_len));
    cu_err_check(_cmd_options(cmd, CMDER_OPT_OPTIONAL_ARG, just_lengths ? NULL : &gopts->oargs, &gopts->oargs_len));
    cu_err_check(_cmd_options(cmd, CMDER_OPT_MANDATORY_ARG, just_lengths ? NULL : &gopts->margs, &gopts->margs_len));
    
    goto _return;
_error:
    _free_gopts(gopts);
    gopts = NULL;
_return:
    *out_gopts = gopts;
    return err;
}

/**
 * @param cmd command
 * @param out_len signature length
 * @param o_gopts options groups
 * @param gopts_just_lengths true - just lengths; false - lengths and opts
 * 
 * @return CU_OK on success
 */
static cu_err_t _calc_signature_len(cmder_cmd_handle_t cmd, unsigned int* out_len, cmder_gopts_t** o_gopts, bool gopts_just_lengths) {
    if(!cmd || !out_len) {
        return CU_ERR_INVALID_ARG;
    }

    *out_len = 0;

    cu_err_t err = CU_OK;
    unsigned int name_len = strlen(cmd->name);

    if(name_len == 0) {
        return CU_ERR_INVALID_ARG;
    }

    unsigned int len = name_len;
    cmder_gopts_t* gopts = NULL;

    cu_err_check(_cmd_gopts(cmd, &gopts, gopts_just_lengths));

    len++; // space

    if(gopts->flags_len > 0) {
        len += 9; // "[OPTION] "

        if(gopts->flags_len > 1) {
            len += 4; // "... "
        }
    }

    for(uint16_t i = 0; i < gopts->oargs_len; i++) {
        len += 10; // "[-x xval] "
    }

    for(uint16_t i = 0; i < gopts->margs_len; i++) {
        len += 8; // "-x xval "
    }

    len--; // last space

    goto _return;
_error:
    _free_gopts(gopts);
    gopts = NULL;
_return:
    *out_len = len;
    if(o_gopts)     { *o_gopts = gopts; } else { _free_gopts(gopts); }
    return err;
}

/**
 * @param cmd command
 * @param out_signature signature (if NULL pointer - new memory will be allocated, 
 *                      otherwise outside buffer will be used)
 * @param len signature length
 * @param gopts options groups (make sure to provide opts in gopts)
 * 
 * @return CU_OK on success
 */
static cu_err_t _create_signature(cmder_cmd_handle_t cmd, char** out_signature, unsigned int len, cmder_gopts_t* gopts) {
    if(!cmd || !out_signature || len == 0 || !gopts) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    char* signature = NULL;
    char* ptr = NULL;

    // if outside pointer is not null, that's consider like buffer has been provided
    // and that buffer will be used instead of allocating new memory
    cu_mem_check(signature = *out_signature ? *out_signature : malloc(len + 1));
    ptr = signature;

    unsigned int name_len = strlen(cmd->name);
    memcpy(ptr, cmd->name, name_len);
    ptr += name_len;
    *ptr++ = ' ';

    if(gopts->flags_len > 0) {
        memcpy(ptr, "[OPTION] ", 9);
        ptr += 9;

        if(gopts->flags_len > 1) {
            memcpy(ptr, "... ", 4);
            ptr += 4;
        }
    }

    for(uint16_t i = 0; i < gopts->oargs_len; i++) {
        *ptr++ = '[';
        *ptr++ = '-';
        *ptr++ = gopts->oargs[i]->name;
        *ptr++ = ' ';
        *ptr++ = gopts->oargs[i]->name;
        memcpy(ptr, "val] ", 5);
        ptr += 5;
    }

    for(uint16_t i = 0; i < gopts->margs_len; i++) {
        *ptr++ = '-';
        *ptr++ = gopts->margs[i]->name;
        *ptr++ = ' ' ;
        *ptr++ = gopts->margs[i]->name;
        memcpy(ptr, "val ", 4);
        ptr += 4;
    }

    *--ptr = '\0'; // replace last space

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - signature == (int) len);

    goto _return;
_error:
    free(signature);
    signature = NULL;
    len = 0;
_return:
    *out_signature = signature;
    return err;
}

cu_err_t cmder_cmd_signature(cmder_cmd_handle_t cmd, char** out_signature, unsigned int* out_len) {
    if(!cmd || !out_signature) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err;
    char* signature = NULL;
    unsigned int len = 0;
    cmder_gopts_t* gopts = NULL;
    cu_err_check(_calc_signature_len(cmd, &len, &gopts, false));

    signature = NULL; // new memory will be allocated
    cu_err_check(_create_signature(cmd, &signature, len, gopts));

    goto _return;
_error:
    free(signature);
    signature = NULL;
    len = 0;
_return:
    _free_gopts(gopts);
    if(out_len) { *out_len = len; }
    *out_signature = signature;
    return err;
}

static cu_err_t _calc_manual_len(cmder_cmd_handle_t cmd, unsigned int* out_len, unsigned int* out_sig_len, cmder_gopts_t** o_gopts) {
    if(!cmd) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    unsigned int len = 0, sig_len = 0;
    cmder_gopts_t* gopts = NULL;

    cu_err_check(_calc_signature_len(cmd, &sig_len, &gopts, false));

    len += 7; // "Usage: "
    len += sig_len;
    len ++; // "\n"

    if(gopts->margs_len > 0) {
        len += 16; // "Mandatory args:[NL]"
        for(uint16_t i = 0; i < gopts->margs_len; i++) {
            len += 3; // "[TAB]-x"
            if(gopts->margs[i]->description) {
                len++; // [SPACE]
                len += strlen(gopts->margs[i]->description);
            }
            len++; // "[NL]"
        }
    }

    if(gopts->oargs_len > 0) {
        len += 15; // "Optional args:[NL]"
        for(uint16_t i = 0; i < gopts->oargs_len; i++) {
            len += 3; // "[TAB]-x"
            if(gopts->oargs[i]->description) {
                len++; // [SPACE]
                len += strlen(gopts->oargs[i]->description);
            }
            len++; // "[NL]"
        }
    }

    if(gopts->flags_len > 0) {
        len += 9; // "Options:[NL]"
        for(uint16_t i = 0; i < gopts->flags_len; i++) {
            len += 3; // "[TAB]-x"
            if(gopts->flags[i]->description) {
                len++; // [SPACE]
                len += strlen(gopts->flags[i]->description);
            }
            len++; // "[NL]"
        }
    }

    if(len > 0) {
        len--; // remove last [NL]
    }

    goto _return;
_error:
    _free_gopts(gopts);
    len = 0;
_return:
    if(out_len) { *out_len = len; }
    if(out_sig_len) { *out_sig_len = sig_len; }
    if(o_gopts) { *o_gopts = gopts; } else { _free_gopts(gopts); }
    return err;
}

static cu_err_t _create_manual(cmder_cmd_handle_t cmd, char** out_manual, size_t len, size_t sig_len, cmder_gopts_t* gopts) {
    if(!cmd || !out_manual || !gopts) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    char* manual = NULL;
    char* ptr = NULL;

    cu_mem_check(manual = malloc(len + 1)); // +1 for "\0"

    ptr = manual;
    memcpy(ptr, "Usage: ", 7);
    ptr += 7;
    cu_err_check(_create_signature(cmd, &ptr, sig_len, gopts));

    ptr += sig_len;
    *ptr++ = '\n';

    if(gopts->margs_len > 0) {
        memcpy(ptr, "Mandatory args:\n", 16);
        ptr += 16;

        for(uint16_t i = 0; i < gopts->margs_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = gopts->margs[i]->name;
            if(gopts->margs[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(gopts->margs[i]->description);
                memcpy(ptr, gopts->margs[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }
    }

    if(gopts->oargs_len > 0) {
        memcpy(ptr, "Optional args:\n", 15);
        ptr += 15;

        for(uint16_t i = 0; i < gopts->oargs_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = gopts->oargs[i]->name;
            if(gopts->oargs[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(gopts->oargs[i]->description);
                memcpy(ptr, gopts->oargs[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }
    }

    if(gopts->flags_len > 0) {
        memcpy(ptr, "Options:\n", 9);
        ptr += 9;

        for(uint16_t i = 0; i < gopts->flags_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = gopts->flags[i]->name;
            if(gopts->flags[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(gopts->flags[i]->description);
                memcpy(ptr, gopts->flags[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }
    }

    *--ptr = '\0'; // replace last [NL]

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - manual == (int) len);

    goto _return;
_error:
    free(manual);
    manual = NULL;
    len = 0;
_return:
    *out_manual = manual;
    return err;
}

cu_err_t cmder_cmd_manual(cmder_cmd_handle_t cmd, char** out_manual, unsigned int* out_len) {
    if(!cmd || !out_manual) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err;
    unsigned int len = 0, sig_len = 0;
    cmder_gopts_t* gopts = NULL;
    char* manual = NULL;

    cu_err_check(_calc_manual_len(cmd, &len, &sig_len, &gopts));
    cu_err_check(_create_manual(cmd, &manual, len, sig_len, gopts));
    
    goto _return;
_error:
    free(manual);
    manual = NULL;
    len = 0;
_return:
    _free_gopts(gopts);
    if(out_len) { *out_len = len; }
    *out_manual = manual;
    return err;
}

cu_err_t cmder_get_optval(cmder_cmdval_t* cmdval, char optname, cmder_optval_t** out_optval) {
    if(!cmdval) {
        return CU_ERR_INVALID_ARG;
    }

    if(!cmdval->optvals || xlist_is_empty(cmdval->optvals)) {
        return CU_ERR_CMDER_NO_OPTVALS;
    }

    xlist_each(cmder_optval_t*, cmdval->optvals, {
        if(xdata->opt->name == optname) {
            if(out_optval) {
                *out_optval = xdata;
            }

            return CU_OK;
        }
    });

    return CU_ERR_NOT_FOUND;
}

cu_err_t cmder_cmdval_errstr(cmder_cmdval_t* cmdval, char** out_errstr, unsigned int* out_len) {
    if(!cmdval || !out_errstr || cmdval->error == CMDER_CMDVAL_NO_ERROR || !cmdval->cmd) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    char* errorstr = NULL;
    unsigned int cmdname_len = strlen(cmdval->cmd->name);
    unsigned int len = cmdname_len;
    len += 2; // ": "

    switch (cmdval->error)
    {
        case CMDER_CMDVAL_UNKNOWN_OPTION:
            len += 7; // "invalid"
            break;
        
        case CMDER_CMDVAL_OPTION_VALUE_MISSING:
            len += 17; // "missing value for"
            break;

        case CMDER_CMDVAL_NO_ERROR:
            // ignore
            break;
    }

    len += 14; // " option -- 'x'"

    cu_mem_check(errorstr = malloc(len + 1));

    char* ptr = errorstr;
    memcpy(ptr, cmdval->cmd->name, cmdname_len);
    ptr += cmdname_len;
    *ptr++ = ':';
    *ptr++ = ' ';

    switch (cmdval->error)
    {
        case CMDER_CMDVAL_UNKNOWN_OPTION:
            memcpy(ptr, "invalid", 7);
            ptr += 7;
            break;
        
        case CMDER_CMDVAL_OPTION_VALUE_MISSING:
            memcpy(ptr, "missing value for", 17);
            ptr += 17;
            break;

        case CMDER_CMDVAL_NO_ERROR:
            // ignore
            break;
    }

    memcpy(ptr, " option -- '", 12);
    ptr += 12;
    *ptr++ = cmdval->error_option_name;
    *ptr++ = '\'';
    *ptr = '\0';

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - errorstr == (int) len);

    goto _return;
_error:
    free(errorstr);
    errorstr = NULL;
    len = 0;
_return:
    *out_errstr = errorstr;
    if(out_len) { *out_len = len; }

    return err;
}

static cmder_opt_handle_t _first_mandatory_opt_which_is_not_set(cmder_cmdval_t* cmdval) {
    if(!cmdval || !cmdval->optvals || xlist_is_empty(cmdval->optvals)) {
        return NULL;
    }

    xlist_each(cmder_optval_t*, cmdval->optvals, {
        if(xdata->opt->is_arg && !xdata->opt->is_optional && !xdata->val) {
            return xdata->opt;
        }
    });

    return NULL;
}

static cu_err_t _argv_is_in_good_condition(int argc, char** argv, bool* condition) {
    if(argc <= 0 || !argv || !condition) {
        return CU_ERR_INVALID_ARG;
    }

    *condition = true;

    for(int i = 0; i < argc; i++) {
        if(!estr_is_trimmed(argv[i]) || estr_contains_unescaped_chr(argv[i], '\"')) {
            *condition = false;
            return CU_ERR_SYNTAX_ERROR;
        }
    }

    return CU_OK;
}

static cu_err_t _cmder_run_args(cmder_handle_t cmder, int argc, char** argv, const void* run_context, bool safe) {
    if(!cmder) {
        return CU_ERR_INVALID_ARG;
    }
    
    if(!argv || argc <= 0) { // nothing to do
        return CU_ERR_CMDER_IGNORE;
    }

    cu_err_t err = CU_OK;
    bool argv_condition;

    if(safe && ((err = _argv_is_in_good_condition(argc, argv, &argv_condition)) != CU_OK || !argv_condition)) {
        return err;
    }

    cmder_cmd_handle_t cmd = NULL;

    if(cmder_get_cmd_by_name(cmder, argv[0], &cmd) != CU_OK) { // argv[0] is cmd name
        return CU_ERR_CMDER_CMD_NOEXIST;
    }
    
    cmder_cmdval_t* cmdval = NULL;

    cu_mem_check(cmdval = cu_ctor(cmder_cmdval_t,
        .cmder = cmder,
        .cmd = cmd,
        .context = cmder->context,
        .run_context = run_context,
        .error = CMDER_CMDVAL_NO_ERROR
    ));

    if(! xlist_is_empty(cmd->opts)) {
        // making one optval in cmdval for every opt in cmd

        cu_err_check(xlist_create(&(xlist_config_t){
            .data_free_handler = &_xlist_optval_free
        }, &cmdval->optvals));

        xlist_each(cmder_opt_handle_t, cmd->opts, {
            cu_err_negative_check(xlist_vadd(cmdval->optvals, cu_ctor(cmder_optval_t,
                .opt = xdata
            )));
        });
    }

    opterr = 0;
    optind = 0;

    cmder_optval_t* optval = NULL;
    int o;
    while((o = getopt(argc, argv, (!cmd->getoopts ? "" : cmd->getoopts))) != -1) {
        if(o == ':') {
            cu_err_checkl(cmder_get_optval(cmdval, optopt, &optval), _return);
            
            if(!optval->opt->is_optional) {
                err = CU_ERR_CMDER_OPT_VAL_MISSING;
                cmdval->error = CMDER_CMDVAL_OPTION_VALUE_MISSING;
                cmdval->error_option_name = optopt;
                goto _error;
            }
        } else if(o == '?') {
            err = CU_ERR_CMDER_UNKNOWN_OPTION;
            cmdval->error = CMDER_CMDVAL_UNKNOWN_OPTION;
            cmdval->error_option_name = optopt;
            goto _error;
        } else {
            cu_err_checkl(cmder_get_optval(cmdval, o, &optval), _return);

            if(optval->opt->is_arg) {
                optval->val = optarg;
            } else {
                optval->state = true;
            }
        }
    }

    cmder_opt_handle_t notset_opt = _first_mandatory_opt_which_is_not_set(cmdval);

    if(notset_opt) {
        err = CU_ERR_CMDER_OPT_VAL_MISSING;
        cmdval->error = CMDER_CMDVAL_OPTION_VALUE_MISSING;
        cmdval->error_option_name = notset_opt->name;
        goto _error;
    }
    
    if(argc - optind > 0) {
        cu_err_check(xlist_create(NULL, &cmdval->extra_args));

        for(int i = optind; i < argc; i++) {
            cu_err_negative_check(xlist_vadd(cmdval->extra_args, argv[i]));
        }
    }

    cmd->callback(cmdval);
    goto _return;
_error:
    xlist_destroy(cmdval->optvals);
    cmdval->optvals = NULL;
    xlist_destroy(cmdval->extra_args);
    cmdval->extra_args = NULL;
    cmd->callback(cmdval);
_return:
    _cmdval_free(cmdval);
    return err;
}

cu_err_t cmder_run_args(cmder_handle_t cmder, int argc, char** argv, const void* run_context) {
    return _cmder_run_args(cmder, argc, argv, run_context, true);
}

cu_err_t cmder_vrun_args(cmder_handle_t cmder, int argc, char** argv) {
    return cmder_run_args(cmder, argc, argv, NULL);
}

cu_err_t cmder_run(cmder_handle_t cmder, const char* cmdline, const void* run_context) {
    if(!cmder || !cmdline)
        return CU_ERR_INVALID_ARG;

    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    if(xlist_is_empty(cmder->cmds)) // no registered cmds
        return CU_ERR_CMDER_NO_CMDS;

    if(cmdline_len > cmder->cmdline_max_len)
        return CU_ERR_CMDER_CMDLINE_TOO_BIG;
    
    size_t cmder_name_len = strlen(cmder->name);

    if(cmder->name_as_cmdline_prefix && !estrn_eq(cmdline, cmder->name, cmder_name_len)) // not for us
        return CU_ERR_CMDER_IGNORE;

    int argc;
    char** argv = NULL;

    cu_err_t err = cmder_args(
        cmder->name_as_cmdline_prefix ? cmdline + cmder_name_len + 1 : cmdline,
        &argc,
        &argv
    );

    if(err != CU_OK) {
        return err;
    }

    err = _cmder_run_args(cmder, argc, argv, run_context, false); // not-safe call (cmder_args is safe)
    cu_list_free(argv, argc);

    return err;
}

cu_err_t cmder_vrun(cmder_handle_t cmder, const char* cmdline) {
    return cmder_run(cmder, cmdline, NULL);
}

cu_err_t cmder_destroy(cmder_handle_t cmder) {
    if(!cmder) {
        return CU_ERR_INVALID_ARG;
    }

    xlist_destroy(cmder->cmds);
    cmder->cmds = NULL;
    free(cmder->name);
    cmder->name = NULL;
    cmder->context = NULL;
    free(cmder);

    return CU_OK;
}