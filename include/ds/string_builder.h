#ifndef MEDO_DS_STRING_BUILDER_H_
#define MEDO_DS_STRING_BUILDER_H_

#include "ds/dynamic_array.h"

da_Type(String_Builder, char);

#define sb_zero da_zero
#define sb_end da_end
#define sb_remove_from da_remove_from

#define sb_append_n da_append_n
#define sb_append_cstr(sb, cstr) sb_append_n(sb, cstr, strlen(cstr))

int str_rindexs_from(String_Builder sb, const char *s, size_t from);
int str_rindexc_from(String_Builder sb, char c, size_t from);

#endif // MEDO_DS_STRING_BUILDER_H_
