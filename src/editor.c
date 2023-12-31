#define _DEFAULT_SOURCE

#include "editor.h"
#include "ds/string_builder.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SV_IMPLEMENTATION
#include "sv.h"

#define sv_c_str(c_chunk, sv_chunk)                     \
    c_chunk = malloc(sv_chunk.count + 1);               \
    memcpy(c_chunk, sv_chunk.data, sv_chunk.count);     \
    c_chunk[sv_chunk.count] = '\0';                

// File I/O
static void save_file(const Editor *e);
static void open_file(Editor *e, const char *filename);
static void open_dir(Editor *e, const char *dirname);

// Editor Operations
#define editor_delete_char(e) editor_delete_char_at(e, e->c);
static void editor_browsing(Editor *e, EditorKey key);
static void editor_action(Editor *e, EditorKey key);
static size_t editor_select(Editor *e, EditorKey key, size_t cur);
#define editor_paste(e) editor_write(e, e->clipboard);

static void update_pathname(Editor *e, String_View path);

static size_t editor_selection_delete(Editor *e);

// Search operations
static void editor_search_start(Editor *e);
static int editor_search_next(Editor *e, size_t cur);
static int editor_search_prev(Editor *e, size_t cur);


Editor editor_init(void)
{
    Editor e = {0};

    da_zero(&e.pathname);
    e.clipboard = NULL;
    e.mode = EM_EDITING;

    char cwdbuf[256];
    getcwd(cwdbuf, sizeof(cwdbuf));
    sb_append_cstr(&e.pathname, cwdbuf);

    e.be = (Basic_Editor) {0};

    editor_open(&e, (String_View) SV_STATIC("."));

    e.match = -1;

    return e;
}

void editor_clear(Editor *e)
{
    e->mode = EM_EDITING;
    e->be.cur = 0;
    e->be.data.size = 0;
}

static_assert(sizeof(Editor) == 176, "Editor structure has changed");

void editor_process_key(Editor *e, EditorKey key)
{
    switch (e->mode) {
        case EM_SELECTION: 
        case EM_EDITING: {
            switch (key) {
                case EK_LEFT:
                case EK_RIGHT:
                case EK_UP:
                case EK_DOWN:
                case EK_LEFTW:
                case EK_RIGHTW:
                case EK_LINE_HOME:
                case EK_LINE_END:
                case EK_HOME:
                case EK_END:
                case EK_PAGEUP:
                case EK_PAGEDOWN: 
                case EK_NEXT_PARAGRAPH: 
                case EK_PREV_PARAGRAPH: {
                    if (e->mode == EM_SELECTION) {
                        e->mode = EM_EDITING;
                        if (((key == EK_UP   || key == EK_LEFT ) && e->be.cur > e->select_cur) ||
                            ((key == EK_DOWN || key == EK_RIGHT) && e->be.cur < e->select_cur))
                        {
                            e->be.cur = e->select_cur;
                        }
                        return;
                    }

                    e->be.cur = editor_move(e, key, e->be.cur);
                } break;

                case EK_BACKSPACE:
                case EK_DELETE:
                case EK_BACKSPACEW:
                case EK_DELETEW:
                case EK_LINE_BELOW:
                case EK_LINE_ABOVE:
                case EK_REMOVE_LINE:
                case EK_MERGE_LINE:
                case EK_RETURN: // Return effect defaults to BREAK_LINE
                case EK_BREAK_LINE:
                case EK_TAB: {
                    if (e->mode == EM_SELECTION) {
                        e->be.cur = editor_selection_delete(e);
                        e->mode = EM_EDITING;
                        return ;
                    }
                    e->be.cur = editor_edit(e, key, e->be.cur);
                } break;

                case EK_INDENT:
                case EK_UNINDENT: {
                    e->be.cur = editor_edit(e, key, e->be.cur);
                } break;

                case EK_SAVE:
                case EK_COPY:
                case EK_PASTE: 
                case EK_CUT: 
                case EK_SEARCH_START: {
                    editor_action(e, key);
                } break;

                case EK_SELECT_LEFT:
                case EK_SELECT_RIGHT:
                case EK_SELECT_UP:
                case EK_SELECT_DOWN: 
                case EK_SELECT_LEFTW: 
                case EK_SELECT_RIGHTW:
                case EK_SELECT_LINE_HOME:
                case EK_SELECT_LINE_END:
                case EK_SELECT_WORD:
                case EK_SELECT_LINE:
                case EK_SELECT_OUTER_BLOCK:
                case EK_SELECT_NEXT_BLOCK:
                case EK_SELECT_HOME:
                case EK_SELECT_END:
                case EK_SELECT_NEXT_PARAGRAPH:
                case EK_SELECT_PREV_PARAGRAPH:
                case EK_SELECT_ALL: {
                    e->be.cur = editor_select(e, key, e->be.cur);
                } break;

                case EK_ESC: {
                    e->mode = EM_EDITING;
                } break;

                default:
                    printf("unreachable: %s: %d\n", __FILE__, __LINE__);
            }
        } break;

        case EM_BROWSING: {
            switch (key) {
                case EK_LEFT:
                case EK_RIGHT:
                case EK_UP:
                case EK_DOWN:
                case EK_PAGEUP:
                case EK_PAGEDOWN: 
                case EK_RETURN:
                case EK_HOME:
                case EK_END: {
                    editor_browsing(e, key);
                } break;

                default: break;
            }
        } break;

        case EM_SEARCHING: {
            switch (key) {
                case EK_ESC: {
                    e->mode = EM_EDITING;
                    if (e->match != -1) {
                        size_t searchlen = strlen(e->searchbuf);
                        e->be.cur += searchlen;
                        e->select_cur = e->be.cur;
                        for (size_t i = searchlen; i > 0; i--) {
                            e->select_cur = editor_move(e, EK_LEFT, e->select_cur);
                        }
                        e->mode = EM_SELECTION;
                    }
                } break;

                case EK_SEARCH_NEXT: {
                    size_t cur = editor_move(e, EK_RIGHT, e->be.cur);
                    e->match = editor_search_next(e, cur);
                    if (e->match != -1) {
                        e->be.cur = e->match;
                    }
                } break;

                case EK_SEARCH_PREV: {
                    size_t cur = editor_move(e, EK_LEFT, e->be.cur);
                    e->match = editor_search_prev(e, cur);
                    if (e->match != -1) {
                        e->be.cur = e->match;
                    }
                } break;
                
                case EK_BACKSPACE: {
                    size_t searchlen = strlen(e->searchbuf);
                    if (searchlen > 0) {
                        e->searchbuf[searchlen - 1] = '\0';
                    }
                    e->match = editor_search_next(e, e->be.cur);
                    if (e->match != -1) {
                        e->be.cur = e->match;
                    }
                }

                default:
                    break;
            }
        } break;
    }
}

