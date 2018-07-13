#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>
#include <stdlib.h>

inline float Saturate(float f)
{
    if (f > 1.0f)
        return 1.0f;
    else if (f < 0.0f)
        return 0.0f;
    else
        return f;
}

inline float Lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline float sRGBU8_to_Linear(unsigned char c)
{
    float ret = float(c) / 255.0f;
    return pow(ret, 2.2f);
}

inline unsigned char Linear_to_sRGBU8(float f)
{
    f = Saturate(f);
    f = pow(f, 1.0f / 2.2f);
    return unsigned char(f * 255.0f);
}

// Do the lerp in shader code, in full 32 bit floats, then write out to a U8 buffer for storage
void ShaderLerp(const unsigned char* data, int width, int height, int numChannels)
{
    std::vector<unsigned char> imageCopy;
    imageCopy.resize(width*height*numChannels);
    memcpy(&imageCopy[0], data, imageCopy.size());

    std::vector<unsigned char> imageCopy2;
    imageCopy2.resize(width*height*numChannels);
    memcpy(&imageCopy2[0], data, imageCopy.size());

    float HDRColorR = 1.6f;
    float HDRColorG = 1.4f;
    float HDRColorB = 0.8f;

    for (int y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = &data[width*y * 4];
        unsigned char* dstRow = &imageCopy[width*y * 4];
        unsigned char* dstRow2 = &imageCopy2[width*y * 4];
        for (int x = 0; x < width; ++x)
        {
            float r = sRGBU8_to_Linear(srcRow[0]);
            float g = sRGBU8_to_Linear(srcRow[1]);
            float b = sRGBU8_to_Linear(srcRow[2]);

            float lerpAmount = float(x) / float(width);

            r = Lerp(r, HDRColorR, lerpAmount);
            g = Lerp(g, HDRColorG, lerpAmount);
            b = Lerp(b, HDRColorB, lerpAmount);

            dstRow[0] = Linear_to_sRGBU8(r);
            dstRow[1] = Linear_to_sRGBU8(g);
            dstRow[2] = Linear_to_sRGBU8(b);
            dstRow[3] = srcRow[3];

            if (r > 1.0f || g > 1.0f || b > 1.0f)
            {
                dstRow2[0] = 255;
                dstRow2[1] = 0;
                dstRow2[2] = 0;
                dstRow2[3] = dstRow[3];
            }
            else
            {
                dstRow2[0] = dstRow[0];
                dstRow2[1] = dstRow[1];
                dstRow2[2] = dstRow[2];
                dstRow2[3] = dstRow[3];
            }

            srcRow += 4;
            dstRow += 4;
            dstRow2 += 4;
        }
    }

    stbi_write_png("out/1_shader.png", width, height, 4, &imageCopy[0], width * 4);
    stbi_write_png("out/1_shaderclip.png", width, height, 4, &imageCopy2[0], width * 4);
}

// Do the lerp via alpha blending
void U8AlphaLerp(const unsigned char* data, int width, int height, int numChannels)
{
    std::vector<unsigned char> imageCopy;
    imageCopy.resize(width*height*numChannels);
    memcpy(&imageCopy[0], data, imageCopy.size());

    float HDRColorR = 1.6f;
    float HDRColorG = 1.4f;
    float HDRColorB = 0.8f;

    for (int y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = &data[width*y * 4];
        unsigned char* dstRow = &imageCopy[width*y * 4];
        for (int x = 0; x < width; ++x)
        {
            float r = sRGBU8_to_Linear(srcRow[0]);
            float g = sRGBU8_to_Linear(srcRow[1]);
            float b = sRGBU8_to_Linear(srcRow[2]);

            float lerpAmount = float(x) / float(width);

            r = Lerp(r, Saturate(HDRColorR), lerpAmount);
            g = Lerp(g, Saturate(HDRColorG), lerpAmount);
            b = Lerp(b, Saturate(HDRColorB), lerpAmount);

            dstRow[0] = Linear_to_sRGBU8(r);
            dstRow[1] = Linear_to_sRGBU8(g);
            dstRow[2] = Linear_to_sRGBU8(b);
            dstRow[3] = srcRow[3];

            srcRow += 4;
            dstRow += 4;
        }
    }

    stbi_write_png("out/2_U8.png", width, height, 4, &imageCopy[0], width * 4);
}

// Do the lerp via premultiplied alpha blending
void PMALerp(const unsigned char* data, int width, int height, int numChannels)
{
    std::vector<unsigned char> imageCopy;
    imageCopy.resize(width*height*numChannels);
    memcpy(&imageCopy[0], data, imageCopy.size());

    float HDRColorR = 1.6f;
    float HDRColorG = 1.4f;
    float HDRColorB = 0.8f;

    for (int y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = &data[width*y * 4];
        unsigned char* dstRow = &imageCopy[width*y * 4];
        for (int x = 0; x < width; ++x)
        {
            float r = sRGBU8_to_Linear(srcRow[0]);
            float g = sRGBU8_to_Linear(srcRow[1]);
            float b = sRGBU8_to_Linear(srcRow[2]);

            float lerpAmount = float(x) / float(width);

            float PMA_R = Saturate(HDRColorR * lerpAmount);
            float PMA_G = Saturate(HDRColorG * lerpAmount);
            float PMA_B = Saturate(HDRColorB * lerpAmount);

            r = r * (1.0f - lerpAmount) + PMA_R;
            g = g * (1.0f - lerpAmount) + PMA_G;
            b = b * (1.0f - lerpAmount) + PMA_B;

            dstRow[0] = Linear_to_sRGBU8(r);
            dstRow[1] = Linear_to_sRGBU8(g);
            dstRow[2] = Linear_to_sRGBU8(b);
            dstRow[3] = srcRow[3];

            srcRow += 4;
            dstRow += 4;
        }
    }

    stbi_write_png("out/3_PMA.png", width, height, 4, &imageCopy[0], width * 4);
}

int main(int argc, char** argv)
{
    int width, height, numChannels;
    unsigned char *data = stbi_load("scenery.png", &width, &height, &numChannels, 0);

    if (numChannels != 4)
    {
        printf("Image must have 4 channels, or you must put in code to handle images with other channel counts.\n");
        return 1;
    }

    ShaderLerp(data, width, height, numChannels);
    U8AlphaLerp(data, width, height, numChannels);
    PMALerp(data, width, height, numChannels);

    stbi_image_free(data);
    return 0;
}