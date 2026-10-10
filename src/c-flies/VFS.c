#define _CRT_SECURE_NO_WARNINGS

#include "VFS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>

static VNode* g_root = NULL;
static char g_name[256] = "default";
static char* g_physical = NULL;
static int g_dirs = 0;
static int g_files = 0;

static void make_empty(void);

void console_printf(const char* fmt, ...) {
    char buf[2048];
    wchar_t wide[2048];
    va_list ap;
    int n;
    HANDLE out;
    DWORD written = 0;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, -1, wide, 2048);
    if (n <= 0)
        n = MultiByteToWideChar(1251, 0, buf, -1, wide, 2048);

    out = GetStdHandle(STD_OUTPUT_HANDLE);
    fflush(stdout);
    if (n > 1 && WriteConsoleW(out, wide, (DWORD)(n - 1), &written, NULL))
        return;
    fputs(buf, stdout);
}

static void* xmalloc(size_t n) {
    void* p = malloc(n ? n : 1);
    if (!p) {
        console_printf("Недостаточно памяти\n");
        exit(1);
    }
    return p;
}

static char* xstrdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* d = (char*)xmalloc(n);
    memcpy(d, s, n);
    return d;
}

static void node_free(VNode* n) {
    if (!n)
        return;
    node_free(n->child);
    node_free(n->next);
    free(n->name);
    free(n->data);
    free(n);
}

const char* vfs_name(void) {
    return g_name;
}

void vfs_use_name(const char* name) {
    if (!g_root)
        make_empty();
    if (!name || !name[0])
        snprintf(g_name, sizeof(g_name), "default");
    else
        snprintf(g_name, sizeof(g_name), "%s", name);
}

VNode* vfs_root(void) {
    return g_root;
}

static void make_empty(void) {
    node_free(g_root);
    g_root = (VNode*)xmalloc(sizeof(VNode));
    memset(g_root, 0, sizeof(VNode));
    g_root->name = xstrdup("");
    g_root->is_dir = 1;
}

static int read_file(VNode* node, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long sz = ftell(f);
    if (sz < 0 || sz > 1024L * 1024L) {
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    char* buf = (char*)xmalloc((size_t)sz + 1);
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    node->data = buf;
    node->data_len = n;
    return 0;
}

static void add_child(VNode* parent, VNode* child) {
    child->parent = parent;
    child->next = parent->child;
    parent->child = child;
}

static int load_dir(VNode* parent, const char* disk_dir, int depth) {
    char pattern[MAX_PATH];
    if (depth > 64)
        return -1;
    if (strlen(disk_dir) + 3 >= MAX_PATH)
        return -1;
    snprintf(pattern, sizeof(pattern), "%s\\*", disk_dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    int rc = 0;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            continue;

        char child_path[MAX_PATH];
        if (strlen(disk_dir) + 1 + strlen(fd.cFileName) + 1 >= MAX_PATH) {
            rc = -1;
            break;
        }
        snprintf(child_path, sizeof(child_path), "%s\\%s", disk_dir, fd.cFileName);

        VNode* node = (VNode*)xmalloc(sizeof(VNode));
        memset(node, 0, sizeof(VNode));
        node->name = xstrdup(fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            node->is_dir = 1;
            add_child(parent, node);
            g_dirs++;
            if (load_dir(node, child_path, depth + 1) != 0)
                rc = -1;
        }
        else {
            if (read_file(node, child_path) != 0) {
                free(node->name);
                free(node);
                rc = -1;
            }
            else {
                add_child(parent, node);
                g_files++;
            }
        }
        if (rc != 0)
            break;
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return rc;
}

int vfs_load(const char* path) {
    if (!g_root)
        make_empty();

    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        console_printf("Ошибка загрузки VFS: файл не найден: %s\n", path);
        return 1;
    }
    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        console_printf("Ошибка загрузки VFS: неверный формат: %s\n", path);
        return 2;
    }

    VNode* loaded = (VNode*)xmalloc(sizeof(VNode));
    memset(loaded, 0, sizeof(VNode));
    loaded->name = xstrdup("");
    loaded->is_dir = 1;
    g_dirs = 0;
    g_files = 0;
    if (load_dir(loaded, path, 0) != 0) {
        node_free(loaded);
        console_printf("Ошибка загрузки VFS: неверный формат: %s\n", path);
        return 2;
    }

    node_free(g_root);
    g_root = loaded;
    free(g_physical);
    g_physical = xstrdup(path);
    console_printf("VFS загружена из \"%s\": каталогов %d, файлов %d\n", path, g_dirs, g_files);
    return 0;
}

static int is_safe_to_clear(const char* path) {
    char tmp[MAX_PATH];
    size_t n = strlen(path);
    int seps = 0;
    size_t i;
    if (n == 0 || n >= MAX_PATH)
        return 0;
    for (i = 0; i <= n; i++)
        tmp[i] = (path[i] == '/') ? '\\' : path[i];
    while (n > 1 && tmp[n - 1] == '\\')
        tmp[--n] = '\0';
    if (n < 4)
        return 0;
    if (tmp[1] == ':' && tmp[2] == '\\' && strchr(tmp + 3, '\\') == NULL)
        return 0;
    for (i = 0; i < n; i++) {
        if (tmp[i] == '\\')
            seps++;
    }
    return seps >= 3;
}

static int clear_dir(const char* dir) {
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    int rc = 0;

    if (strlen(dir) + 3 >= MAX_PATH)
        return -1;
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    do {
        char child[MAX_PATH];
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        if (strlen(dir) + 1 + strlen(fd.cFileName) + 1 >= MAX_PATH) {
            rc = -1;
            continue;
        }
        snprintf(child, sizeof(child), "%s\\%s", dir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                if (clear_dir(child) != 0)
                    rc = -1;
            }
            SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
            if (!RemoveDirectoryA(child))
                rc = -1;
        }
        else {
            SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileA(child))
                rc = -1;
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return rc;
}

void vfs_reset(void) {
    make_empty();
    vfs_use_name("default");
    console_printf("vfs-init: текущая VFS заменена на VFS по умолчанию\n");
    if (!g_physical) {
        console_printf("vfs-init: физическое представление не задано\n");
        return;
    }
    if (!is_safe_to_clear(g_physical) || clear_dir(g_physical) != 0) {
        console_printf("vfs-init: не удалось очистить физическое представление: %s\n", g_physical);
        return;
    }
    console_printf("vfs-init: физическое представление очищено: %s\n", g_physical);
}
