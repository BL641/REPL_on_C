#ifndef VFS_H
#define VFS_H

/* Узел дерева VFS. Все команды работают с этой копией в памяти. */
typedef struct VNode {
    char* name;
    int is_dir;
    char* data;
    size_t data_len;
    struct VNode* parent;
    struct VNode* child;
    struct VNode* next;
} VNode;

/* Имя, которое печатается в приглашении. */
const char* vfs_name(void);
void vfs_use_name(const char* name);

/* 0 — каталог прочитан в память.
   1 — путь не найден.
   2 — это не каталог или его не удалось прочитать. */
int vfs_load(const char* path);

/* Пустая VFS с именем default и очистка каталога на диске. */
void vfs_reset(void);

VNode* vfs_root(void);

#endif