static_assert(EK_COUNT == 52, "The number of editor keys has changed");

size_t editor_write_at(Editor *e, const char *s, size_t at)
{
    if (e->mode == EM_BROWSING) {
        return at;
    }

    if (e->mode == EM_SEARCHING) {
        size_t searchlen = strlen(e->searchbuf);
        assert(searchlen + strlen(s) < 64);
        strcpy(&e->searchbuf[searchlen], s);
        e->match = editor_search_next(e, e->be.cur);
        if (e->match != -1) {
            at = e->match;
        }
        return at;
    }

    if (e->mode == EM_SELECTION) {
        e->be.cur = editor_selection_delete(e);
        e->mode = EM_EDITING;
    }

    at = be_insert_sn(&e->be, s, strlen(s));
    return at;
}

/* Editor Operations */

static size_t editor_selection_delete(Editor *e)
{
    size_t begin = e->select_cur;
    size_t end = e->be.cur;
    if (begin > end) {
        size_t tmp = begin;
        begin = end;
        end = tmp;
    }
    size_t n = end - begin;

    be_delete_n_from(&e->be, n, begin);

    return begin;
}

static void editor_selection_copy(Editor *e)
{
    if (e->clipboard) free(e->clipboard);

    size_t begin = e->select_cur;
    size_t end = e->be.cur;
    if (begin > end) {
        size_t tmp = begin;
        begin = end;
        end = tmp;
    }
    size_t n = end - begin;

    e->clipboard = malloc(n + 1);
    strncpy(e->clipboard, e->be.data.data + begin, n);
    e->clipboard[n] = '\0';
}

