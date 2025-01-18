#include <stdio.h>
#include <math.h>
#include "img.c"


typedef struct {
    int has_frame;
    int pixels_per_vertex;
    float min_thickness;
    float max_thickness;
    float bright_scale;
    float frame_thickness;
    float frame_angle;
    float frame_width;
} LithoOptions;

LithoOptions defaultLithoOptions() {
    return (LithoOptions){
        .has_frame = 0,
        .pixels_per_vertex = 3,
        .min_thickness = 1.0,
        .max_thickness = 5,
        .bright_scale = 0.01,
        .frame_thickness = 30,
        .frame_angle = 90,
        .frame_width = 20,
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

    int imglen = img.width*img.height*img.channels;
    int vcount = 0;
    int fcount = 0;
    int vwidth = brightness.width/opts.pixels_per_vertex;
    int vheight = brightness.height/opts.pixels_per_vertex;
    for (int y = 0; y < vheight; y += 1) {
        for (int x = 0; x < vwidth; x += 1) { // face vertices
            int b = brightness.img[img.width*y*opts.pixels_per_vertex + x*opts.pixels_per_vertex];
            //float h = b*opts.bright_scale + opts.min_thickness;
            float h = -b*opts.bright_scale;

            obj.verts[vcount++] = (Pos){.x = x, .y=h, .z=y};
            if ((x != (vwidth - 1)) && (y != 0)) {
                // vertex indices start at zero for face grouping
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount - vwidth + 1, .v3=vcount - vwidth};  // right hand rule gives the right normal
                obj.faces[fcount++] = (Face){.v1=vcount, .v2=vcount + 1, .v3=vcount - vwidth + 1};
            }
        }
    }
    if (opts.has_frame == 0) {
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
        obj.faces[fcount++] = (Face){.v1=bx0, .v2=bx0 + 2*vwidth - 1, .v3= bx0 + 1}; // 2 backside faces
        obj.faces[fcount++] = (Face){.v1=bx0, .v2= bx0 + 2*vwidth - 2, .v3=bx0 + 2*vwidth - 1};
    } else {
        int fc0 = vcount; // start of the 8 corners of the frame
        obj.verts[vcount++] = (Pos){.x = -opts.frame_width, .y=-opts.frame_thickness/2.0, .z=-opts.frame_width};                  // 1: - - -
        obj.verts[vcount++] = (Pos){.x = vwidth + opts.frame_width, .y=-opts.frame_thickness/2.0, .z=-opts.frame_width};          // 2: + - -
        obj.verts[vcount++] = (Pos){.x = -opts.frame_width, .y=-opts.frame_thickness/2.0, .z=vheight + opts.frame_width};         // 3: - - +
        obj.verts[vcount++] = (Pos){.x = vwidth + opts.frame_width, .y=-opts.frame_thickness/2.0, .z=vheight + opts.frame_width}; // 4: + - +
        obj.verts[vcount++] = (Pos){.x = -opts.frame_width, .y=opts.frame_thickness/2.0, .z=-opts.frame_width};                   // 5: - + -
        obj.verts[vcount++] = (Pos){.x = vwidth + opts.frame_width, .y=opts.frame_thickness/2.0, .z=-opts.frame_width};           // 6: + + -
        obj.verts[vcount++] = (Pos){.x = -opts.frame_width, .y=opts.frame_thickness/2.0, .z=vheight + opts.frame_width};          // 7: - + +
        obj.verts[vcount++] = (Pos){.x = vwidth + opts.frame_width, .y=opts.frame_thickness/2.0, .z=vheight + opts.frame_width};  // 8: + + +
        obj.faces[fcount++] = (Face){.v1=fc0+6, .v2=fc0+2, .v3=fc0+1};
        obj.faces[fcount++] = (Face){.v1=fc0+6, .v2=fc0+1, .v3=fc0+5};
        obj.faces[fcount++] = (Face){.v1=fc0+1, .v2=fc0+3, .v3=fc0+7};
        obj.faces[fcount++] = (Face){.v1=fc0+1, .v2=fc0+7, .v3=fc0+5};
        obj.faces[fcount++] = (Face){.v1=fc0+4, .v2=fc0+7, .v3=fc0+3};
        obj.faces[fcount++] = (Face){.v1=fc0+4, .v2=fc0+8, .v3=fc0+7};
        obj.faces[fcount++] = (Face){.v1=fc0+6, .v2=fc0+4, .v3=fc0+2};
        obj.faces[fcount++] = (Face){.v1=fc0+4, .v2=fc0+6, .v3=fc0+8};
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