
#define _CRT_SECURE_NO_WARNINGS
#define STD_BUFSz 256
#define STD_CMDSz 64
#define STD_ARGSz 192

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <ctype.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "advapi32.lib")

#include "VFS.h"

static int boolean = 1;

typedef struct VFS {
    const char* VFS_name;
} VFS;

typedef struct var_map {
    int key;
    const char* name;
} var_map;

typedef struct REPLComm {
    const char* command;
    void (*Chandler)(struct REPLComm* table, char* args);
} REPLComm;
    
static int first_arg(const char* args, char* dest, size_t n) {
    size_t i = 0;
    while (*args == ' ')
        args++;
    if (*args == '\0') {
        dest[0] = '\0';
        return 0;
    }
    while (*args && *args != ' ' && i + 1 < n)
        dest[i++] = *args++;
    dest[i] = '\0';
    while (*args == ' ')
        args++;
    return *args ? 2 : 1;
}

static void ls_print(VNode* node) {
    if (!node)
        return;
    ls_print(node->next);
    printf("%s\n", node->name);
}

void cmd_cd(REPLComm* this, char* args) {
    char path[STD_ARGSz];
    int n = first_arg(args, path, sizeof(path));
    int err;
    if (n == 2) {
        console_printf("cd: лишние аргументы\n");
        return;
    }
    err = vfs_cd(n == 0 ? NULL : path);
    if (err == 1)
        console_printf("cd: файл не найден: %s\n", path);
    else if (err == 2)
        console_printf("cd: это не каталог: %s\n", path);
}

void cmd_ls(REPLComm* this, char* args) {
    char path[STD_ARGSz];
    VNode* node = NULL;
    int n = first_arg(args, path, sizeof(path));
    int err;
    if (n == 2) {
        console_printf("ls: лишние аргументы\n");
        return;
    }
    if (n == 0)
        node = vfs_cwd();
    else {
        err = vfs_resolve(path, &node);
        if (err == 1) {
            console_printf("ls: файл не найден: %s\n", path);
            return;
        }
        if (err == 2) {
            console_printf("ls: это не каталог: %s\n", path);
            return;
        }
    }
    if (!node->is_dir)
        printf("%s\n", node->name);
    else
        ls_print(node->child);
}

void cmd_echo(REPLComm* this, char* args) {
    if (strlen(args) == 0) {
        console_printf("Вывод на консоль был включен\n");
    }
    else {
        printf("%s\n", args);
    }
}

void cmd_exit(REPLComm* this, char* args) {
    boolean = 0;
}

void cmd_vfs_init(REPLComm* this, char* args) {
    if (args[0] != '\0') {
        console_printf("vfs-init: лишние аргументы\n");
        return;
    }
    vfs_reset();
}

void cmd_whoami(REPLComm* this, char* args) {
    char name[256];
    DWORD size = sizeof(name);
    if (args[0] != '\0') {
        console_printf("whoami: лишние аргументы\n");
        return;
    }
    if (!GetUserNameA(name, &size))
        snprintf(name, sizeof(name), "user");
    printf("%s\n", name);
}

static void tac_write(const char* data, size_t len) {
    size_t pos;
    if (!data || len == 0)
        return;
    pos = len;
    while (pos > 0) {
        size_t end = pos;
        size_t start;
        if (data[end - 1] == '\n')
            end--;
        if (end > 0 && data[end - 1] == '\r')
            end--;
        start = end;
        while (start > 0 && data[start - 1] != '\n')
            start--;
        fwrite(data + start, 1, end - start, stdout);
        fputc('\n', stdout);
        if (start == 0)
            break;
        pos = start;
    }
}

void cmd_tac(REPLComm* this, char* args) {
    char buf[STD_ARGSz];
    char* tok;
    if (args[0] == '\0') {
        console_printf("tac: не указан файл\n");
        return;
    }
    snprintf(buf, sizeof(buf), "%s", args);
    tok = strtok(buf, " ");
    while (tok) {
        VNode* node = NULL;
        int err = vfs_resolve(tok, &node);
        if (err == 1)
            console_printf("tac: файл не найден: %s\n", tok);
        else if (err == 2 || node->is_dir)
            console_printf("tac: это каталог: %s\n", tok);
        else
            tac_write(node->data, node->data_len);
        tok = strtok(NULL, " ");
    }
}

void cmd_touch(REPLComm* this, char* args) {
    char buf[STD_ARGSz];
    char* tok;
    if (args[0] == '\0') {
        console_printf("touch: не указан файл\n");
        return;
    }
    snprintf(buf, sizeof(buf), "%s", args);
    tok = strtok(buf, " ");
    while (tok) {
        int err = vfs_touch(tok);
        if (err == 1)
            console_printf("touch: файл не найден: %s\n", tok);
        else if (err == 2)
            console_printf("touch: это каталог: %s\n", tok);
        else if (err == 3)
            console_printf("touch: это не каталог: %s\n", tok);
        tok = strtok(NULL, " ");
    }
}