size_t editor_move(Editor *e, EditorKey key, size_t cur)
{
#define issymbol(c) (isalnum(c) || c == '_')
    switch (key) {
        case EK_LEFT: {
            if (cur > 0) cur--;
        } break;

        case EK_RIGHT: {
            if (cur < e->be.data.size) cur++;
        } break;

        case EK_UP: {
            cur = be_move_up(&e->be, cur);
        } break;

        case EK_DOWN: {
            cur = be_move_down(&e->be, cur);
        } break;

        case EK_LEFTW: {
            cur = be_move_leftw(&e->be, cur);
        } break;

        case EK_RIGHTW: {
            cur = be_move_rightw(&e->be, cur);
        } break;

        case EK_LINE_HOME: {
            cur = be_move_home(&e->be, cur);
        } break;

        case EK_LINE_END: {
            cur = be_move_end(&e->be, cur);
        } break;

        case EK_HOME: {
            cur = 0;
        } break;

        case EK_END: {
            cur = e->be.data.size;
        } break;

        case EK_NEXT_PARAGRAPH: {
            cur = be_move_next_paragraph(&e->be, cur);
        } break;
        
        case EK_PREV_PARAGRAPH: {
            cur = be_move_prev_paragraph(&e->be, cur);
        } break;

        default:
            assert(0);
    }

    return cur;
#undef issymbol
}

size_t editor_edit(Editor *e, EditorKey key, size_t cur)
{
    switch (key) {
        case EK_BACKSPACE: {
            cur = be_backspace(&e->be);
        } break;

        case EK_DELETE: {
            be_delete(&e->be);
        } break;

        // case EK_BACKSPACEW: {
        //     editor_selection_start(e, e->c);
        //     pos = editor_move(e, EK_LEFTW, pos);
        //     e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
        //     editor_selection_stop(e);
        // } break;

        // case EK_DELETEW: {
        //     editor_selection_start(e, e->c);
        //     pos = editor_move(e, EK_RIGHTW, pos);
        //     e->be.cur = editor_selection_delete(e, e->be.cur, e->select_cur);
        //     editor_selection_stop(e);
        // } break;

        case EK_TAB: {
            Line line = be_get_line(&e->be, e->be.cur);
            size_t col = e->be.cur - line.home;
            size_t tabstop = 4 - (col % 4);
            for (size_t i = 0; i < tabstop; i++) {
                e->be.cur = editor_write_at(e, " ", e->be.cur);
            }
        } break;

        case EK_INDENT: {
            // TODO
        } break; 

        case EK_UNINDENT: {
            // TODO
        } break;

        case EK_LINE_ABOVE: {
            cur = be_insert_line_above(&e->be, e->be.cur);
        } break;

        case EK_LINE_BELOW: {
            cur = be_insert_line_below(&e->be, e->be.cur);
        } break;

        case EK_REMOVE_LINE: {
            // TODO
        } break;

        case EK_RETURN:
        case EK_BREAK_LINE: {
            cur = be_insert_s(&e->be, "\n");
        } break;

        default:
            assert(0);
    }

    return cur;
}

static void editor_action(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_SAVE: {
            save_file(e);
        } break;

        case EK_SEARCH_START: {
            if (e->mode != EM_SEARCHING) {
                editor_search_start(e);
            }
        } break;

        case EK_COPY: {
            editor_selection_copy(e);
        } break;

        case EK_PASTE: {
            e->be.cur = editor_write_at(e, e->clipboard, e->be.cur);
        } break;

        case EK_CUT: {
            editor_selection_copy(e);
            e->be.cur = editor_selection_delete(e);
        } break;

        default:
            assert(0);
    }
}

static void editor_browsing(Editor *e, EditorKey key)
{
    switch (key) {
        case EK_LEFT:
        case EK_UP: {
            e->be.cur = editor_move(e, EK_UP, e->be.cur);
        } break;
            
        case EK_RIGHT:
        case EK_DOWN: {
            e->be.cur = editor_move(e, EK_DOWN, e->be.cur);
        } break;

        case EK_RETURN: {
            Line line = be_get_line(&e->be, e->be.cur);
            String_View path = sv_from_parts(&e->be.data.data[line.home], line.end - line.home);
            editor_open(e, path);
        } break;

        case EK_HOME: {
            e->be.cur = 0;
        } break;

        case EK_END: {
            Line line = be_get_line(&e->be, e->be.data.size);
            e->be.cur = line.home;
        } break;    

        default:
            assert(0);
    }
}

