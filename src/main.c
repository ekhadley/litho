#include <stdio.h>
#include "img.c"


int main(void) {
    int width, height, channels;
    unsigned char *img = stbi_load("golem.png", &width, &height, &channels, 0);
    if(img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
}