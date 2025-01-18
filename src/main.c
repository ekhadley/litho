#include <stdio.h>
#include <math.h>
#include "geometry.c"


int main(void) {
    const char *fname = "dorm.png";
    Image img = loadInputImage(fname);
    if(img.img == NULL) {
        printf("Failed to load image: '%s'\n", fname);
        exit(1);
    }
    printf("Loaded image '%s' of shape (%d, %d, %d)\n", fname, img.height, img.width, img.channels);
    
    LithoOptions opts = defaultLithoOptions();
    opts.has_frame = 1;
    Obj litho = makeLithoObj(img, opts);
    printf("\nCreated lithophane object with %d vertices and %d faces", litho.nverts, litho.nfaces);

    saveObj(litho, "litho.obj");
    printf("\nSaved lithophane obj file.");
}