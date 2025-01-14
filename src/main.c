#include <stdio.h>
#include <math.h>
#include "geometry.c"


int main(void) {
    const char *fname = "dorm.png";
    Image img = load_input_image(fname);
    if(img.img == NULL) {
        printf("Failed to load image: '%s'\n", fname);
        exit(1);
    }
    printf("Loaded image '%s' of shape (%d, %d, %d)\n", fname, img.height, img.width, img.channels);
    
    Obj litho = makeLithoObj(img, defaultLithoOptions());

    saveObj(litho, "litho.obj");
}