void execute_com(REPLComm* table, int count, const char* user_cmd, char* user_args) {
    for (int i = 0; i < count; i++) {
        if (strcmp(table[i].command, user_cmd) == 0) {
            table[i].Chandler(&table[i], user_args);
            return;
        }
    }

    printf("Undefined command: %s\n", user_cmd);
}

char* new_userVars(char* user_args) {
    return user_args;
}

void load_config(const char* file_name, char* vfs_out, char* script_out) {
    vfs_out[0] = '\0';
    script_out[0] = '\0';
    FILE* f = fopen(file_name, "r");
    if (!f) {
        console_printf("Не открыть файл\n");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        char* p = line;

        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#') continue;

        char* colon = strchr(p, ':');
        if (!colon) continue;

        *colon = '\0';
        char* key = p;
        char* value = colon + 1;
        while (*value == ' ' || *value == '\t') value++;

        if (strcmp(key, "vfs") == 0)
            snprintf(vfs_out, 256, "%s", value);
        else if (strcmp(key, "script") == 0)
            snprintf(script_out, 256, "%s", value);
    }
    fclose(f);
}

void handle_line(char* line, REPLComm* table, int count);
void run_script(const char* file_name, REPLComm* table, int count, const char* VFS_name) {
    FILE* scr_file = fopen(file_name, "r");
    char buff[256];
    if (!scr_file) {
        console_printf("Не удалось прочитать файл\n");
        boolean = 0;
        return;
    }
    while (fgets(buff, sizeof(buff), scr_file)) {
        buff[strcspn(buff, "\r\n")] = '\0';
        printf("vfs:%s> %s\n", VFS_name, buff);
        handle_line(buff, table, count);
        if (!boolean)
            break;
    }
    fclose(scr_file);
}

void expand_vars(char* dist, char* source, size_t size) {
    size_t w = 0;
    for (int i = 0; source[i] && w < size - 1; i++) {
        if (source[i] == '$') {
            char var[STD_ARGSz] = { 0 };
            int k = 0;
            i++;
            while (isalnum(source[i]) || source[i] == '_') {
                var[k++] = source[i++];
            }
            i--;

            char* value = getenv(var);
            if (value) {
                while (*value && w < size - 1) {
                    dist[w++] = *value++;
                }
            }

        }
        else {
            dist[w++] = source[i];
        }

    }
    dist[w] = '\0';
}

void handle_line(char* line, REPLComm* table, int count) {
    char expand[STD_CMDSz];
    char cmd[STD_CMDSz];
    char args[STD_ARGSz];
    memset(cmd, 0, sizeof(cmd));
    memset(args, 0, sizeof(args));
    memset(expand, 0, sizeof(expand));
    expand_vars(expand, line, STD_CMDSz);
    int pos = 0;
    char* token = strtok(expand, " ");
    while (token) {
        if (pos == 0) {
            strcat(cmd, token);
            pos++;
        }
        else {
            if (*args != '\0') strcat(args, " ");
            strcat(args, token);
            pos++;
        }
        token = strtok(NULL, " ");
    }
    if (cmd[0] == '\0')
        return;
    execute_com(table, count, cmd, args);
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Russian");
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    const char* vfs_path = NULL;
    const char* script_path = NULL;
    const char* config_path = NULL;

    char vfs_buf[256] = "";
    char script_buf[256] = "";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vfs") == 0 && i + 1 < argc)
            vfs_path = argv[++i];
        else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc)
            script_path = argv[++i];
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            config_path = argv[++i];
        else {
            console_printf("Неизвестные аргументы.\n");
        }
    }
    REPLComm comm_dict[] = {
        {"cd", cmd_cd},
        {"ls", cmd_ls},
        {"echo", cmd_echo},
        {"whoami", cmd_whoami},
        {"tac", cmd_tac},
        {"touch", cmd_touch},
        {"exit", cmd_exit},
        {"vfs-init", cmd_vfs_init}
    };
    if (config_path)
        load_config(config_path, vfs_buf, script_buf);
    if (vfs_buf[0] != '\0')
        vfs_path = vfs_buf;
    if (script_buf[0] != '\0')
        script_path = script_buf;

    vfs_use_name(vfs_path ? vfs_path : "default");
    if (vfs_path)
        vfs_load(vfs_path);

    VFS vfsys;
    vfsys.VFS_name = vfs_name();

    if (script_path)
        run_script(script_path, comm_dict, sizeof(comm_dict) / sizeof(comm_dict[0]), vfsys.VFS_name);

    printf("[DEBUG] VFS path: %s\n", vfs_path ? vfs_path : "(not set)");
    printf("[DEBUG] Script path: %s\n", script_path ? script_path : "(not set)");

    char Buffer[STD_CMDSz];
    while (boolean) {
        printf("vfs:%s> ", vfsys.VFS_name);

        if (fgets(Buffer, STD_CMDSz, stdin) == NULL)
            break;

        Buffer[strcspn(Buffer, "\n")] = '\0';

        handle_line(Buffer, comm_dict, sizeof(comm_dict) / sizeof(comm_dict[0]));
    }
}