#include "pacman_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static AURHelper current_aur_helper = AUR_HELPER_NONE;

typedef struct {
    LogCallback callback;
    gpointer user_data;
    int pipe_fd;
    GIOChannel *channel;
    guint watch_id;
    pid_t child_pid;
} AsyncOperation;

typedef struct {
    PackageListCallback callback;
    gpointer user_data;
} AsyncPackageLoadData;

typedef struct {
    PackageListCallback callback;
    PackageList *packages;
    gpointer user_data;
} PackageLoadResult;

AURHelper detect_aur_helper(void) {
    if (system("which yay > /dev/null 2>&1") == 0) {
        return AUR_HELPER_YAY;
    } else if (system("which paru > /dev/null 2>&1") == 0) {
        return AUR_HELPER_PARU;
    }
    return AUR_HELPER_NONE;
}

void set_aur_helper(AURHelper helper) {
    current_aur_helper = helper;
}

static char* run_command(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    char *output = malloc(4096);
    char *result = malloc(1);
    result[0] = '\0';

    while (fgets(output, 4096, fp)) {
        result = realloc(result, strlen(result) + strlen(output) + 1);
        strcat(result, output);
    }

    pclose(fp);
    free(output);
    return result;
}

static gboolean read_pipe_data(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    AsyncOperation *op = (AsyncOperation*)user_data;

    if (condition & G_IO_IN) {
        gchar *line;
        gsize length;
        GError *error = NULL;

        GIOStatus status = g_io_channel_read_line(channel, &line, &length, NULL, &error);

        if (status == G_IO_STATUS_NORMAL) {
            if (length > 0 && line[length-1] == '\n') {
                line[length-1] = '\0';
            }

            if (op->callback) {
                op->callback(line, op->user_data);
            }

            g_free(line);
            return TRUE;
        }
    }

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        // Pipe closed - check child process status
        int status;
        waitpid(op->child_pid, &status, 0);

        // Send operation completion signal
        if (op->callback) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                op->callback("=== Operation completed successfully ===", op->user_data);
            } else {
                op->callback("=== Operation failed ===", op->user_data);
            }

            // Special operation end marker
            op->callback("__OPERATION_FINISHED__", op->user_data);
        }

        g_io_channel_unref(channel);
        close(op->pipe_fd);
        g_free(op);
        return FALSE;
    }

    return TRUE;
}

static gboolean run_command_async(const char *cmd, LogCallback callback, gpointer user_data) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        return FALSE;
    }

    pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return FALSE;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else {
        // Parent process
        close(pipefd[1]);

        AsyncOperation *op = g_malloc(sizeof(AsyncOperation));
        op->callback = callback;
        op->user_data = user_data;
        op->pipe_fd = pipefd[0];
        op->child_pid = pid;

        op->channel = g_io_channel_unix_new(pipefd[0]);
        g_io_channel_set_flags(op->channel, G_IO_FLAG_NONBLOCK, NULL);

        op->watch_id = g_io_add_watch(op->channel,
                                      G_IO_IN | G_IO_HUP | G_IO_ERR,
                                      read_pipe_data, op);

        return TRUE;
    }
}

