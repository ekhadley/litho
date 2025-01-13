#include <stdio.h>
#include "img.c"


int main(void) {
    const char* fname = "golem.png";
    Image img = load_input_image(fname);
    if(img.img == NULL) {
        printf("Failed to load image: '%d'\n", fname);
        exit(1);
    }
    printf("Loaded image '%s' with w: %dpx, h: %dpx and %d channels\n", fname, img.width, img.height, img.channels);
}