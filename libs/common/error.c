#include "types.h"
#include <libs/common/error.h>

// エラー番号とエラーメッセージの対応表。
static const char *error_names[] = {
    [-OK] = "Success",
    [-ERR_NO_MEMORY] = "No Memory",
    [-ERR_NO_RESOURCES] = "No Resources Available",
    [-ERR_ALREADY_EXISTS] = "Already Exists",
    [-ERR_ALREADY_USED] = "Already In Use",
    [-ERR_ALREADY_DONE] = "Already Done",
    [-ERR_STILL_USED] = "Still In Use",
    [-ERR_NOT_FOUND] = "Not Found",
    [-ERR_NOT_ALLOWED] = "Not Allowed",
    [-ERR_NOT_SUPPORTED] = "Not Supported",
    [-ERR_UNEXPECTED] = "Unexpected Error",
    [-ERR_INVALID_ARG] = "Invalid Argument",
    [-ERR_INVALID_TASK] = "Invalid Task",
    [-ERR_INVALID_SYSCALL] = "Invalid System Call",
    [-ERR_INVALID_PADDR] = "Invalid Physical Address",
    [-ERR_INVALID_UADDR] = "Invalid User Address",
    [-ERR_TOO_MANY_TASKS] = "Too Many Tasks",
    [-ERR_TOO_LARGE] = "Too Large",
    [-ERR_TOO_SMALL] = "Too Small",
    [-ERR_WOULD_BLOCK] = "Operation Would Block",
    [-ERR_TRY_AGAIN] = "Try Again",
    [-ERR_ABORTED] = "Aborted",
    [-ERR_EMPTY] = "Empty",
    [-ERR_NOT_EMPTY] = "Not Empty",
    [-ERR_DEAD_LOCK] = "Deadlock Detected",
    [-ERR_NOT_A_FILE] = "Not A File",
    [-ERR_NOT_A_DIR] = "Not A Directory",
    [-ERR_EOF] = "End of File",
};

// エラー番号からエラーメッセージを取得する。
const char *err2str(int err) {
    if (err < ERR_END || err > 0) {
        return "(Unknown Error)";
    }

    return error_names[-err];
}
