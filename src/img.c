#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

typedef struct {
    int width;
    int height;
    int channels;
    unsigned char* img;
} Image;

Image load_input_image(const char* filename) {
    int width, height, channels;
    unsigned char *img = stbi_load(filename, &width, &height, &channels, 0);
    return (Image){.width = width, .height = height, .channels = channels, .img = img};
}