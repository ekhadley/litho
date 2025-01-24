#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "geometry.c"


// ANSI color codes
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

// Get absolute path, with cross-platform support
char* get_absolute_path(const char* path) {
    char* abs_path = (char*)malloc(PATH_MAX);
    #ifdef _WIN32
        // Windows version
        if (GetFullPathNameA(path, PATH_MAX, abs_path, NULL) == 0) {
            free(abs_path);
            return strdup(path);  // Return original on error
        }
    #else
        // Unix version
        if (realpath(path, abs_path) == NULL) {
            free(abs_path);
            return strdup(path);  // Return original on error
        }
    #endif
    return abs_path;
}

void print_usage() {
    LithoOptions defaults = defaultLithoOptions();
    printf("%s%sUsage:%s litho <input_image> [options]\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("\n%sOptions:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s-o, --output%s <file>         Output file name (default: %slitho.obj%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %s--has_frame%s                 Add a frame (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.has_frame, COLOR_RESET);
    printf("  %s--bevel_corners%s             Bevel the frame corners (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.bevel_corners, COLOR_RESET);
    printf("  %s--pixels_per_vertex%s <n>     Number of pixels per vertex (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.pixels_per_vertex, COLOR_RESET);
    printf("  %s--min_thickness%s <mm>        Minimum thickness in mm (default: %s%.1f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.min_thickness, COLOR_RESET);
    printf("  %s--max_thickness%s <mm>        Maximum thickness in mm (default: %s%.1f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.max_thickness, COLOR_RESET);
    printf("  %s--bright_scale%s <n>          Brightness scaling factor (default: %s%.3f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.bright_scale, COLOR_RESET);
    printf("  %s--frame_thickness%s <mm>      Frame thickness in mm (default: %s%.1f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.frame_thickness, COLOR_RESET);
    printf("  %s--frame_angle%s <degrees>     Frame bevel angle in degrees (default: %s%.1f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.frame_angle, COLOR_RESET);
    printf("  %s--frame_width%s <mm>          Frame width in mm (default: %s%.1f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.frame_width, COLOR_RESET);
    printf("  %s--scale%s <n>                 Overall scale factor (default: %s%.2f%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.scale, COLOR_RESET);
    printf("  %s--flip_x%s                    Flip along X axis (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.flip_x, COLOR_RESET);
    printf("  %s--flip_y%s                    Flip along Y axis (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.flip_y, COLOR_RESET);
    printf("  %s--flip_z%s                    Flip along Z axis (default: %s%d%s)\n", COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, defaults.flip_z, COLOR_RESET);
    printf("\n%sExample:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  litho %simage.png%s %s--bevel_corners --frame_width=25%s -o %soutput.obj%s\n", 
           COLOR_MAGENTA, COLOR_RESET, COLOR_GREEN, COLOR_RESET, COLOR_MAGENTA, COLOR_RESET);
}

// Helper function to get option value, either from next argument or after '='
const char* get_option_value(const char* arg, int* should_increment_i) {
    char* equals_pos = strchr(arg, '=');
    if (equals_pos) {
        *should_increment_i = 0;
        return equals_pos + 1;
    }
    *should_increment_i = 1;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = "litho.obj";
    LithoOptions opts = defaultLithoOptions();

    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        int should_increment_i = 0;
        const char* value = get_option_value(argv[i], &should_increment_i);
        
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--has_frame") == 0) {
            opts.has_frame = 1;
        } else if (strcmp(argv[i], "--bevel_corners") == 0) {
            opts.bevel_corners = 1;
        } else if (strncmp(argv[i], "--pixels_per_vertex", 18) == 0) {
            if (value || (i + 1 < argc)) {
                opts.pixels_per_vertex = atoi(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--min_thickness", 14) == 0) {
            if (value || (i + 1 < argc)) {
                opts.min_thickness = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--max_thickness", 14) == 0) {
            if (value || (i + 1 < argc)) {
                opts.max_thickness = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--bright_scale", 13) == 0) {
            if (value || (i + 1 < argc)) {
                opts.bright_scale = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--frame_thickness", 16) == 0) {
            if (value || (i + 1 < argc)) {
                opts.frame_thickness = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--frame_angle", 12) == 0) {
            if (value || (i + 1 < argc)) {
                opts.frame_angle = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--frame_width", 12) == 0) {
            if (value || (i + 1 < argc)) {
                opts.frame_width = atof(value ? value : argv[++i]);
            }
        } else if (strncmp(argv[i], "--scale", 7) == 0) {
            if (value || (i + 1 < argc)) {
                opts.scale = atof(value ? value : argv[++i]);
            }
        } else if (strcmp(argv[i], "--flip_x") == 0) {
            opts.flip_x = 1;
        } else if (strcmp(argv[i], "--flip_y") == 0) {
            opts.flip_y = 1;
        } else if (strcmp(argv[i], "--flip_z") == 0) {
            opts.flip_z = 1;
        } else {
            printf("%sUnknown option:%s %s\n", COLOR_RED, COLOR_RESET, argv[i]);
            print_usage();
            return 1;
        }
    }

    // Get absolute paths
    char* abs_input_path = get_absolute_path(input_file);
    char* abs_output_path = get_absolute_path(output_file);

    // Load and process image
    Image img = loadInputImage(abs_input_path);
    if(img.img == NULL) {
        printf("%sError:%s Failed to load image: '%s%s%s'\n", COLOR_RED, COLOR_RESET, COLOR_YELLOW, abs_input_path, COLOR_RESET);
        free(abs_input_path);
        free(abs_output_path);
        return 1;
    }
    printf("%sLoaded image%s '%s%s%s' of shape (%s%d%s, %s%d%s, %s%d%s)\n", 
           COLOR_GREEN, COLOR_RESET, 
           COLOR_YELLOW, abs_input_path, COLOR_RESET,
           COLOR_CYAN, img.height, COLOR_RESET,
           COLOR_CYAN, img.width, COLOR_RESET,
           COLOR_CYAN, img.channels, COLOR_RESET);
    
    Obj litho = makeLithoObj(img, opts);
    printf("%sCreated lithophane%s with %s%d%s vertices and %s%d%s faces\n", 
           COLOR_GREEN, COLOR_RESET,
           COLOR_CYAN, litho.n_verts, COLOR_RESET,
           COLOR_CYAN, litho.n_faces, COLOR_RESET);

    saveObj(litho, abs_output_path, argc, argv);
    printf("%sSaved lithophane%s to: '%s%s%s'\n", 
           COLOR_GREEN, COLOR_RESET,
           COLOR_YELLOW, abs_output_path, COLOR_RESET);

    // Clean up
    free(abs_input_path);
    free(abs_output_path);
    free(litho.verts);
    free(litho.faces);
    return 0;
}