static size_t editor_select(Editor *e, EditorKey key, size_t cur)
{
    if (e->mode != EM_SELECTION) {
        e->select_cur = e->be.cur;
    }

    switch (key) {
        case EK_SELECT_LEFT: {
            cur = editor_move(e, EK_LEFT, cur);
        } break;

        case EK_SELECT_RIGHT: {
            cur = editor_move(e, EK_RIGHT, cur);
        } break;

        case EK_SELECT_UP: {
            cur = editor_move(e, EK_UP, cur);
        } break;

        case EK_SELECT_DOWN: {
            cur = editor_move(e, EK_DOWN, cur);
        } break;

        case EK_SELECT_LEFTW: {
            cur = editor_move(e, EK_LEFTW, cur);
        } break;

        case EK_SELECT_RIGHTW: {
            cur = editor_move(e, EK_RIGHTW, cur);
        } break;

        case EK_SELECT_LINE_HOME: {
            cur = editor_move(e, EK_LINE_HOME, cur);
        } break;

        case EK_SELECT_LINE_END: {
            cur = editor_move(e, EK_LINE_END, cur);
        } break;

        case EK_SELECT_WORD: {
            cur = editor_move(e, EK_RIGHTW, cur);
            e->select_cur = editor_move(e, EK_LEFTW, cur);
        } break;

        case EK_SELECT_LINE: {
            Line line = be_get_line(&e->be, e->be.cur);
            e->select_cur = line.home;
            cur = line.end;
        } break;

        case EK_SELECT_OUTER_BLOCK: {
            size_t start_cur = e->select_cur;
            
            size_t stack_count = 0;
            while (start_cur > 0) {
                start_cur = editor_move(e, EK_LEFT, start_cur);
                if (strchr("{[(", e->be.data.data[start_cur]) != NULL) {
                    if (stack_count == 0) break;
                    stack_count--;
                }
                if (strchr("}])", e->be.data.data[start_cur]) != NULL) {
                    stack_count++;
                }
            }

            if (stack_count != 0) {
                e->mode = EM_EDITING;
                return cur;
            }

            size_t end_cur = cur;
            while (end_cur < e->be.data.size) {
                end_cur = editor_move(e, EK_RIGHT, end_cur);
                if (strchr("}])", e->be.data.data[end_cur]) != NULL) {
                    if (stack_count == 0) break;
                    stack_count--;
                }
                if (strchr("{[(", e->be.data.data[end_cur]) != NULL) {
                    stack_count++;
                }
            }

            if (stack_count != 0) {
                e->mode = EM_EDITING;
                return cur;
            }

            end_cur = editor_move(e, EK_RIGHT, end_cur);
            e->select_cur = start_cur;
            cur = end_cur;
        } break;

        // case EK_SELECT_NEXT_BLOCK: {
        //     const char *s = editor_get_line(e);
        //     size_t slen = strlen(s);

        //     while ((isspace(s[pos.x]) || s[pos.x] == '\0') && 
        //            (pos.x < slen || pos.y + 1 < e->lines.length)) 
        //     {
        //         pos = editor_move(e, EK_RIGHT, pos);
        //         s = editor_get_line(e);
        //         slen = strlen(s);
        //     }

        //     if (*strchrnul("{[(", s[pos.x]) != '\0') {
        //         find_scope_end(e);
        //         pos = editor_move(e, EK_RIGHT, pos);
        //     } else {
        //         while ((*strchrnul("{[(", s[pos.x]) == '\0') &&
        //                (pos.x < slen || pos.y + 1 < e->lines.length)) 
        //         {
        //             pos = editor_move(e, EK_RIGHT, pos);
        //             s = editor_get_line(e);
        //         }
        //     }
        // } break;

        case EK_SELECT_HOME: {
            cur = editor_move(e, EK_HOME, cur);
        } break;

        case EK_SELECT_END: {
            cur = editor_move(e, EK_END, cur);
        } break;
    
        case EK_SELECT_NEXT_PARAGRAPH: {
            cur = editor_move(e, EK_NEXT_PARAGRAPH, cur);
        } break;
        
        case EK_SELECT_PREV_PARAGRAPH: {
            cur = editor_move(e, EK_PREV_PARAGRAPH, cur);
        } break;
        
        case EK_SELECT_ALL: {
            e->select_cur = 0;
            cur = editor_move(e, EK_END, cur);
        } break;

        default:
            assert(0);
    }
    e->mode = EM_SELECTION;

    return cur;
}

static void editor_search_start(Editor *e)
{
    e->searchbuf[0] = '\0';
    e->mode = EM_SEARCHING;
}

