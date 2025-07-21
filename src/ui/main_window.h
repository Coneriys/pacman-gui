#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk-4.0/gtk/gtk.h>
#include "../pacman_wrapper.h"
#include "dependency_viewer.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    
    // Search tab widgets
    GtkWidget *search_entry;
    GtkWidget *package_list;
    GtkWidget *source_combo;
    
    // Installed packages tab widgets
    GtkWidget *installed_list;
    GtkWidget *refresh_installed_btn;
    
    // Buttons (shared between tabs)
    GtkWidget *install_btn;
    GtkWidget *remove_btn;
    GtkWidget *update_btn;
    GtkWidget *deps_btn;
    GtkWidget *clean_cache_btn;
    GtkWidget *clean_all_cache_btn;
    GtkWidget *cache_size_label;
    GtkWidget *status_label;

    // Log window widgets
    GtkWidget *log_window;
    GtkWidget *log_textview;
    GtkTextBuffer *log_buffer;
    GtkWidget *close_log_btn;

    PackageList *current_packages;
    PackageList *installed_packages;
    char *selected_package;
    gboolean operation_in_progress;
    DependencyViewer *dep_viewer;
} MainWindow;

MainWindow* main_window_new(void);
void main_window_show(MainWindow *win);
void main_window_free(MainWindow *win);

#endif