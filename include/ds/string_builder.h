#ifndef MEDO_DS_STRING_BUILDER_H_
#define MEDO_DS_STRING_BUILDER_H_

#include "ds/dynamic_array.h"

da_Type(String_Builder, char);

#define sb_zero da_zero
#define sb_end da_end

#define sb_append_n da_append_n
#define sb_append_null(sb) sb_append_n(sb, "", 1)

#endif // MEDO_DS_STRING_BUILDER_H_
