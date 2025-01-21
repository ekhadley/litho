#include <stdio.h>
#include <math.h>
#include "img.c"


typedef struct {
    int has_frame;
    int bevel_corners;
    int pixels_per_vertex;
    float min_thickness;
    float max_thickness;
    float bright_scale;
    float frame_thickness;
    float frame_angle;
    float frame_width;
    float scale;
    int flip_x;
    int flip_y;
    int flip_z;
} LithoOptions;

LithoOptions defaultLithoOptions() {
    return (LithoOptions){
        .has_frame = 1,
        .bevel_corners = 0,
        .pixels_per_vertex = 2,
        .min_thickness = 3.0,
        .max_thickness = 10,
        .bright_scale = 0.03,
        .frame_thickness = 25,
        .frame_angle = 45,
        .frame_width = 25,
        .scale = 0.25,
        .flip_x = 0,
        .flip_y = 1,
        .flip_z = 1,
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
    int max_verts;
    int n_verts;
    Face* faces;
    int max_faces;
    int n_faces;
} Obj;
void addVert(Obj* obj, float x, float y, float z) {
    obj->verts[obj->n_verts++] = (Pos){.x = x, .y = y, .z = z};
}
void addFace(Obj* obj, int v1, int v2, int v3) {
    obj->faces[obj->n_faces++] = (Face){.v1 = v1, .v2 = v2, .v3 = v3};
}

void scaleObj(Obj* obj, float scale) {
    for (int i = 0; i < obj->n_verts; i++) {
        obj->verts[i].x *= scale;
        obj->verts[i].y *= scale;
        obj->verts[i].z *= scale;
    }
}

void flipObjX(Obj* obj) {
    for (int i = 0; i < obj->n_verts; i++) {
        obj->verts[i].x = -obj->verts[i].x;
    }
    for (int i = 0; i < obj->n_faces; i++) {
        int temp = obj->faces[i].v2;
        obj->faces[i].v2 = obj->faces[i].v3;
        obj->faces[i].v3 = temp;
    }
}

void flipObjY(Obj* obj) {
    for (int i = 0; i < obj->n_verts; i++) {
        obj->verts[i].y = -obj->verts[i].y;
    }
    for (int i = 0; i < obj->n_faces; i++) {
        int temp = obj->faces[i].v2;
        obj->faces[i].v2 = obj->faces[i].v3;
        obj->faces[i].v3 = temp;
    }
}

void flipObjZ(Obj* obj) {
    for (int i = 0; i < obj->n_verts; i++) {
        obj->verts[i].z = -obj->verts[i].z;
    }
    for (int i = 0; i < obj->n_faces; i++) {
        int temp = obj->faces[i].v2;
        obj->faces[i].v2 = obj->faces[i].v3;
        obj->faces[i].v3 = temp;
    }
}

Obj initLithoObj(const Image img, const LithoOptions opts) {
    int vwidth = floor(img.width/opts.pixels_per_vertex); // width in vertices
    int vheight = floor(img.height/opts.pixels_per_vertex); // height in vertices
    int max_verts = 0;
    if (opts.has_frame == 1) {
        // vertices: image vertices, upper frame perimeter vertices, 4 backside corners, 8 inner frame vertices, 8 outer frame vertices, 8 backside frame perimeter vertices, 
        max_verts = vwidth*vheight + 2*vwidth + 2*vheight + 4 + 8 + 8 + 8;
    } else {
        max_verts = vwidth*vheight + 2*vwidth + 2*vheight + 4;
    }
    int max_faces = max_verts*2 + 100; // seems to work
    Pos* verts = (Pos*)malloc(sizeof(Pos)*max_verts);
    Face* faces = (Face*)malloc(sizeof(Face)*max_faces);
    return (Obj){ .verts = verts, .max_verts = max_verts, .n_verts = 0, .faces = faces, .max_faces = max_faces, .n_faces = 0};
}

Obj makeLithoObj(Image img, LithoOptions opts) {
    Obj obj = initLithoObj(img, opts);
    Image brightness = rgbToBrightness(img);

    float pixel_mean = getPixelMean(brightness, 0);
    float pixel_var = getPixelVar(brightness, pixel_mean, 0);
    float max_pixel_brightness = opts.bright_scale * (getPixelMinMax(brightness, 0).max - pixel_mean) / pixel_var;

    int vwidth = brightness.width/opts.pixels_per_vertex;
    int vheight = brightness.height/opts.pixels_per_vertex;

    for (int y = 0; y < vheight; y += 1) {
        for (int x = 0; x < vwidth; x += 1) { // face vertices
            int b = brightness.img[img.width*y*opts.pixels_per_vertex + x*opts.pixels_per_vertex];
            // float h = -((b - pixel_mean)/pixel_var)*opts.bright_scale + opts.min_thickness;
            float h = fmax(-((b - pixel_mean))*opts.bright_scale + opts.min_thickness, -opts.min_thickness);

            addVert(&obj, x, h, y);
            if ((x != 0) && (y != 0)) {
                addFace(&obj, obj.n_verts, obj.n_verts - vwidth, obj.n_verts - vwidth - 1);  // right hand rule gives the right normal
                addFace(&obj, obj.n_verts, obj.n_verts - vwidth - 1, obj.n_verts - 1);
            }
        }
    }
    if (opts.has_frame == 1) {
        // the horizontal distance to the inner frame edge which gives the desired bevel angle
        float hdist = opts.frame_thickness / (2*tan(opts.frame_angle * 3.14159 / 180.0)); 
        int ofc0 = obj.n_verts; // outer frame vertices start index
        addVert(&obj, -(hdist + opts.frame_width),         -opts.frame_thickness/2, -(hdist + opts.frame_width)); // 8 outer corners of the frame
        addVert(&obj, vwidth + (hdist + opts.frame_width), -opts.frame_thickness/2, -(hdist + opts.frame_width));         
        addVert(&obj, -(hdist + opts.frame_width),         -opts.frame_thickness/2, vheight + (hdist + opts.frame_width));
        addVert(&obj, vwidth + (hdist + opts.frame_width), -opts.frame_thickness/2, vheight + (hdist + opts.frame_width));
        addVert(&obj, -(hdist + opts.frame_width),         opts.frame_thickness/2,  -(hdist + opts.frame_width));         
        addVert(&obj, vwidth + (hdist + opts.frame_width), opts.frame_thickness/2,  -(hdist + opts.frame_width));         
        addVert(&obj, -(hdist + opts.frame_width),         opts.frame_thickness/2,  vheight + (hdist + opts.frame_width));
        addVert(&obj, vwidth + (hdist + opts.frame_width), opts.frame_thickness/2,  vheight + (hdist + opts.frame_width));
        addFace(&obj, ofc0+6, ofc0+2, ofc0+1); // the 4 outside faces of the outer frame
        addFace(&obj, ofc0+6, ofc0+1, ofc0+5);
        addFace(&obj, ofc0+1, ofc0+3, ofc0+7);
        addFace(&obj, ofc0+1, ofc0+7, ofc0+5);
        addFace(&obj, ofc0+4, ofc0+7, ofc0+3);
        addFace(&obj, ofc0+4, ofc0+8, ofc0+7);
        addFace(&obj, ofc0+6, ofc0+4, ofc0+2);
        addFace(&obj, ofc0+4, ofc0+6, ofc0+8);
        int ifc0 = obj.n_verts; // inner frame vertices start index
        addVert(&obj, -hdist,         -opts.frame_thickness/2, -hdist); // 8 inner corners of the frame
        addVert(&obj, vwidth + hdist, -opts.frame_thickness/2, -hdist);
        addVert(&obj, -hdist,         opts.frame_thickness/2,  -hdist);
        addVert(&obj, vwidth + hdist, opts.frame_thickness/2,  -hdist);
        addVert(&obj, -hdist,         -opts.frame_thickness/2, vheight + hdist);
        addVert(&obj, vwidth + hdist, -opts.frame_thickness/2, vheight + hdist);
        addVert(&obj, -hdist,         opts.frame_thickness/2,  vheight + hdist);
        addVert(&obj, vwidth + hdist, opts.frame_thickness/2,  vheight + hdist);
        addFace(&obj, ofc0+4, ifc0+6, ofc0+2); // bottom faces connecting inner and outer frames
        addFace(&obj, ofc0+2, ifc0+6, ifc0+2);
        addFace(&obj, ifc0+2, ifc0+1, ofc0+1);
        addFace(&obj, ofc0+1, ofc0+2, ifc0+2); // TODO: change these based on the bevel_corners setting so that the outer corners also bevel.
        addFace(&obj, ifc0+6, ofc0+4, ofc0+3); // or add another setting like bevel_outer_corners
        addFace(&obj, ifc0+6, ofc0+3, ifc0+5);
        addFace(&obj, ifc0+5, ofc0+3, ofc0+1);
        addFace(&obj, ofc0+1, ifc0+1, ifc0+5);
        addFace(&obj, ofc0+8, ofc0+6, ifc0+8); // topside faces connecting inner and outer frame corners
        addFace(&obj, ofc0+6, ifc0+4, ifc0+8);
        addFace(&obj, ifc0+4, ofc0+5, ifc0+3);
        addFace(&obj, ofc0+5, ifc0+4, ofc0+6);
        addFace(&obj, ofc0+5, ifc0+7, ifc0+3);
        addFace(&obj, ofc0+5, ofc0+7, ifc0+7);
        addFace(&obj, ifc0+7, ofc0+7, ofc0+8);
        addFace(&obj, ifc0+7, ofc0+8, ifc0+8);

        int fpnt = obj.n_verts; // start of frame perimeter top north side
        for (int x = 0; x < vwidth; x += 1) {
            addVert(&obj, x+0.01, opts.frame_thickness/2, -hdist);
            if (x != 0) {
                addFace(&obj, x, x+1, fpnt+x);
                addFace(&obj, fpnt+x+1, fpnt+x, x+1);
            }
        }
        int fpst = obj.n_verts; // frame perimeter top south
        for (int x = 0; x < vwidth; x += 1) {
            int bottomx = x + vwidth*(vheight - 1);
            addVert(&obj, x+0.01, opts.frame_thickness/2, vheight+hdist);
            if (x != 0) {
                addFace(&obj, bottomx, fpst+x, bottomx+1);
                addFace(&obj, fpst+x+1, bottomx+1, fpst+x);
            }
        }
        int fpwt = obj.n_verts; // frame perimeter top west side
        for (int y = 0; y < vheight; y += 1) {
            addVert(&obj, -hdist, opts.frame_thickness/2, y+0.01);
            if (y != 0) {
                addFace(&obj, y*vwidth + 1, (y - 1)*vwidth + 1, fpwt + y);
                addFace(&obj, fpwt + y, fpwt + y + 1, y*vwidth + 1);
            }
        }
        int fpet = obj.n_verts; // frame perimeter top east side
        for (int y = 0; y < vheight; y += 1) {
            addVert(&obj, vwidth + hdist, opts.frame_thickness/2, y+0.01);
            if (y != 0) {
                addFace(&obj, (y + 1)*vwidth, fpet + y, y*vwidth);
                addFace(&obj, (y + 1)*vwidth, fpet + y + 1, fpet + y);
            }
        }
        int bp0 = obj.n_verts; // start of backside vertices
        addVert(&obj, 0, -opts.min_thickness, 0);                    // bp0 + 1: 1    (directly under vertex 1, the top left corner of image)
        addVert(&obj, vwidth, -opts.min_thickness, 0);               // bp0 + 2: vwidth   (directly under top right corner of image)
        addVert(&obj, 0, -opts.min_thickness, vheight);              // bp0 + 3: vwidth*(vheight - 1) + 1     (directly under bottom left corner of image)
        addVert(&obj, vwidth, -opts.min_thickness, vheight);         // bp0 + 4: vwidth*vheight   (directly under bottom right corner of image)
        addVert(&obj, -hdist, -opts.frame_thickness/2, 0);               // bp0 + 5: fpnt + 1   (directly under first north perimeter vertex)
        addVert(&obj, 0, -opts.frame_thickness/2, -hdist);               // bp0 + 6: fpwt + 1   (etc)
        addVert(&obj, vwidth + hdist, -opts.frame_thickness/2, 0);       // bp0 + 7: fpet + 1
        addVert(&obj, vwidth, -opts.frame_thickness/2, -hdist);          // bp0 + 8: fpnt + vwidth
        addVert(&obj, -hdist, -opts.frame_thickness/2, vheight);         // bp0 + 9: fpwt + vheight
        addVert(&obj, 0, -opts.frame_thickness/2, vheight + hdist);      // bp0 + 10: fpst
        addVert(&obj, vwidth + hdist, -opts.frame_thickness/2, vheight); // bp0 + 11: fpet + vheight
        addVert(&obj, vwidth, -opts.frame_thickness/2, vheight + hdist); // bp0 + 12: fpst + vwidth
        
        addFace(&obj, bp0 + 1, bp0 + 4, bp0 + 3); // two back side faces
        addFace(&obj, bp0 + 1, bp0 + 2, bp0 + 4);
        addFace(&obj, bp0 + 1, bp0 + 9, bp0 + 5); // 4 rectangular sections for the back side-inner frame surfaces
        addFace(&obj, bp0 + 1, bp0 + 3, bp0 + 9); 
        addFace(&obj, bp0 + 4, bp0 + 12, bp0 + 10); 
        addFace(&obj, bp0 + 4, bp0 + 10, bp0 + 3); 
        addFace(&obj, bp0 + 2, bp0 + 6, bp0 + 8); 
        addFace(&obj, bp0 + 2, bp0 + 1, bp0 + 6); 
        addFace(&obj, bp0 + 4, bp0 + 7, bp0 + 11); 
        addFace(&obj, bp0 + 4, bp0 + 2, bp0 + 7); 
        if (opts.bevel_corners == 1) {
            addFace(&obj, fpwt + 1, fpnt + 1, ifc0 + 3); // topside corner connections
            addFace(&obj, fpwt + 1, 1, fpnt + 1);
            addFace(&obj, fpnt + vwidth, fpet + 1, ifc0 + 4);
            addFace(&obj, fpnt + vwidth, vwidth, fpet + 1);
            addFace(&obj, fpet + vheight, vwidth*vheight, fpst + vwidth);
            addFace(&obj, fpst + vwidth, ifc0 + 8, fpet + vheight);
            addFace(&obj, vwidth*(vheight - 1) + 1, fpwt + vheight, fpst + 1);
            addFace(&obj, fpst + 1, fpwt + vheight, ifc0 + 7);
            addFace(&obj, bp0 + 6, bp0 + 5, ifc0 + 1); // topside corner connections
            addFace(&obj, bp0 + 6, bp0 + 1, bp0 + 5);
            addFace(&obj, bp0 + 8, ifc0 + 2, bp0 + 7);
            addFace(&obj, bp0 + 8, bp0 + 7, bp0 + 2);
            addFace(&obj, bp0 + 11, bp0 + 12, bp0 + 4);
            addFace(&obj, bp0 + 12, bp0 + 11, ifc0 + 6);
            addFace(&obj, bp0 + 3, bp0 + 10, bp0 + 9);
            addFace(&obj, bp0 + 10, ifc0 + 5, bp0 + 9);
        } else {
            addFace(&obj, 1, fpnt + 1, ifc0 + 3); // topside corner connections
            addFace(&obj, 1, ifc0 + 3, fpwt + 1);
            addFace(&obj, vwidth, ifc0 + 4, fpnt + vwidth);
            addFace(&obj, vwidth, fpet + 1, ifc0 + 4);
            addFace(&obj, vwidth*vheight, ifc0 + 8, fpet + vheight);
            addFace(&obj, vwidth*vheight, fpst + vwidth, ifc0 + 8);
            addFace(&obj, vwidth*(vheight - 1) + 1, fpwt + vheight, ifc0 + 7);
            addFace(&obj, vwidth*(vheight - 1) + 1, ifc0 + 7, fpst + 1);
            addFace(&obj, bp0 + 1, bp0 + 5, ifc0 + 1); // bottom side corner connections
            addFace(&obj, bp0 + 1, ifc0 + 1, bp0 + 6);
            addFace(&obj, bp0 + 2, bp0 + 8, ifc0 + 2);
            addFace(&obj, bp0 + 2, ifc0 + 2, bp0 + 7);
            addFace(&obj, bp0 + 4, bp0 + 11, ifc0 + 6);
            addFace(&obj, bp0 + 4, ifc0 + 6, bp0 + 12);
            addFace(&obj, bp0 + 3, ifc0 + 5, bp0 + 9);
            addFace(&obj, bp0 + 3, bp0 + 10, ifc0 + 5);
        }

    } else {
        int bx0 = obj.n_verts + 1; // this is the index (shifted by 1 cuase of obj file indexing) of the first vertex placed on the backside perimeter
        for (int x = 0; x < vwidth; x += 1) { // y-parallel back plane perimeter vertices
            addVert(&obj, x, -max_pixel_brightness, 0);
            addVert(&obj, x, -max_pixel_brightness, vheight - 1);
            if (x != 0) {
                addFace(&obj, obj.n_verts - 1, (obj.n_verts - 1) - 2, x);
                addFace(&obj, obj.n_verts - 1, x, x + 1);
                addFace(&obj, obj.n_verts, obj.n_verts - vwidth - x - 1, obj.n_verts - vwidth - x - 2);
                addFace(&obj, obj.n_verts, obj.n_verts - vwidth - x - 2, obj.n_verts - 2);
            }
        }
        int by0 = obj.n_verts + 1; // above
        for (int y = 1; y < vheight-1; y += 1) {
            addVert(&obj, 0, -max_pixel_brightness, y);
            addVert(&obj, vwidth - 1, -max_pixel_brightness, y);
            if (y != 1) {
                addFace(&obj, obj.n_verts - 1, y*vwidth + 1, (y-1)*vwidth + 1);
                addFace(&obj, obj.n_verts - 1, (y-1)*vwidth + 1, (obj.n_verts - 1) - 2);
                addFace(&obj, obj.n_verts, y*vwidth, (y + 1)*vwidth);
                addFace(&obj, obj.n_verts, obj.n_verts - 2, y*vwidth);
            }
        }
        addFace(&obj, bx0, by0, 1); // connecting the empty squares between the y-parallel backside perimeter vertices and the x-parallel backside vertices
        addFace(&obj, by0, vwidth + 1, 1);
        addFace(&obj, vwidth*(vheight-2) + 1, by0 + 2*(vheight - 3), bx0+1);
        addFace(&obj, vwidth*(vheight-2) + 1, bx0 + 1, vwidth*(vheight - 1) + 1);
        addFace(&obj, by0 + 1, vwidth, 2*vwidth);
        addFace(&obj, by0 + 1, bx0+2*(vwidth-1), vwidth);
        addFace(&obj, bx0 + 2*vwidth - 1, vwidth*(vheight-1), vwidth*vheight);
        addFace(&obj, bx0 + 2*vwidth - 1, by0 + 2*(vheight - 3) - 1, vwidth*(vheight-1));
        addFace(&obj, bx0, bx0 + 2*vwidth - 1,  bx0 + 1); // 2 backside faces
        addFace(&obj, bx0,  bx0 + 2*vwidth - 2, bx0 + 2*vwidth - 1);
    }

    // if (obj.n_verts != obj.max_verts) {
    //     printf("Warning: allocated %d vertices for the lithophane object, but created %d\n", obj.max_verts, obj.n_verts);
    // }
    // if (obj.n_faces != obj.max_faces) {
    //     printf("Warning: allocated %d faces for the lithophane object, but created %d\n", obj.max_faces, obj.n_faces);
    // } 

    // Apply transformations
    if (opts.scale != 1.0) {
        scaleObj(&obj, opts.scale);
    }
    if (opts.flip_x) {
        flipObjX(&obj);
    }
    if (opts.flip_y) {
        flipObjY(&obj);
    }
    if (opts.flip_z) {
        flipObjZ(&obj);
    }

    stbi_image_free(brightness.img);
    return obj;
}



void saveObj(Obj obj, const char* filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "# Lithophane obj file made using https://github.com/ekhadley/litho\n");
    fprintf(f, "o litho\n");
    for (int i = 0; i < obj.n_verts; i++) {
        fprintf(f, "v %f %f %f\n", obj.verts[i].x, obj.verts[i].y, obj.verts[i].z);
    }
    fprintf(f, "g faces\n");
    for (int i = 0; i < obj.n_faces; i++) {
        fprintf(f, "f %d %d %d\n", obj.faces[i].v1, obj.faces[i].v2, obj.faces[i].v3);
    }
    fclose(f);
}