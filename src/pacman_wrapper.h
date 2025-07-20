#ifndef PACMAN_WRAPPER_H
#define PACMAN_WRAPPER_H

#include <glib.h>

typedef struct {
    char *name;
    char *version;
    char *description;
    char *repository;
    gboolean installed;
} Package;

typedef struct {
    Package *packages;
    int count;
} PackageList;

typedef enum {
    AUR_HELPER_NONE,
    AUR_HELPER_YAY,
    AUR_HELPER_PARU
} AURHelper;

typedef void (*LogCallback)(const char *line, gpointer user_data);

typedef struct {
    char **dependencies;
    int count;
} DependencyList;

typedef struct {
    char *name;
    DependencyList *depends;
    DependencyList *required_by;
    int depth;
} DependencyNode;

typedef struct {
    DependencyNode *nodes;
    int count;
    int capacity;
} DependencyTree;

PackageList* pacman_search(const char *query);
PackageList* aur_search(const char *query);
gboolean pacman_install_async(const char *package_name, LogCallback callback, gpointer user_data);
gboolean pacman_remove_async(const char *package_name, LogCallback callback, gpointer user_data);
gboolean aur_install_async(const char *package_name, LogCallback callback, gpointer user_data);
PackageList* pacman_list_installed(void);
PackageList* pacman_list_updates(void);
gboolean pacman_update_system_async(LogCallback callback, gpointer user_data);

void package_list_free(PackageList *list);
char* pacman_get_package_info(const char *package_name);
DependencyList* pacman_get_dependencies(const char *package_name);
DependencyList* pacman_get_required_by(const char *package_name);
DependencyTree* pacman_build_dependency_tree(const char *package_name, int max_depth);
void dependency_list_free(DependencyList *list);
void dependency_tree_free(DependencyTree *tree);
AURHelper detect_aur_helper(void);
void set_aur_helper(AURHelper helper);

#endif