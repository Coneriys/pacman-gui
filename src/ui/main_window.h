#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk-4.0/gtk/gtk.h>
#include "../pacman_wrapper.h"
#include "dependency_viewer.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *search_entry;
    GtkWidget *package_list;
    GtkWidget *install_btn;
    GtkWidget *remove_btn;
    GtkWidget *update_btn;
    GtkWidget *deps_btn;
    GtkWidget *aur_search_btn;
    GtkWidget *status_label;

    // Log window widgets
    GtkWidget *log_window;
    GtkWidget *log_textview;
    GtkTextBuffer *log_buffer;
    GtkWidget *close_log_btn;

    // Source selection
    GtkWidget *source_combo;

    PackageList *current_packages;
    char *selected_package;
    gboolean operation_in_progress;
    DependencyViewer *dep_viewer;
} MainWindow;

MainWindow* main_window_new(void);
void main_window_show(MainWindow *win);
void main_window_free(MainWindow *win);

#endif