#include "fs.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>

void fs_read(const char *path) {
    task_t fs_server = ipc_lookup("fs");
    ASSERT_OK(fs_server);

    struct message m;
    m.type = FS_OPEN_MSG;
    strcpy_safe(m.fs_open.path, sizeof(m.fs_open.path), path);
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to open a file: '%s' (%s)", path, err2str(err));
        return;
    }

    ASSERT(m.type == FS_OPEN_REPLY_MSG);
    int fd = m.fs_open_reply.fd;

    while (true) {
        m.type = FS_READ_MSG;
        m.fs_read.fd = fd;
        m.fs_read.len = PAGE_SIZE;
        error_t err = err = ipc_call(fs_server, &m);
        if (err == ERR_EOF) {
            break;
        }

        if (IS_ERROR(err)) {
            WARN("failed to read a file: %s", err2str(err));
            return;
        }

        ASSERT(m.type == FS_READ_REPLY_MSG);
        unsigned end =
            MIN(sizeof(m.fs_read_reply.data) - 1, m.fs_read_reply.data_len);
        m.fs_read_reply.data[end] = '\0';
        DBG("%s", m.fs_read_reply.data);
    }
}

void fs_write(const char *path, const uint8_t *buf, size_t len) {
    task_t fs_server = ipc_lookup("fs");
    ASSERT_OK(fs_server);

    struct message m;
    if (len > sizeof(m.fs_write.data)) {
        WARN("too large data to write");
        return;
    }

    m.type = FS_MKFILE_MSG;
    strcpy_safe(m.fs_mkfile.path, sizeof(m.fs_mkfile.path), path);
    error_t err = ipc_call(fs_server, &m);
    if (err != OK && err != ERR_ALREADY_EXISTS) {
        WARN("failed to create a file: '%s' (%s)", path, err2str(err));
        return;
    }

    m.type = FS_OPEN_MSG;
    strcpy_safe(m.fs_open.path, sizeof(m.fs_open.path), path);
    err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to open a file: '%s' (%s)", path, err2str(err));
        return;
    }

    ASSERT(m.type == FS_OPEN_REPLY_MSG);
    int fd = m.fs_open_reply.fd;

    m.type = FS_WRITE_MSG;
    m.fs_write.fd = fd;
    memcpy(m.fs_write.data, buf, len);
    m.fs_write.data_len = len;
    err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to write into a file: %s", err2str(err));
        return;
    }

    ASSERT(m.type == FS_WRITE_REPLY_MSG);
}

void fs_listdir(const char *path) {
    task_t fs_server = ipc_lookup("fs");
    ASSERT_OK(fs_server);

    TRACE("Contents of %s:", path);
    for (int i = 0;; i++) {
        struct message m;
        m.type = FS_READDIR_MSG;
        m.fs_readdir.index = i;
        strcpy_safe(m.fs_readdir.path, sizeof(m.fs_readdir.path), path);
        error_t err = ipc_call(fs_server, &m);
        if (err == ERR_EOF) {
            return;
        }

        if (IS_ERROR(err)) {
            WARN("failed to readdir: '%s' (%s)", path, err2str(err));
            return;
        }

        ASSERT(m.type == FS_READDIR_REPLY_MSG);
        TRACE("[%s] \"%s\"",
              (m.fs_readdir_reply.type == 0xff) ? "FILE" : "DIR ",
              m.fs_readdir_reply.name);
    }
}

void fs_mkdir(const char *path) {
    task_t fs_server = ipc_lookup("fs");
    ASSERT_OK(fs_server);

    struct message m;
    m.type = FS_MKDIR_MSG;
    strcpy_safe(m.fs_mkdir.path, sizeof(m.fs_mkdir.path), path);
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to create a directory: '%s' (%s)", path, err2str(err));
        return;
    }

    ASSERT(m.type == FS_MKDIR_REPLY_MSG);
}

void fs_delete(const char *path) {
    task_t fs_server = ipc_lookup("fs");
    ASSERT_OK(fs_server);

    struct message m;
    m.type = FS_DELETE_MSG;
    strcpy_safe(m.fs_delete.path, sizeof(m.fs_delete.path), path);
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to delete an entry: '%s' (%s)", path, err2str(err));
        return;
    }

    ASSERT(m.type == FS_DELETE_REPLY_MSG);
}