static int editor_search_next(Editor *e, size_t cur)
{
    size_t i;
    for (i = cur; i < e->be.data.size; i++) {
        if (strncmp(e->searchbuf, &e->be.data.data[i], strlen(e->searchbuf)) == 0) {
            return i;
        }
    }

    for (i = 0; i < cur; i++) {
        if (strncmp(e->searchbuf, &e->be.data.data[i], strlen(e->searchbuf)) == 0) {
            return i;
        }   
    }
    
    return -1;
}

static int editor_search_prev(Editor *e, size_t cur)
{
    int i;
    for (i = cur; i >= 0; i--) {
        if (strncmp(e->searchbuf, &e->be.data.data[i], strlen(e->searchbuf)) == 0) {
            return i;
        }
    }

    for (i = (int) e->be.data.size - 1; i > (int) cur; i--) {
        if (strncmp(e->searchbuf, &e->be.data.data[i], strlen(e->searchbuf)) == 0) {
            return i;
        }
    }

    return -1;
}


static_assert(EK_COUNT == 52, "The number of editor keys has changed");

/* File I/O */

void editor_open(Editor *e, String_View path)
{
    update_pathname(e, path);
    sb_append_n(&e->pathname, "", 1);

    const char *pathname = e->pathname.data;
    errno = 0;
    struct stat statbuf;
    if (stat(pathname, &statbuf) != 0) {
        if (errno == ENOENT) return; // File does not exist, don't crash
        fprintf(stderr, "Could not get stat for \"%s\": %s", pathname, strerror(errno));
        exit(1);
    }

    editor_clear(e);

    mode_t mode = statbuf.st_mode & S_IFMT;
    if (mode == S_IFDIR) { // Directory
        open_dir(e, pathname);
        e->mode = EM_BROWSING;
    } else if (mode == S_IFREG) { // Regular file
        open_file(e, pathname);
        e->mode = EM_EDITING;
    }
    sb_remove_from(&e->pathname, e->pathname.size - 1);
}

static void save_file(const Editor *e)
{
    FILE *file = fopen(e->pathname.data, "w");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\": %s\n", e->pathname.data, strerror(errno));
        exit(1);
    }

    fwrite(e->be.data.data, 1, e->be.data.size, file);
    fclose(file);
}       

static void open_file(Editor *e, const char *filename)
{
    be_load_from_file(&e->be, filename);
}

static int entrycmp(const void *ap, const void *bp)
{
    const char *a = *(const char**)ap;
    const char *b = *(const char**)bp;
    return strcmp(b, a);
}

static void open_dir(Editor *e, const char *dirname)
{
    DIR *dirp = opendir(dirname);
    if (dirp == NULL) {
        fprintf(stderr, "Could not open dir \"%s\": %s\n", dirname, strerror(errno));
        exit(1);
    }

    da_var_zero(entries, char *);

    errno = 0;
    struct dirent *direntry;
    while ((direntry = readdir(dirp)) != NULL) {
        char *s = strdup(direntry->d_name);
        da_append(&entries, &s);
    }

    if (errno) {
        fprintf(stderr, "Could not read dir: \"%s\": %s\n", dirname, strerror(errno));
        if (closedir(dirp) != 0) {
            fprintf(stderr, "Could not close dir: \"%s\": %s\n", dirname, strerror(errno));
        };
        exit(1);
    }

    qsort(entries.data, entries.size, TYPESIZE(&entries), entrycmp);

    size_t cur = e->be.cur;
    for (size_t i = 0; i < entries.size; i++) {
        cur = editor_write_at(e, entries.data[i], cur);
        if (i + 1 != entries.size) {
            cur = editor_write_at(e, "\n", cur);
        }
    }

    da_end(&entries);
}

static void update_pathname(Editor *e, String_View path)
{
    assert(e->pathname.size > 0 && path.count > 0);
    assert(sv_index_of(path, '/', NULL) == false && "Cannot handle composite path; TODO: Implement such feature with String_View");

    if (strncmp(path.data, ".", path.count) == 0) {
        return;
    }

    if (strncmp(path.data, "..", path.count) == 0) {
        int index = str_rindexc_from(e->pathname, '/', e->pathname.size - 1);
        if (index <= 0) index = 1; // start from root '/' path
        e->pathname.size = index;
    } else {
        sb_append_cstr(&e->pathname, "/");
        sb_append_n(&e->pathname, path.data, path.count);
    }
}
