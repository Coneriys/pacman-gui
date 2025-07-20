#include "dependency_viewer.h"
#include <math.h>

static void draw_dependency_graph(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
    DependencyViewer *viewer = (DependencyViewer*)user_data;
    
    if (!viewer->current_tree || viewer->current_tree->count == 0) {
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, width/2 - 100, height/2);
        cairo_show_text(cr, "No dependency data to display");
        return;
    }
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);
    
    int levels[10] = {0};
    for (int i = 0; i < viewer->current_tree->count; i++) {
        if (viewer->current_tree->nodes[i].depth < 10) {
            levels[viewer->current_tree->nodes[i].depth]++;
        }
    }
    
    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    
    for (int i = 0; i < viewer->current_tree->count; i++) {
        DependencyNode *node = &viewer->current_tree->nodes[i];
        if (node->depth >= 10) continue;
        
        int level_count = levels[node->depth];
        int node_index = 0;
        for (int j = 0; j < i; j++) {
            if (viewer->current_tree->nodes[j].depth == node->depth) {
                node_index++;
            }
        }
        
        int x = 50 + node->depth * viewer->level_spacing;
        int y = 50 + node_index * viewer->node_spacing + (height - level_count * viewer->node_spacing) / 2;
        
        if (node->depth == 0) {
            cairo_set_source_rgb(cr, 0.2, 0.8, 0.2);
        } else {
            cairo_set_source_rgb(cr, 0.7, 0.9, 1.0);
        }
        
        cairo_rectangle(cr, x, y, viewer->node_width, viewer->node_height);
        cairo_fill(cr);
        
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_rectangle(cr, x, y, viewer->node_width, viewer->node_height);
        cairo_stroke(cr);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, node->name, &extents);
        
        int text_x = x + (viewer->node_width - extents.width) / 2;
        int text_y = y + viewer->node_height / 2 + extents.height / 2;
        
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, node->name);
        
        if (node->depends) {
            for (int j = 0; j < node->depends->count; j++) {
                for (int k = 0; k < viewer->current_tree->count; k++) {
                    DependencyNode *dep_node = &viewer->current_tree->nodes[k];
                    if (strcmp(dep_node->name, node->depends->dependencies[j]) == 0) {
                        int dep_level_count = levels[dep_node->depth];
                        int dep_node_index = 0;
                        for (int l = 0; l < k; l++) {
                            if (viewer->current_tree->nodes[l].depth == dep_node->depth) {
                                dep_node_index++;
                            }
                        }
                        
                        int dep_x = 50 + dep_node->depth * viewer->level_spacing;
                        int dep_y = 50 + dep_node_index * viewer->node_spacing + (height - dep_level_count * viewer->node_spacing) / 2;
                        
                        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
                        cairo_set_line_width(cr, 1.0);
                        cairo_move_to(cr, x + viewer->node_width, y + viewer->node_height / 2);
                        cairo_line_to(cr, dep_x, dep_y + viewer->node_height / 2);
                        cairo_stroke(cr);
                        
                        cairo_move_to(cr, dep_x - 5, dep_y + viewer->node_height / 2 - 3);
                        cairo_line_to(cr, dep_x, dep_y + viewer->node_height / 2);
                        cairo_line_to(cr, dep_x - 5, dep_y + viewer->node_height / 2 + 3);
                        cairo_stroke(cr);
                        break;
                    }
                }
            }
        }
    }
}

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
    DependencyViewer *viewer = (DependencyViewer*)user_data;
    
    const char *package_name = gtk_editable_get_text(GTK_EDITABLE(viewer->package_entry));
    if (strlen(package_name) == 0) {
        gtk_label_set_text(GTK_LABEL(viewer->status_label), "Please enter a package name");
        return;
    }
    
    int depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->depth_spin));
    
    gtk_label_set_text(GTK_LABEL(viewer->status_label), "Building dependency tree...");
    
    if (viewer->current_tree) {
        dependency_tree_free(viewer->current_tree);
    }
    
    viewer->current_tree = pacman_build_dependency_tree(package_name, depth);
    
    if (viewer->current_tree && viewer->current_tree->count > 0) {
        char status[256];
        snprintf(status, sizeof(status), "Found %d packages in dependency tree", viewer->current_tree->count);
        gtk_label_set_text(GTK_LABEL(viewer->status_label), status);
        
        free(viewer->root_package);
        viewer->root_package = strdup(package_name);
        viewer->max_depth = depth;
    } else {
        gtk_label_set_text(GTK_LABEL(viewer->status_label), "Package not found or no dependencies");
    }
    
    gtk_widget_queue_draw(viewer->drawing_area);
}

