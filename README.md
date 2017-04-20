<img src="https://github.com/victorfisac/rPBR/blob/master/src/icon/rpbr.png">

# rPBR
_Created by Víctor Fisac [www.victorfisac.com]_

rPBR is a 3D model viewer with a physically based rendering (PBR) pipeline written in pure C. The PBR pipeline is written directly using OpenGL and the viewer uses raylib programming library for windows management, inputs and interface drawing.

The viewer uses a High Dynamic Range (HDR) file to load and create an environment: cubemap, prefilter reflection map, irradiance map (global illumination) and brdf map. By the other hand, physically based rendering materials are created to store model textures: albedo, tangent space normals, metallic, roughness, ambient occlusion, emission and parallax.

The header contains a few customizable define values. I set the values that gived me the best results.

```c
#define         MODEL_SCALE                 1.75f           // Model scale transformation for rendering
#define         MODEL_OFFSET                0.45f           // Distance between models for rendering
#define         ROTATION_SPEED              0.0f            // Models rotation speed

#define         LIGHT_SPEED                 0.1f            // Light rotation input speed
#define         LIGHT_DISTANCE              3.5f            // Light distance from center of world
#define         LIGHT_HEIGHT                1.0f            // Light height from center of world
#define         LIGHT_RADIUS                0.05f           // Light gizmo drawing radius
#define         LIGHT_OFFSET                0.03f           // Light gizmo drawing radius when mouse is over

#define         CUBEMAP_SIZE                1024            // Cubemap texture size
#define         IRRADIANCE_SIZE             32              // Irradiance map from cubemap texture size
#define         PREFILTERED_SIZE            256             // Prefiltered HDR environment map texture size
#define         BRDF_SIZE                   512             // BRDF LUT texture map size
```

_Note: paths to environment and physically based rendering shaders are defined in pbrcore.h. Check the paths if your program doesn't load shaders properly._


Dependencies
-----

rPBR uses the following C libraries

   *  [raylib.h](https://github.com/raysan5/raylib/blob/master/src/raylib.h)     - raylib framework for window management and inputs.
   *  [raygui.h](https://github.com/raysan5/raygui/blob/master/raygui.h)     - raylib user interface drawing functions.
   *  stdlio.h     - Trace log messages [printf()].
   *  math.h       - Math operations functions [pow()].
   *  [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)  - Image loading [Sean Barret].
   *  [glad.h](https://github.com/glfw/glfw/blob/master/deps/glad/glad.h)       - OpenGL API [3.3 Core profile].


Screenshots
-----
<img src="https://github.com/victorfisac/rPBR/blob/master/screenshots/pbr_cerberus.png">

<img src="https://github.com/victorfisac/rPBR/blob/master/screenshots/pbr_gold.png">

<img src="https://github.com/victorfisac/rPBR/blob/master/screenshots/pbr_blaster.png">

Credits
-----

   * [Victor Fisac](http://www.victorfisac.com) - Main rPBR developer
   * [Ramón Santamaria](http://www.raylib.com) - Support designing and developing rPBR