PackageList* pacman_search(const char *query) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pacman -Ss %s", query);

    char *output = run_command(cmd);
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        if (strstr(line, "/")) {
            Package *pkg = &list->packages[list->count];

            char *slash = strchr(line, '/');
            char *space = strchr(slash, ' ');

            pkg->repository = strndup(line, slash - line);
            pkg->name = strndup(slash + 1, space - slash - 1);
            pkg->version = strdup(space + 1);

            line = strtok(NULL, "\n");
            if (line && line[0] == ' ') {
                pkg->description = strdup(line + 4);
            }

            pkg->installed = FALSE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

PackageList* aur_search(const char *query) {
    if (current_aur_helper == AUR_HELPER_NONE) {
        current_aur_helper = detect_aur_helper();
    }

    char cmd[512];
    if (current_aur_helper == AUR_HELPER_YAY) {
        snprintf(cmd, sizeof(cmd), "yay -Ss %s", query);
    } else if (current_aur_helper == AUR_HELPER_PARU) {
        snprintf(cmd, sizeof(cmd), "paru -Ss %s", query);
    } else {
        return NULL; // No AUR helper available
    }

    char *output = run_command(cmd);
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        if (strstr(line, "/")) {
            Package *pkg = &list->packages[list->count];

            char *slash = strchr(line, '/');
            char *space = strchr(slash, ' ');

            pkg->repository = strndup(line, slash - line);
            pkg->name = strndup(slash + 1, space - slash - 1);
            pkg->version = strdup(space + 1);

            line = strtok(NULL, "\n");
            if (line && line[0] == ' ') {
                pkg->description = strdup(line + 4);
            }

            pkg->installed = FALSE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

gboolean pacman_install_async(const char *package_name, LogCallback callback, gpointer user_data) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pkexec pacman -S --noconfirm %s", package_name);

    return run_command_async(cmd, callback, user_data);
}

gboolean aur_install_async(const char *package_name, LogCallback callback, gpointer user_data) {
    if (current_aur_helper == AUR_HELPER_NONE) {
        current_aur_helper = detect_aur_helper();
    }

    char cmd[512];
    if (current_aur_helper == AUR_HELPER_YAY) {
        snprintf(cmd, sizeof(cmd), "yay -S --noconfirm %s", package_name);
    } else if (current_aur_helper == AUR_HELPER_PARU) {
        snprintf(cmd, sizeof(cmd), "paru -S --noconfirm %s", package_name);
    } else {
        return FALSE;
    }

    return run_command_async(cmd, callback, user_data);
}

gboolean pacman_remove_async(const char *package_name, LogCallback callback, gpointer user_data) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pkexec pacman -R --noconfirm %s", package_name);

    return run_command_async(cmd, callback, user_data);
}

gboolean pacman_update_system_async(LogCallback callback, gpointer user_data) {
    return run_command_async("pkexec pacman -Syu --noconfirm", callback, user_data);
}

PackageList* pacman_list_installed(void) {
    char *output = run_command("pacman -Q");
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 1000);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 1000) {
        char *space = strchr(line, ' ');
        if (space) {
            Package *pkg = &list->packages[list->count];
            pkg->name = strndup(line, space - line);
            pkg->version = strdup(space + 1);
            pkg->repository = strdup("local");
            
            // Get detailed package info including description
            char info_cmd[256];
            snprintf(info_cmd, sizeof(info_cmd), "pacman -Qi %s 2>/dev/null | grep 'Description' | cut -d ':' -f2-", pkg->name);
            char *desc_output = run_command(info_cmd);
            if (desc_output && strlen(desc_output) > 1) {
                // Remove leading whitespace and newline
                char *desc_start = desc_output;
                while (*desc_start == ' ' || *desc_start == '\t') desc_start++;
                char *desc_end = strchr(desc_start, '\n');
                if (desc_end) *desc_end = '\0';
                pkg->description = strdup(desc_start);
                free(desc_output);
            } else {
                pkg->description = strdup("No description available");
                if (desc_output) free(desc_output);
            }
            
            pkg->installed = TRUE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

PackageList* pacman_list_updates(void) {
    char *output = run_command("pacman -Qu");
    if (!output) return NULL;

    PackageList *list = malloc(sizeof(PackageList));
    list->packages = malloc(sizeof(Package) * 100);
    list->count = 0;

    char *line = strtok(output, "\n");
    while (line && list->count < 100) {
        char *space = strchr(line, ' ');
        if (space) {
            Package *pkg = &list->packages[list->count];
            pkg->name = strndup(line, space - line);
            pkg->version = strdup(space + 1);
            pkg->repository = strdup("local");
            pkg->description = strdup("Update available");
            pkg->installed = TRUE;
            list->count++;
        }
        line = strtok(NULL, "\n");
    }

    free(output);
    return list;
}

void package_list_free(PackageList *list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->packages[i].name);
        free(list->packages[i].version);
        free(list->packages[i].description);
        free(list->packages[i].repository);
    }

    free(list->packages);
    free(list);
}

DependencyList* pacman_get_dependencies(const char *package_name) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pacman -Qi %s 2>/dev/null | grep 'Depends On' | cut -d ':' -f2", package_name);
    
    char *output = run_command(cmd);
    if (!output) return NULL;
    
    DependencyList *list = malloc(sizeof(DependencyList));
    list->dependencies = malloc(sizeof(char*) * 50);
    list->count = 0;
    
    char *token = strtok(output, " \t\n");
    while (token && list->count < 50) {
        if (strcmp(token, "None") != 0) {
            char *dep_name = strdup(token);
            char *version_pos = strchr(dep_name, '>');
            if (!version_pos) version_pos = strchr(dep_name, '<');
            if (!version_pos) version_pos = strchr(dep_name, '=');
            if (version_pos) *version_pos = '\0';
            
            list->dependencies[list->count++] = dep_name;
        }
        token = strtok(NULL, " \t\n");
    }
    
    free(output);
    return list;
}

DependencyList* pacman_get_required_by(const char *package_name) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "pacman -Qi %s 2>/dev/null | grep 'Required By' | cut -d ':' -f2", package_name);
    
    char *output = run_command(cmd);
    if (!output) return NULL;
    
    DependencyList *list = malloc(sizeof(DependencyList));
    list->dependencies = malloc(sizeof(char*) * 50);
    list->count = 0;
    
    char *token = strtok(output, " \t\n");
    while (token && list->count < 50) {
        if (strcmp(token, "None") != 0) {
            list->dependencies[list->count++] = strdup(token);
        }
        token = strtok(NULL, " \t\n");
    }
    
    free(output);
    return list;
}