static void create_legend(DependencyViewer *viewer) {
    GtkWidget *legend_frame = gtk_frame_new("Legend");
    GtkWidget *legend_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(legend_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(legend_grid), 10);
    gtk_widget_set_margin_start(legend_grid, 10);
    gtk_widget_set_margin_end(legend_grid, 10);
    gtk_widget_set_margin_top(legend_grid, 10);
    gtk_widget_set_margin_bottom(legend_grid, 10);
    
    GtkWidget *root_color = gtk_drawing_area_new();
    gtk_widget_set_size_request(root_color, 20, 20);
    GtkWidget *root_label = gtk_label_new("Root Package");
    gtk_label_set_xalign(GTK_LABEL(root_label), 0.0);
    
    GtkWidget *dep_color = gtk_drawing_area_new();
    gtk_widget_set_size_request(dep_color, 20, 20);
    GtkWidget *dep_label = gtk_label_new("Dependencies");
    gtk_label_set_xalign(GTK_LABEL(dep_label), 0.0);
    
    GtkWidget *arrow_label = gtk_label_new("→ Dependency relationship");
    gtk_label_set_xalign(GTK_LABEL(arrow_label), 0.0);
    
    gtk_grid_attach(GTK_GRID(legend_grid), root_color, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(legend_grid), root_label, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(legend_grid), dep_color, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(legend_grid), dep_label, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(legend_grid), arrow_label, 0, 2, 2, 1);
    
    gtk_frame_set_child(GTK_FRAME(legend_frame), legend_grid);
    viewer->legend_box = legend_frame;
}

DependencyViewer* dependency_viewer_new(void) {
    DependencyViewer *viewer = malloc(sizeof(DependencyViewer));
    viewer->current_tree = NULL;
    viewer->root_package = NULL;
    viewer->max_depth = 3;
    
    viewer->node_width = 120;
    viewer->node_height = 30;
    viewer->level_spacing = 180;
    viewer->node_spacing = 50;
    
    viewer->window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(viewer->window), "Package Dependency Viewer");
    gtk_window_set_default_size(GTK_WINDOW(viewer->window), 900, 700);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    
    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *package_label = gtk_label_new("Package:");
    viewer->package_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(viewer->package_entry), "Enter package name...");
    
    GtkWidget *depth_label = gtk_label_new("Max Depth:");
    viewer->depth_spin = gtk_spin_button_new_with_range(1, 5, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(viewer->depth_spin), 3);
    
    viewer->refresh_btn = gtk_button_new_with_label("Refresh");
    g_signal_connect(viewer->refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), viewer);
    
    viewer->close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(viewer->close_btn, "clicked", G_CALLBACK(gtk_window_close), viewer->window);
    
    gtk_box_append(GTK_BOX(controls_box), package_label);
    gtk_box_append(GTK_BOX(controls_box), viewer->package_entry);
    gtk_box_append(GTK_BOX(controls_box), depth_label);
    gtk_box_append(GTK_BOX(controls_box), viewer->depth_spin);
    gtk_box_append(GTK_BOX(controls_box), viewer->refresh_btn);
    gtk_box_append(GTK_BOX(controls_box), viewer->close_btn);
    
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget *graph_frame = gtk_frame_new("Dependency Graph");
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    viewer->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(viewer->drawing_area, 800, 600);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(viewer->drawing_area), draw_dependency_graph, viewer, NULL);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), viewer->drawing_area);
    gtk_frame_set_child(GTK_FRAME(graph_frame), scrolled);
    
    create_legend(viewer);
    
    gtk_box_append(GTK_BOX(content_box), graph_frame);
    gtk_box_append(GTK_BOX(content_box), viewer->legend_box);
    
    viewer->status_label = gtk_label_new("Enter a package name and click Refresh to view dependencies");
    gtk_label_set_xalign(GTK_LABEL(viewer->status_label), 0.0);
    
    gtk_box_append(GTK_BOX(main_box), controls_box);
    gtk_box_append(GTK_BOX(main_box), content_box);
    gtk_box_append(GTK_BOX(main_box), viewer->status_label);
    
    gtk_window_set_child(GTK_WINDOW(viewer->window), main_box);
    
    return viewer;
}

void dependency_viewer_show(DependencyViewer *viewer) {
    gtk_window_present(GTK_WINDOW(viewer->window));
}

void dependency_viewer_set_package(DependencyViewer *viewer, const char *package_name) {
    gtk_editable_set_text(GTK_EDITABLE(viewer->package_entry), package_name);
    on_refresh_clicked(GTK_BUTTON(viewer->refresh_btn), viewer);
}

void dependency_viewer_free(DependencyViewer *viewer) {
    if (viewer->current_tree) dependency_tree_free(viewer->current_tree);
    if (viewer->root_package) free(viewer->root_package);
    free(viewer);
}