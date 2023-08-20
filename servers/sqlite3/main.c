#include "sqlite3.h"
#include <libs/common/print.h>
#include <libs/common/string.h>

extern char _binary_test_sqlite3_start[];
extern char _binary_test_sqlite3_end[];
extern char _binary_test_sqlite3_size[];

uint8_t *get_database(void) {
    return (uint8_t *) _binary_test_sqlite3_start;
}

size_t get_database_size(void) {
    return (size_t) _binary_test_sqlite3_size;
}

static void not_implemented() {
    backtrace();
    NYI();
}

static int hinaosRead(sqlite3_file *file, void *pBuf, int iAmt, sqlite3_int64 iOfst) {
    // TRACE("read: pBuf=%x, offset=%x, size=%x", pBuf, iOfst, iAmt);
    memcpy(pBuf, get_database() + iOfst, iAmt);
    return SQLITE_OK;
}

static int hinaosDeviceCharacteristics(sqlite3_file *pFile) {
  return SQLITE_IOCAP_ATOMIC |
         SQLITE_IOCAP_POWERSAFE_OVERWRITE |
         SQLITE_IOCAP_SAFE_APPEND |
         SQLITE_IOCAP_SEQUENTIAL;
}

static int hinaosFileControl(sqlite3_file *pFile, int op, void *pArg){
  int rc = SQLITE_NOTFOUND;
  return rc;
}

static int hinaosSync(sqlite3_file *pFile, int flags){
  return SQLITE_OK;
}

static int hinaosFileSize(sqlite3_file *pFile, sqlite_int64 *pSize){
  *pSize = get_database_size();
  return SQLITE_OK;
}

static int hinaosSectorSize(sqlite3_file *pFile){
  return 1024;
}

static int hinaosLock(sqlite3_file *pFile, int eLock){
  return SQLITE_OK;
}

static int hinaosUnlock(sqlite3_file *pFile, int eLock){
  return SQLITE_OK;
}

static int hinaosCheckReservedLock(sqlite3_file *pFile, int *pResOut){
  *pResOut = 0;
  return SQLITE_OK;
}

int hinaosClose(sqlite3_file *file) {
    WARN("closing sqlite3 file...");
    backtrace();
    return SQLITE_OK;
}

static int sqlite3_os_open(
  sqlite3_vfs *pVfs,
  const char *zName,
  sqlite3_file *pFile,
  int flags,
  int *pOutFlags
){
    static sqlite3_io_methods file_methods = {
      3,                              /* iVersion */
      hinaosClose,                      /* xClose */
      hinaosRead,                       /* xRead */
      (void *) not_implemented,                      /* xWrite */
      (void *) not_implemented,                   /* xTruncate */
      (void *) hinaosSync,                       /* xSync */
      (void *) hinaosFileSize,                   /* xFileSize */
      (void *) hinaosLock,                       /* xLock */
      (void *) hinaosUnlock,                     /* xUnlock */
      (void *) hinaosCheckReservedLock,          /* xCheckReservedLock */
      (void *) hinaosFileControl,                /* xFileControl */
      (void *) hinaosSectorSize,                 /* xSectorSize */
      (void *) hinaosDeviceCharacteristics,      /* xDeviceCharacteristics */
      (void *) not_implemented,                     /* xShmMap */
      (void *) not_implemented,                    /* xShmLock */
      (void *) not_implemented,                 /* xShmBarrier */
      (void *) not_implemented,                   /* xShmUnmap */
      (void *) not_implemented,                      /* xFetch */
      (void *) not_implemented                     /* xUnfetch */
    };

    pFile->pMethods = &file_methods;
    return SQLITE_OK;
}

SQLITE_API int sqlite3_os_end(void) {
    NYI();
}

static int hinaosFullPathname(
  sqlite3_vfs *pVfs,
  const char *zPath,
  int nOut,
  char *zOut
){
    sqlite3_snprintf(nOut, zOut, "%s", zPath);
    return SQLITE_OK;
}

static int hinaosAccess(
  sqlite3_vfs *pVfs,
  const char *zPath,
  int flags,
  int *pResOut
){
    *pResOut = 0;
    return SQLITE_OK;
}


SQLITE_API int sqlite3_os_init(void) {
    static  sqlite3_vfs vfs = {
      1,                   /* fVersion */
      0,                   /* szOsFile */
      32,                  /* mxPathname */
      0,                   /* pNext */
      "hinaos",               /* zName */
      0,                   /* pAppData */
      sqlite3_os_open,     /* xOpen */
      (void *) not_implemented,     /* xDelete */
      hinaosAccess,     /* xAccess */
      hinaosFullPathname,     /* xFullPathname */
      (void *) not_implemented,     /* xDlOpen */
      (void *) not_implemented,     /* xDlError */
      (void *) not_implemented,     /* xDlSym */
      (void *) not_implemented,     /* xDlClose */
      (void *) not_implemented,     /* xRandomness */
      (void *) not_implemented,     /* xSleep */
      (void *) not_implemented,     /* xCurrentTime */
      (void *) not_implemented,     /* xGetLastError */
      (void *) not_implemented,     /* xCurrentTimeInt64 */
      (void *) not_implemented,     /* xSetSystemCall */
      (void *) not_implemented,     /* xGetSystemCall */
      (void *) not_implemented,     /* xNextSystemCall */
    };

    sqlite3_vfs_register(&vfs, 1 /* make default */);
    return SQLITE_OK;
}

void gmtime() {
    NYI();
}

void strftime() {
    NYI();
}

// libgcc.a
void *_impure_ptr = (void *) 0xdead0000;
void fprintf() { NYI(); }
void fflush() { NYI(); }
void abort() { NYI(); }

static int callback(void *unused, int argc, char **argv, char **col_name) {
    int i;
    for(i = 0; i < argc; i++) {
        DBG("%s = %s", col_name[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

void main() {
    int rc;
    sqlite3 *db;

    INFO("opening database...");
    rc = sqlite3_open("test.db", &db);
    if(rc) {
        PANIC("can't open sqlite3 database: %s\n", sqlite3_errmsg(db));
    }

    char *errmsg;
    const char* sql = "SELECT * FROM users;";
    INFO("executing query: %s", sql);
    rc = sqlite3_exec(db, sql, callback, 0, &errmsg);
    if(rc != SQLITE_OK) {
        PANIC("sqlite3 error: %s\n", errmsg);
    }

    INFO("done!");
}
