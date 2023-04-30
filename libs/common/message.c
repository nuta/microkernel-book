#include <libs/common/message.h>

// IPCスタブジェネレータが生成するコンパイル時チェック
IPCSTUB_STATIC_ASSERTIONS

// メッセージの種類に対応する名前を返す。デバッグ用。
const char *msgtype2str(int type) {
    int id = MSG_ID(type);
    if (id == 0 || id > IPCSTUB_MSGID_MAX) {
        return "(invalid)";
    }

    return IPCSTUB_MSGID2STR[id];
}
