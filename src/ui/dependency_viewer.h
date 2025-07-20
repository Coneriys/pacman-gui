#ifndef DEPENDENCY_VIEWER_H
#define DEPENDENCY_VIEWER_H

#include <gtk-4.0/gtk/gtk.h>
#include "../pacman_wrapper.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *package_entry;
    GtkWidget *depth_spin;
    GtkWidget *refresh_btn;
    GtkWidget *close_btn;
    GtkWidget *status_label;
    GtkWidget *legend_box;
    
    DependencyTree *current_tree;
    char *root_package;
    int max_depth;
    
    // Drawing properties
    int node_width;
    int node_height;
    int level_spacing;
    int node_spacing;
} DependencyViewer;

DependencyViewer* dependency_viewer_new(void);
void dependency_viewer_show(DependencyViewer *viewer);
void dependency_viewer_set_package(DependencyViewer *viewer, const char *package_name);
void dependency_viewer_free(DependencyViewer *viewer);

#endif