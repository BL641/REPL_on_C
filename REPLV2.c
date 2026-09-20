
#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <ctype.h>

static int boolean = 1;

typedef struct VFS {
    const char* VFS_name;
} VFS;

typedef struct var_map {
    const char* name;
    int key;
};


typedef struct REPLComm {
    const char* command;
    void (*Chandler)(struct ReplComm* table, char* args);
} REPLComm;


void cmd_cd(REPLComm* this, char* args) {
    printf("cd: args = '%s'\n", args);
}

void cmd_ls(REPLComm* this, char* args) {
    printf("ls: args = '%s'\n", args);
}

void cmd_echo(REPLComm* this, char* args) {
    if (strlen(args) == 0) {
        printf("Вывод на консоль был включен\n");
    }
    else {
        printf("%s\n", args);
    }
}

void cmd_exit(REPLComm* this, char* args) {
    boolean = 0;
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

void expand_vars(char* dist, char* source, size_t size) {
    size_t w = 0;
    for (int i = 0; source[i] && w < size - 1; i++) {
        if (source[i] == '$') {

            char var[64] = { 0 };
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

int main(int argc, char** argv) {
    setlocale(LC_ALL, "Russian");

    VFS vfsys;
    vfsys.VFS_name = "default";

    REPLComm comm_dict[] = {
        {"cd", cmd_cd},
        {"ls", cmd_ls},
        {"echo", cmd_echo},
        {"exit", cmd_exit}
    };

    char expand[256];
    char Buffer[256];
    char cmd[64];
    char args[192];

    while (boolean) {
        memset(cmd, 0, 64);
        memset(args, 0, 192);
        memset(expand, 0, sizeof(expand));
        printf("vfs:%s> ", vfsys.VFS_name);
        if (fgets(Buffer, 256, stdin) == NULL) {
            break;
        } 
        Buffer[strcspn(Buffer, "\n")] = '\0';

        expand_vars(expand, Buffer, 256);
        
        int pos = 0;
        char* token = strtok(expand, " ");
        while (token) {
            int len = strlen(token);
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
        execute_com(comm_dict, 4, cmd, args);
        
    }
}