#include <stdio.h>
#include <math.h>
#include "img.c"


typedef struct {
    const int has_frame;
    const int pixels_per_vertex;
    const float min_thickness;
    const float max_thickness;
    //const float obj_height;
    //const float obj_width;
    const float bright_scale;
    // need to define frame thickness, width, bevel angle, ?
    // we need to know how many pixels are in between the vertices on the image side, like the resolution of the 3d object.
    // theres no point in having fewer than vertex per pixel, but is there any cost to having exactly 1 per pixel?
    // this fixes us to maximum spatial resolution, so file sizes will be larger, but the slicer will just get rid of as
    // much detail as necessary to print so ..?
    // will test later with different p/v values to see effect on file size and possible remove if negligible.
} LithoOptions;

LithoOptions defaultLithoOptions() {
    return (LithoOptions){
        .has_frame = 0,
        .pixels_per_vertex = 150,
        .min_thickness = 1.0,
        .max_thickness = 5,
        //.obj_height = 100,
        //.obj_width = 100,
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
    int nverts = vwidth*vheight + 2*vwidth + 2*vheight - 4; // front side picture vertices + backside perimeter vertices (minus double counted corners)
    // front image triangles, side triangles, and 2 on the back
    int nfaces = 2*((vwidth - 1) * (vheight - 1)) + 4*(vwidth - 1) + 4*(vheight - 1) + 2;
    if (opts.has_frame) {
        nverts += 8; // 8 corners of the optional frame
        nfaces += 40; // 20 new faces, 2 triangles each
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
        for (int x = 0; x < vwidth; x += 1) { // face vertices
            int b = brightness.img[img.width*y*opts.pixels_per_vertex + x*opts.pixels_per_vertex];
            float h = (b/50.0)*opts.bright_scale + opts.min_thickness;

            obj.verts[vcount++] = (Pos){.x = x, .y=h, .z=y};
            if ((x != (vwidth - 1)) && (y != 0)) {
                // vertex indices start at zero for face grouping
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - vwidth + 1, .v3=vcount - vwidth};  // right hand rule gives the right normal
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount + 1, .v3=vcount - vwidth + 1};
            }
        }
    }
    int bx0 = vcount + 1; // this is the index (shifted by 1 cuase of obj file indexing) of the first vertex placed on the backside perimeter
    for (int x = 0; x < vwidth; x += 1) { // y-parallel back plane perimeter vertices
        obj.verts[vcount++] = (Pos){.x = x, .y=0, .z=0};
        obj.verts[vcount++] = (Pos){.x = x, .y=0, .z=vheight-1};
        if (x != 0) {
            obj.faces[fcount++] = (Face){.v1=vcount - 1, .v2=(vcount - 1) - 2, .v3=x};
            obj.faces[fcount++] = (Face){.v1=vcount - 1, .v2=x, .v3=x+1};
            
            obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - vwidth - x - 1, .v3=vcount - vwidth - x - 2};
            obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - vwidth - x - 2, .v3=vcount - 2};
        }
    }
    int by0 = vcount + 1; // above
    if (opts.has_frame == 0) {
        for (int y = 1; y < vheight-1; y += 1) {
            obj.verts[vcount++] = (Pos){.x = 0, .y=0, .z=y};
            obj.verts[vcount++] = (Pos){.x = vwidth-1, .y=0, .z=y}; // cringe variable naming bro
            if (y != 1) {
                obj.faces[fcount++] = (Face){.v1=vcount - 1, .v2=y*vwidth + 1, .v3=(y-1)*vwidth + 1};
                obj.faces[fcount++] = (Face){.v1=vcount - 1, .v2=(y-1)*vwidth + 1, .v3=(vcount - 1) - 2};
                
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=y*vwidth, .v3=(y + 1)*vwidth};
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - 2, .v3=y*vwidth};
            }
        }
        // connecting the empty squares between the y-parallel backside perimeter vertices and the x-parallel backside vertices
        obj.faces[fcount++] = (Face){.v1=bx0, .v2=by0, .v3=1};
        obj.faces[fcount++] = (Face){.v1=by0, .v2=vwidth + 1, .v3=1};
        obj.faces[fcount++] = (Face){.v1=vwidth*(vheight-2) + 1, .v2=by0 + 2*(vheight - 3), .v3=bx0+1};
        obj.faces[fcount++] = (Face){.v1=vwidth*(vheight-2) + 1, .v2=bx0 + 1, .v3=vwidth*(vheight - 1) + 1};
        obj.faces[fcount++] = (Face){.v1=by0 + 1, .v2=vwidth, .v3=2*vwidth};
        obj.faces[fcount++] = (Face){.v1=by0 + 1, .v2=bx0+2*(vwidth-1), .v3=vwidth};
        obj.faces[fcount++] = (Face){.v1=bx0 + 2*vwidth - 1, .v2=vwidth*(vheight-1), .v3=vwidth*vheight};
        obj.faces[fcount++] = (Face){.v1=bx0 + 2*vwidth - 1, .v2=by0 + 2*(vheight - 3) - 1, .v3=vwidth*(vheight-1)};
        // 2 backside faces
        obj.faces[fcount++] = (Face){.v1=bx0, .v2=bx0 + 2*vwidth - 1, .v3= bx0 + 1};
        obj.faces[fcount++] = (Face){.v1=bx0, .v2= bx0 + 2*vwidth - 2, .v3=bx0 + 2*vwidth - 1};
    }
    if (vcount != obj.nverts) {
        printf("\nWarning: allocated %d vertices for the lithophane object, but created %d", obj.nverts, vcount);
        obj.nverts = vcount;
    }
    if (fcount != obj.nfaces) {
        printf("\nWarning: allocated %d faces for the lithophane object, but created %d", obj.nfaces, fcount);
        obj.nfaces = fcount;
    }
    
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