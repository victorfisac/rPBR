/*******************************************************************************************
*
*   rPBR [utils] - Physically Based Rendering utilities
*
*   Copyright (c) 2017 Victor Fisac
*
********************************************************************************************/

//----------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------
#include "external/stb_image_write.h"                   // Required for image saving

//----------------------------------------------------------------------------------
// Functions Declaration
//----------------------------------------------------------------------------------
void CaptureScreenshot(int width, int height);          // Take screenshot from screen and save it

//----------------------------------------------------------------------------------
// Functions Definition
//----------------------------------------------------------------------------------
// Take screenshot from screen and save it
void CaptureScreenshot(int width, int height)
{
    // Read screen pixels and save image as PNG
    unsigned char *screenData = (unsigned char *)calloc(width*height*4, sizeof(unsigned char));

    // NOTE: glReadPixels returns image flipped vertically -> (0,0) is the bottom left corner of the framebuffer
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

    // Flip image vertically!
    unsigned char *imgData = (unsigned char *)malloc(width*height*sizeof(unsigned char)*4);

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < (width*4); x++)
        {
            // Flip line
            imgData[((height - 1) - y)*width*4 + x] = screenData[(y*width*4) + x];

            // Set alpha component value to 255 (no trasparent image retrieval)
            // NOTE: Alpha value has already been applied to RGB in framebuffer, we don't need it!
            if (((x + 1)%4) == 0) imgData[((height - 1) - y)*width*4 + x] = 255;
        }
    }

    free(screenData);
    stbi_write_png("screenshot.png", width, height, 4, imgData, width*4);
    free(imgData);
}