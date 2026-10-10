#ifndef VFS_H
#define VFS_H

/* Печать в консоль Windows без зависимости от кодировки окна. */
void console_printf(const char* fmt, ...);

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

/* Текущий каталог. После загрузки и vfs-init это корень. */
VNode* vfs_cwd(void);

/* Найти узел по пути. Путь от корня начинается с / или \.
   Пустая строка — текущий каталог. . и .. работают как в UNIX.
   0 — найден, 1 — нет такого имени, 2 — по пути встретился файл. */
int vfs_resolve(const char* path, VNode** out);

/* Сменить текущий каталог. Без пути — в корень.
   0 — получилось, 1 — не найден, 2 — это не каталог. */
int vfs_cd(const char* path);

/* Создать пустой файл в памяти или оставить уже существующий.
   На диск ничего не пишется.
   0 — файл есть, 1 — нет каталога по пути,
   2 — это каталог, 3 — по пути встретился файл. */
int vfs_touch(const char* path);

#endif
