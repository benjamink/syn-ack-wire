/* Host tests for src/amiga/prefs.c: the saved handle's file, S:SynAckWire.prefs.
 *   make test-amiga-prefs */
#include <stdio.h>
#include <string.h>
#include "../src/amiga/prefs.h"

static int failures, ran;
#define CHECK(c) do { if (!(c)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)
#define RUN(t) do { ++ran; printf("%s\n", #t); t(); } while (0)

static void test_roundtrip(void)
{
    char file[32], handle[9];
    unsigned char n = prefs_format("NEON", "", file);
    CHECK(strncmp(file, "HANDLE=NEON\n", n) == 0 && n == 12);
    CHECK(prefs_parse(file, n, handle) == 4);
    CHECK(strcmp(handle, "NEON") == 0);
}
static void test_lowercase_upcased(void)
{
    char handle[9];
    CHECK(prefs_parse("HANDLE=mona\n", 12, handle) == 4);
    CHECK(strcmp(handle, "MONA") == 0);
    CHECK(prefs_parse("handle=case\n", 12, handle) == 4);      /* the key too */
    CHECK(strcmp(handle, "CASE") == 0);
}
static void test_bad_chars_dropped(void)
{
    char handle[9];
    CHECK(prefs_parse("HANDLE=a b{c}~d\r\n", 17, handle) == 4);  /* space and above Z go; CR ends it */
    CHECK(strcmp(handle, "ABCD") == 0);
}
static void test_missing_file_gives_empty(void)
{
    char handle[9] = "JUNK";
    CHECK(prefs_parse("", 0, handle) == 0);
    CHECK(handle[0] == 0);
    CHECK(prefs_parse("OTHER=1\n", 8, handle) == 0);            /* no HANDLE line */
    CHECK(handle[0] == 0);
}
static void test_overlong_truncated_to_8(void)
{
    char handle[9], file[32];
    CHECK(prefs_parse("HANDLE=ABCDEFGHIJKL\n", 20, handle) == 8);
    CHECK(strcmp(handle, "ABCDEFGH") == 0);
    CHECK(prefs_format("ABCDEFGHIJKL", "", file) == 16);           /* HANDLE= + 8 + newline */
}
static void test_handle_line_found_among_others(void)
{
    char handle[9];
    CHECK(prefs_parse("X=1\nHANDLE=ZED\nY=2\n", 19, handle) == 3);
    CHECK(strcmp(handle, "ZED") == 0);
}

static void test_server_line_found(void)
{
    char url[SERVER_MAX + 1];
    const char *f = "HANDLE=ZED\nSERVER=TCP://192.168.1.86:6599/\n";
    CHECK(prefs_server(f, (unsigned char)strlen(f), url) == 24);
    CHECK(strcmp(url, "TCP://192.168.1.86:6599/") == 0);
}
static void test_server_accepts_the_n_prefix_and_lowercase_key(void)
{
    char url[SERVER_MAX + 1];
    const char *f = "server=N:TCP://mark.local:6599/\r\n";
    CHECK(prefs_server(f, (unsigned char)strlen(f), url) == 22);
    CHECK(strcmp(url, "TCP://mark.local:6599/") == 0);          /* host case is kept: it is a name */
}
static void test_server_must_be_tcp(void)
{
    char url[SERVER_MAX + 1] = "JUNK";
    const char *f = "SERVER=HTTP://x/\n";
    CHECK(prefs_server(f, (unsigned char)strlen(f), url) == 0 && url[0] == 0);
    CHECK(prefs_server("", 0, url) == 0);
    CHECK(prefs_server("HANDLE=A\n", 9, url) == 0);
}
static void test_server_too_long_is_ignored(void)
{
    char url[SERVER_MAX + 1], f[200] = "SERVER=TCP://";
    memset(f + 13, 'a', 100);
    memcpy(f + 113, "/\n", 3);
    CHECK(prefs_server(f, 115, url) == 0);                       /* never a truncated, wrong address */
}
static void test_format_keeps_the_server(void)
{
    char file[120], handle[9], url[SERVER_MAX + 1];
    unsigned char n = prefs_format("NEON", "TCP://h:1/", file);
    CHECK(prefs_parse(file, n, handle) == 4 && strcmp(handle, "NEON") == 0);
    CHECK(prefs_server(file, n, url) == 10 && strcmp(url, "TCP://h:1/") == 0);
    n = prefs_format("NEON", "", file);
    CHECK(prefs_server(file, n, url) == 0);                      /* none known: no SERVER line written */
}

int main(void)
{
    RUN(test_roundtrip); RUN(test_lowercase_upcased); RUN(test_bad_chars_dropped);
    RUN(test_missing_file_gives_empty); RUN(test_overlong_truncated_to_8); RUN(test_handle_line_found_among_others);
    RUN(test_server_line_found); RUN(test_server_accepts_the_n_prefix_and_lowercase_key); RUN(test_server_must_be_tcp);
    RUN(test_server_too_long_is_ignored); RUN(test_format_keeps_the_server);
    printf("%d tests, %d failures\n", ran, failures);
    return failures != 0;
}
