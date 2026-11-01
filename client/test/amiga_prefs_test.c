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
    unsigned char n = prefs_format("NEON", file);
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
    CHECK(prefs_format("ABCDEFGHIJKL", file) == 16);           /* HANDLE= + 8 + newline */
}
static void test_handle_line_found_among_others(void)
{
    char handle[9];
    CHECK(prefs_parse("X=1\nHANDLE=ZED\nY=2\n", 19, handle) == 3);
    CHECK(strcmp(handle, "ZED") == 0);
}

int main(void)
{
    RUN(test_roundtrip); RUN(test_lowercase_upcased); RUN(test_bad_chars_dropped);
    RUN(test_missing_file_gives_empty); RUN(test_overlong_truncated_to_8); RUN(test_handle_line_found_among_others);
    printf("%d tests, %d failures\n", ran, failures);
    return failures != 0;
}
