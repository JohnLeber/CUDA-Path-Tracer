# MFC-PATH-TRACER

A DirectX 11/MFC application that implements a simple path tracer. The application loads Crytek's Sponza model and allows the camera to be positioned using the mouse and WASD keys. There is a single light source that can also be positioned. There are two implementations of the algorithm, the first using the CPU and OpenMP, and the second using CUDA.

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/Image_grey_128.png)

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/Image_N128_Upper.png)


## Notes

1) The only use of this application is to learn about and experiment with basic path tracing. I can't imagine the images that it produces being useful for any other purposes although I think the images are technically correct. It is very slow and there are numerous optimizations that could be made, but I do not have time.
2) On a RTX 3060 the CUDA code only seems to run bewteen 3 and 8 times faster than the CPU version (the CPU was an 8 core Intel 9700k).
3) There is only one material - everything is diffuse.
4) I use a very simple octree acceleration structure to reduce ray/triangle intersection tests. This speeds it up a bit, but it is still along way from being optimal.
5) I have tested the CUDA code on an GTX 1080 and a RTX 3060. If no CUDA device is present, it is likely to crash.
6) If replacing the OBJ model, all faces must be converted to triangles else the OBJ loading file may not load the model correctly.
7) To compile the application I used CUDA 11.6 and Visual studio community edition - be sure to install the MFC desktop module.

## Credits

1) The code I used to load the wavefront OBJ files I got from [here](http://code-section.com/blog/dx9-obj-loader).
2) Much of the boilerplate code used for DirectX 11 are based on samples Frank Luna's [DirectX 11 book](https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228).
3) The path tracing approach is partially based on samples from [scratchpixel]( https://www.scratchapixel.com/lessons/3d-basic-rendering/global-illumination-path-tracing) and Kevin Beason's [Global Illumination in 99 lines of C++](https://www.kevinbeason.com/smallpt/).
