#define _DEFAULT_SOURCE
#include <string.h>

#include "ds/string_builder.h"

int str_rindexc_from(String_Builder sb, char c, size_t from)
{
    char s[2] = {c, '\0'};
    return str_rindexs_from(sb, s, from);
}

int str_rindexs_from(String_Builder sb, const char *s, size_t from)
{
    assert(from < sb.size);
    for (int i = from; i >= 0; i--) {
        if (*strchrnul(s, sb.data[i]) != '\0') {
            return i;
        }
    }
    return -1;
}
