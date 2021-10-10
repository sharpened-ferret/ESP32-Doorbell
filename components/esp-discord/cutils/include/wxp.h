#ifndef _CUTILS_WXP_H_
#define _CUTILS_WXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"

/**
 * @brief String expander
 * @param words String that contains words
 * @param argc Length of words array reference
 * @param argv Words array reference
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_EMPTY_STRING;
 *         CU_ERR_SYNTAX_ERROR;
 *         CU_ERR_NO_MEM
 */
cu_err_t wxp(const char* words, int* argc, char*** argv);

#ifdef __cplusplus
}
#endif

#endif