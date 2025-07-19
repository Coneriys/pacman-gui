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
AURHelper detect_aur_helper(void);
void set_aur_helper(AURHelper helper);

#endif