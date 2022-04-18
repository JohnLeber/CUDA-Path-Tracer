# CUDA-PATH-TRACER

A DirectX 11/CUDA application that provides a basis for experimenting with path-tracing algorithms. The application loads a 3D polygon based mesh (Sponza) and allows the camera to be positioned using the mouse and WASD keys. There is a single light source that can also be positioned. There are two implementations of the algorithm, the first using the CPU/OpenMP, and the second using CUDA. I use [Intel's Open Image Denoise](https://www.openimagedenoise.org/index.html) for denosing the image.

The goal was to write a simple test framework that would allow me to load a model, position a camera and light source and experiment with basic ray/path tracing algorithms. Having now done this I would perhaps implement it differently if I could start again (with the knowledge I now have). As such, a better source to learn about how to write a ray tracer in CUDA can be found [here](https://developer.nvidia.com/blog/accelerated-ray-tracing-cuda/).

## Sample images

Crytek's Sponza model (64 samples + denoised)

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/Image_N64_DN_Upper.png)

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/64_DN.png)

64 samples - no denoising

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/64_no_denoising.png)

No lighting (textures only)

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/64_textures_only.png)

Default Sponza model (128 samples)

No lighting (textures only)         | Direct light only         |  Direct light and global illumination
:-------------------------:|:-------------------------:|:-------------------------:
![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/NoLighting.png) | ![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/128Direct2.png) | ![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/128Global2.png) 


## Instructions

### Building the software
1) Download oidn-1.4.3.x64.vc14.windows.zip (or whatever the latest version is) from [Intel's Open Image Denoise](https://www.openimagedenoise.org/downloads.html).
2) Once unzipped, create an environment variable called INTEL_DENOISE_PATH that points to the previous folder (e.g. "D:\oidn-1.4.3.x64.vc14.windows");
3) Download CUDA toolkit 11.X or higher from NVIDIA
4) I used Visual Studio 2019 to build the software (be sure to install the MFC module).

### Using the softare
1) Move the camera using the WASD keys and mouse (with left button down) to position the camera in the correct place for the render.
2) Select "Direct light" only (very fast) or "Global Illumination and Direct Light" (very slow).
3) The image can be divided in resolution by a factor of 1, 2 or 4 to speed up rendering.
4) There is only one light (above the scene) and it can be moved slightly to the left or right. The default position is probably Ok.
5) Set the number of samples to something low (e.g. 8) for a low quality quick render, or to a higher value (e.g. 128) for a better quality render.
6) Select CPU or CUDA if a CUDA device is present (see notes).
7) Press the "Start Render" button and the progress bar should indicate progress. There is no cancel button other than CTRL-ALT-DEL...

![alt text](https://github.com/JohnLeber/MFC-Path-Tracer/blob/master/Images/Screenshot.png)



## Notes

1) The only use of this application is to learn about and experiment with basic path tracing. I can't imagine the images that it produces being useful for any other purposes. It is very slow and there are numerous optimizations that could be made, but I do not have time.
2) On a RTX 3060 the CUDA code only seems to run between 3 and 8 times faster than the CPU version (the CPU was an 8 core Intel 9700k).
3) There is only one material - everything is diffuse.
4) I use a very simple octree acceleration structure to reduce ray/triangle intersection tests. This speeds it up a bit, but it is still a long way from being optimal.
5) I have tested the CUDA code on an GTX 1080 and a RTX 3060. If no CUDA device is present, it is likely to crash... It is hardcoded to deviceID = 0.
6) If replacing the OBJ model, all faces must be converted to triangles else the OBJ loading code may not load the model correctly.
7) To compile the application, I used CUDA 11.6 and Visual studio 2019 community edition - be sure to install the MFC desktop module.
8) I changed the extension of the Sponza OBJ file to .mesh to avoid gitignore filtering out OBJ files. There was probably a better way to do this but...
9) To use the Crytek Sponza model define CRYTEKSPONZA in framework.h

## A few potential improvements and flaws

1) Most of the execution time is spent doing the triangle/ray and bounding box/ray intersection tests. Optimizing this with K-d trees or a similar technique would reduce render times which can be massive on models with lots of triangles.
2) I have not looked at rendering the scene with multiple kernels in several passes. I suspect this would be more efficient rather than having one larger "uber-kernel". 
3) There is aliasing in the texture sampling code (when the "Use textures" feature is enabled). Filtering, similar to how shader sampling use mipmap chains etc could be looked at. Also, interpolation between pixels.
4) I have made no attempt to minimize/optimize instructions and branches in the CUDA code. There is also recursion in the CUDA code. This is a consequence of porting  the CPU code more-or-less directly to CUDA. A commercial GPU based ray-tracer, however, would not use recursion in practice.
5) There is only one material (diffuse) and that is probably not implemented correctly. More sophisticated materials could be added once the above have been corrected/improved. I am not likely to have time to do this however...
6) Experimenting with a denoising algorithm perhaps, to reduce the number of sampels required.

## Credits

1) The default Sponza model was created by Marko Dabrovic and can be downloaded from [here](http://hdri.cgtechniques.com/~sponza/files/). 
2) The other Sponza model is from [Crytek](https://www.crytek.com/cryengine).
3) The code I used to load the wavefront OBJ files I got from [here](http://code-section.com/blog/dx9-obj-loader).
4) Much of the boilerplate code used for DirectX 11 are based on samples Frank Luna's [DirectX 11 book](https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228).
5) The path tracing approach is partially based on samples from [scratchpixel]( https://www.scratchapixel.com/lessons/3d-basic-rendering/global-illumination-path-tracing) and Kevin Beason's [Global Illumination in 99 lines of C++](https://www.kevinbeason.com/smallpt/).
6) The Cornel box was downloaded from [here](https://casual-effects.com/data/).
