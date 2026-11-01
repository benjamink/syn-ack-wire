#include "prefs.h"

static char upper(char c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

static unsigned char allowed(char c)
{
    return (unsigned char)c > ' ' && (unsigned char)c <= 'Z';
}

unsigned char prefs_parse(const char *file, unsigned char len, char *handle)
{
    static const char key[] = "HANDLE=";
    unsigned char i = 0, k, n = 0;

    handle[0] = 0;
    while (i < len) {
        for (k = 0; key[k] && i + k < len && upper(file[i + k]) == key[k]; ++k)
            ;
        if (!key[k]) {
            for (i += k; i < len && file[i] != '\n' && file[i] != '\r' && n < HANDLE_MAX; ++i)
                if (allowed(upper(file[i])))
                    handle[n++] = upper(file[i]);
            handle[n] = 0;
            return n;
        }
        while (i < len && file[i] != '\n')      /* the next line */
            ++i;
        ++i;
    }
    return 0;
}

unsigned char prefs_server(const char *file, unsigned char len, char *url)
{
    static const char key[] = "SERVER=", tcp[] = "TCP://";
    unsigned char i = 0, k, n = 0, end;

    url[0] = 0;
    while (i < len) {
        for (k = 0; key[k] && i + k < len && upper(file[i + k]) == key[k]; ++k)
            ;
        if (!key[k]) {
            i += k;
            if (i + 1 < len && upper(file[i]) == 'N' && file[i + 1] == ':')
                i += 2;
            for (end = i; end < len && file[end] != '\n' && file[end] != '\r'; ++end)
                ;
            for (k = 0; tcp[k] && i + k < end && upper(file[i + k]) == tcp[k]; ++k)
                ;
            if (tcp[k] || end - i > SERVER_MAX)
                return 0;
            for (; i < end; ++i)
                url[n++] = file[i];
            url[n] = 0;
            return n;
        }
        while (i < len && file[i] != '\n')
            ++i;
        ++i;
    }
    return 0;
}

unsigned char prefs_format(const char *handle, const char *server, char *out)
{
    static const char key[] = "HANDLE=", skey[] = "SERVER=";
    unsigned char n = 0, i;

    for (i = 0; key[i]; ++i)
        out[n++] = key[i];
    for (i = 0; handle[i] && n < sizeof key - 1 + HANDLE_MAX; ++i)
        if (allowed(upper(handle[i])))
            out[n++] = upper(handle[i]);
    out[n++] = '\n';
    if (server[0]) {
        for (i = 0; skey[i]; ++i)
            out[n++] = skey[i];
        for (i = 0; server[i] && i < SERVER_MAX; ++i)
            out[n++] = server[i];
        out[n++] = '\n';
    }
    return n;
}
