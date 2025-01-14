#include <stdio.h>
#include <math.h>
#include "img.c"


typedef struct {
    const int has_frame;
    const int pixels_per_vertex;
    const float min_thickness;
    const float max_thickness;
    const float obj_height;
    const float obj_width;
    const float bright_scale;
    // need to define frame thickness, width, bevel angle, ?
    // we need to know how many pixels are in between the vertices on the image side, like the resolution of the 3d object.
    // theres no point in having fewer than vertex per pixel, but is there any cost to having exactly 1 per pixel?
    // this fixes us to maximum spatial resolution, so file sizes will be larger, but the slicer will just get rid of as
    // much detail as necessary to print so ..?
    // will test later with different v/p values to see effect on file size and possible remove if negligible.
} LithoOptions;

LithoOptions defaultLithoOptions() {
    return (LithoOptions){
        .has_frame = 1,
        .pixels_per_vertex = 1,
        .min_thickness = 0.1,
        .max_thickness = 5,
        .obj_height = 100,
        .obj_width = 100,
        .bright_scale = 1.0,
    };
}

typedef struct {
    float x;
    float y;
    float z;
} Pos;

typedef struct {
    int v1;
    int v2;
    int v3;
} Face;

typedef struct {
    Pos* verts;
    int nverts;
    Face* faces;
    int nfaces;
} Obj;

Obj initLithoObj(Image img, LithoOptions opts) {
    int vwidth = floor(img.width/opts.pixels_per_vertex); // width in vertices
    int vheight = floor(img.height/opts.pixels_per_vertex); // height in vertices
    int nverts = vwidth*vheight + 2*vwidth + 2*vheight - 4; // front side picture vertices + backside perimeter vertices
    // front image triangles, side triangles, and 2 on the back
    int nfaces = 2*((vwidth - 1) * (vheight - 1)) + 2*(vwidth - 1) + 2*(vheight - 1) + 2;
    if (opts.has_frame) {
        nverts += 8; // 8 corners of the optional frame
        nfaces += 20*2; // 20 new faces, 2 triangles each
    }
    Pos* verts = (Pos*)malloc(sizeof(Pos)*nverts);
    Face* faces = (Face*)malloc(sizeof(Face)*nfaces);
    return (Obj){ .verts = verts, .nverts = nverts, .faces = faces, .nfaces = nfaces };
}


Obj makeLithoObj(Image img, LithoOptions opts) {
    Obj obj = initLithoObj(img, opts);
    Image brightness = rgbToBrightness(img);

    int vcount = 0;
    int fcount = 0;
    int vwidth = brightness.width/opts.pixels_per_vertex;
    int vheight = brightness.height/opts.pixels_per_vertex;
    for (int y = 0; y < vheight; y += 1) {
        for (int x = 0; x < vwidth; x += 1) {
            int b = brightness.img[img.width*y*opts.pixels_per_vertex + x*opts.pixels_per_vertex];
            float h = (b/50.0)*opts.bright_scale + opts.min_thickness;
            obj.verts[vcount++] = (Pos){.x = x, .y=h, .z=y};
            if ((x != (vwidth - 1)) && (y != 0)) {
                // vertex indices start at zero for face grouping
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - vwidth, .v3=vcount - vwidth + 1}; 
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount + 1, .v3=vcount - vwidth + 1};
            }
        }
    }
    obj.nverts = vcount;
    obj.nfaces = fcount;

    
    stbi_image_free(brightness.img);
    return obj;
}
Obj makeLithoObj(Image img) { return makeLithoObj(img, defaultLithoOptions()); }

void saveObj(Obj obj, const char* filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "# Lithophane obj file made using https://github.com/ekhadley/litho\n");
    fprintf(f, "o litho\n");
    for (int i = 0; i < obj.nverts; i++) {
        fprintf(f, "v %f %f %f\n", obj.verts[i].x, obj.verts[i].y, obj.verts[i].z);
    }
    fprintf(f, "g faces\n");
    for (int i = 0; i < obj.nfaces; i++) {
        fprintf(f, "f %d %d %d\n", obj.faces[i].v1, obj.faces[i].v2, obj.faces[i].v3);
    }
}