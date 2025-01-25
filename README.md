# Image to Lithophane CLI
Turn any image into a 3D-printable lithophane! A lithophane is a thin translucent panel that reveals an image when backlit. Features include:
- Convert any common image format (PNG, JPEG, BMP, etc.) to a 3D lithophane model
- Optional decorative frame with customizable dimensions and other options
- Adjustable thickness, resolution, and scaling  

![results.png](results.png)
could be worse...

## Dependencies
Any C compiler should work. Only depedency is [stb_image](https://github.com/nothings/stb), which is included in `include/`.

## Building
Just clone the repo, cd in and:
```bash
gcc src/main.c -o litho -lm
```

## Usage
Basic usage:
```bash
litho your_image.jpg
```

With options:
```bash
litho your_image.png --frame_width 25 --min_thickness 1.5 --bevel_corners -o output.obj
```

### Options
- `--has_frame`: Add a decorative frame
- `--bevel_corners`: Add beveled corners to the front inside part of the frame
- `--min_thickness <mm>`: Minimum thickness (default: 3.0mm)
- `--max_thickness <mm>`: Maximum thickness (default: 10mm)
- `--frame_width <mm>`: Frame width (default: 25mm)
- `--frame_thickness <mm>`: Frame thickness (default: 25mm)
- `--frame_angle <degrees>`: Frame bevel angle (default: 45°)
- `--pixels_per_vertex <n>`: Resolution control (default: 2)
- `--scale <n>`: Overall scale factor (default: 0.25)
- `--flip_x/y/z`: Flip along respective axis

The output is a standard .obj file that you can slice with your favorite 3D printing software!

## Tips
- For best results, use high-contrast images
- Print vertically with layer heights of 0.12 ish
- Normal print speed is fine.
- Try adjusting `bright_scale` if the contrast looks off
- Lower `pixels_per_vertex` to make the image smaller and lower resolution (but larger files)

## To Do
- Different output formats (STL, 3MF, etc.) (these obj files are really big and slow to slice)
- More frame options
- GUI
    - Possibly a web app
    - Possibly with a renderer for previewing
- Better image preprocessing (denoising, smoothing)
- Refactor how faces are added
    - I'm thinking we have an addFace function that can take some combination of vertex positions or vertex indices.
    - For any vertex positions passed, it would search the current vertices and if one is close enough, just use that index for face grouping.
    - It would then return 3 indices for the vertices it chose to make up the face.
    - This lets us create vertices, join them into faces, and give them names (via the returned indices, avoiding having to repeatedly instantiate the same 3d position or research in the existing vertices) with one call, avoiding the need for elaborate indexing schemes to keep track of previous vertices.
    - This function could also be easily wrapped to create helper functions for creating not just triangles, but more complex faces (e.g. a rect function whcih only requires two arguments).
- Pixels per vertex is a bad setting cause it makes the natural unit of distance in the object file to be 'pixels' which is only meaningful relative to the input image size. We should just set object size, and adjust the pixel width accordingly so the units is mm.
