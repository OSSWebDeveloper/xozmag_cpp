// SQLite'ning dastur ishlatadigan funksiyalari.
// To'liq sqlite3.h o'rniga — kutubxonaning o'zi tashqi/sqlite3.dll da
// (SQLite public domain), g++ unga to'g'ridan-to'g'ri ulanadi.
#pragma once

extern "C" {
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long sqlite3_int64;

int sqlite3_initialize(void);
int sqlite3_open_v2(const char* fayl, sqlite3** db, int bayroqlar, const char* vfs);
int sqlite3_close_v2(sqlite3* db);
int sqlite3_exec(sqlite3* db, const char* sql, int (*cb)(void*, int, char**, char**), void* arg,
                 char** xato);
void sqlite3_free(void* p);
int sqlite3_prepare_v2(sqlite3* db, const char* sql, int uzunlik, sqlite3_stmt** st,
                       const char** qolgan);
int sqlite3_step(sqlite3_stmt* st);
int sqlite3_reset(sqlite3_stmt* st);
int sqlite3_finalize(sqlite3_stmt* st);
int sqlite3_bind_text(sqlite3_stmt* st, int i, const char* matn, int uzunlik, void (*del)(void*));
int sqlite3_bind_double(sqlite3_stmt* st, int i, double qiymat);
int sqlite3_bind_int64(sqlite3_stmt* st, int i, sqlite3_int64 qiymat);
int sqlite3_bind_null(sqlite3_stmt* st, int i);
const unsigned char* sqlite3_column_text(sqlite3_stmt* st, int i);
double sqlite3_column_double(sqlite3_stmt* st, int i);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* st, int i);
int sqlite3_column_type(sqlite3_stmt* st, int i);
int sqlite3_column_count(sqlite3_stmt* st);
const char* sqlite3_errmsg(sqlite3* db);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db);
int sqlite3_changes(sqlite3* db);
int sqlite3_busy_timeout(sqlite3* db, int ms);
const char* sqlite3_libversion(void);
}

#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_NULL 5
#define SQLITE_OPEN_READONLY 0x00000001
#define SQLITE_OPEN_READWRITE 0x00000002
#define SQLITE_OPEN_CREATE 0x00000004
#define SQLITE_OPEN_URI 0x00000040
#define SQLITE_TRANSIENT ((void (*)(void*))-1)
