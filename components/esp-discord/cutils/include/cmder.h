#ifndef _CUTILS_CMDER_H_
#define _CUTILS_CMDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include "xlist.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define CMDER_NAME_MAX_LENGTH 50
#define CMDER_CMD_NAME_MAX_LENGTH 70
#define CMDER_DEFAULT_CMDLINE_MAX_LEN 512

#define CU_ERR_CMDER_BASE                (-3000)
#define CU_ERR_CMDER_OPT_EXIST           (CU_ERR_CMDER_BASE - 1)
#define CU_ERR_CMDER_NO_CMDS             (CU_ERR_CMDER_BASE - 3)
#define CU_ERR_CMDER_CMDLINE_TOO_BIG     (CU_ERR_CMDER_BASE - 4)
#define CU_ERR_CMDER_IGNORE              (CU_ERR_CMDER_BASE - 5)
#define CU_ERR_CMDER_CMD_NOEXIST         (CU_ERR_CMDER_BASE - 6)
#define CU_ERR_CMDER_OPT_VAL_MISSING     (CU_ERR_CMDER_BASE - 7)
#define CU_ERR_CMDER_UNKNOWN_OPTION      (CU_ERR_CMDER_BASE - 8)
#define CU_ERR_CMDER_CMD_EXIST           (CU_ERR_CMDER_BASE - 10)
#define CU_ERR_CMDER_NO_OPTVALS          (CU_ERR_CMDER_BASE - 11)
#define CU_ERR_CMDER_INVALID_OPT_NAME    (CU_ERR_CMDER_BASE - 12)
#define CU_ERR_CMDER_NO_OPTS             (CU_ERR_CMDER_BASE - 13)

struct cmder_handle;
struct cmder_cmd_handle;
typedef struct cmder_handle* cmder_handle_t;
typedef struct cmder_cmd_handle* cmder_cmd_handle_t;

typedef struct {
    const char* name;
    bool name_as_cmdline_prefix;
    void* context;
    size_t cmdline_max_len;
} cmder_t;

typedef struct {
    char name;
    bool is_arg;
    bool is_optional;
    char* description;
} cmder_opt_t;

typedef cmder_opt_t* cmder_opt_handle_t;

typedef struct {
    cmder_opt_handle_t opt;
    bool state;
    const char* val;
} cmder_optval_t;

typedef enum {
    CMDER_CMDVAL_NO_ERROR,
    CMDER_CMDVAL_OPTION_VALUE_MISSING,
    CMDER_CMDVAL_UNKNOWN_OPTION
} cmder_cmdval_err_t;

typedef struct {
    cmder_handle_t cmder;
    cmder_cmd_handle_t cmd;
    const void* context;
    const void* run_context;
    xlist_t optvals;
    xlist_t extra_args;
    cmder_cmdval_err_t error;
    char error_option_name;
} cmder_cmdval_t;

typedef void(*cmder_callback_t)(cmder_cmdval_t* cmdval);

typedef struct {
    const char* name;
    cmder_callback_t callback;
} cmder_cmd_t;

cu_err_t cmder_create(cmder_t* config, cmder_handle_t* out_handle);
cu_err_t cmder_getoopts(cmder_cmd_handle_t cmd, char** out_getoops);
cu_err_t cmder_args(const char* cmdline, int* out_argc, char*** out_argv);
cu_err_t cmder_add_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd, cmder_cmd_handle_t* out_cmd);
cu_err_t cmder_add_vcmd(cmder_handle_t cmder, cmder_cmd_t* cmd);
cu_err_t cmder_add_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt, cmder_opt_handle_t* out_opt);
cu_err_t cmder_add_vopt(cmder_cmd_handle_t cmd, cmder_opt_t* opt);
cu_err_t cmder_cmd_signature(cmder_cmd_handle_t cmd, char** out_signature, unsigned int* out_len);
cu_err_t cmder_cmd_manual(cmder_cmd_handle_t cmd, char** out_manual, unsigned int* out_len);
cu_err_t cmder_run_args(cmder_handle_t cmder, int argc, char** argv, const void* run_context);
cu_err_t cmder_vrun_args(cmder_handle_t cmder, int argc, char** argv);
cu_err_t cmder_run(cmder_handle_t cmder, const char* cmdline, const void* run_context);
cu_err_t cmder_vrun(cmder_handle_t cmder, const char* cmdline);
cu_err_t cmder_get_cmd_by_name(cmder_handle_t cmder, const char* cmd_name, cmder_cmd_handle_t* out_cmd_handle);
cu_err_t cmder_get_optval(cmder_cmdval_t* cmdval, char optname, cmder_optval_t** out_optval);
cu_err_t cmder_cmdval_errstr(cmder_cmdval_t* cmdval, char** out_errstr, unsigned int* out_len);
cu_err_t cmder_destroy(cmder_handle_t cmder);

#ifdef __cplusplus
}
#endif

#endif