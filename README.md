### A Lithophane maker written in C
Convert an image to a lithophane as a 3d object file, ready to be sliced and printed on a normal fdm 3d printer.  
`main` contains code for the CLI and its options.  
`img` defines an image type as well as the method for loading images, using [`stb_image`](https://github.com/nothings/stb).  
`geometry` figures out where the vertices of the object file should be based on the loaded image, and code for how the output file is written.  
  
  actually i want to try out CLay so i might make it an application and potentially have an embedded little window so you can look at and rotate the 3d object as you tweak settings and stuff.  
  hmm...