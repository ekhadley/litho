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
    printf("\nLoaded image '%s' of shape (%d, %d, %d)", fname, img.height, img.width, img.channels);
    
    LithoOptions opts = defaultLithoOptions();
    Obj litho = makeLithoObj(img, opts);
    printf("\nCreated lithophane object with %d vertices and %d faces", litho.n_verts, litho.n_faces);

    saveObj(litho, "litho.obj");
    printf("\nSaved lithophane obj file.");
}