static DependencyNode* find_or_create_node(DependencyTree *tree, const char *name, int depth) {
    for (int i = 0; i < tree->count; i++) {
        if (strcmp(tree->nodes[i].name, name) == 0) {
            if (depth < tree->nodes[i].depth) {
                tree->nodes[i].depth = depth;
            }
            return &tree->nodes[i];
        }
    }
    
    if (tree->count >= tree->capacity) {
        tree->capacity *= 2;
        tree->nodes = realloc(tree->nodes, sizeof(DependencyNode) * tree->capacity);
    }
    
    DependencyNode *node = &tree->nodes[tree->count++];
    node->name = strdup(name);
    node->depends = NULL;
    node->required_by = NULL;
    node->depth = depth;
    
    return node;
}

static void build_tree_recursive(DependencyTree *tree, const char *package_name, int current_depth, int max_depth) {
    if (current_depth > max_depth) return;
    
    DependencyNode *node = find_or_create_node(tree, package_name, current_depth);
    
    if (!node->depends) {
        node->depends = pacman_get_dependencies(package_name);
        if (node->depends) {
            for (int i = 0; i < node->depends->count; i++) {
                build_tree_recursive(tree, node->depends->dependencies[i], current_depth + 1, max_depth);
            }
        }
    }
    
    if (!node->required_by) {
        node->required_by = pacman_get_required_by(package_name);
    }
}

DependencyTree* pacman_build_dependency_tree(const char *package_name, int max_depth) {
    DependencyTree *tree = malloc(sizeof(DependencyTree));
    tree->nodes = malloc(sizeof(DependencyNode) * 100);
    tree->count = 0;
    tree->capacity = 100;
    
    build_tree_recursive(tree, package_name, 0, max_depth);
    
    return tree;
}

void dependency_list_free(DependencyList *list) {
    if (!list) return;
    
    for (int i = 0; i < list->count; i++) {
        free(list->dependencies[i]);
    }
    free(list->dependencies);
    free(list);
}

void dependency_tree_free(DependencyTree *tree) {
    if (!tree) return;
    
    for (int i = 0; i < tree->count; i++) {
        free(tree->nodes[i].name);
        dependency_list_free(tree->nodes[i].depends);
        dependency_list_free(tree->nodes[i].required_by);
    }
    
    free(tree->nodes);
    free(tree);
}

gboolean pacman_clean_cache_async(LogCallback callback, gpointer user_data) {
    return run_command_async("pkexec pacman -Sc --noconfirm", callback, user_data);
}

gboolean pacman_clean_all_cache_async(LogCallback callback, gpointer user_data) {
    return run_command_async("pkexec pacman -Scc --noconfirm", callback, user_data);
}

char* pacman_get_cache_size(void) {
    char *output = run_command("du -sh /var/cache/pacman/pkg 2>/dev/null | cut -f1");
    if (!output || strlen(output) == 0) {
        if (output) free(output);
        return strdup("Unknown");
    }
    
    char *newline = strchr(output, '\n');
    if (newline) *newline = '\0';
    
    return output;
}

static gboolean call_package_list_callback(gpointer data) {
    PackageLoadResult *result = (PackageLoadResult*)data;
    
    result->callback(result->packages, result->user_data);
    
    g_free(result);
    return FALSE; // Remove from idle queue
}

static gpointer async_load_installed_packages(gpointer data) {
    AsyncPackageLoadData *async_data = (AsyncPackageLoadData*)data;
    
    PackageList *packages = pacman_list_installed();
    
    // Create result structure for main thread callback
    PackageLoadResult *result = g_malloc(sizeof(PackageLoadResult));
    result->callback = async_data->callback;
    result->packages = packages;
    result->user_data = async_data->user_data;
    
    // Schedule the callback to be called in the main thread
    g_idle_add(call_package_list_callback, result);
    
    g_free(async_data);
    return NULL;
}

gboolean pacman_list_installed_async(PackageListCallback callback, gpointer user_data) {
    if (!callback) return FALSE;
    
    AsyncPackageLoadData *async_data = g_malloc(sizeof(AsyncPackageLoadData));
    async_data->callback = callback;
    async_data->user_data = user_data;
    
    // Start the background thread
    GThread *thread = g_thread_new("load_installed", async_load_installed_packages, async_data);
    
    if (thread) {
        g_thread_unref(thread);  // We don't need to wait for it
        return TRUE;
    }
    
    g_free(async_data);
    return FALSE;
}