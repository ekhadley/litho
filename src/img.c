#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image.h"
#include "../include/stb_image_write.h"

typedef struct {
    int width;
    int height;
    int channels;
    unsigned char* img;
} Image;

Image loadInputImage(const char* filename) {
    int width, height, channels;
    unsigned char* img = stbi_load(filename, &width, &height, &channels, 0);
    return (Image){.width = width, .height = height, .channels = channels, .img = img};
}
void savePng(const char* filename, Image img) {
    stbi_write_png(filename, img.width, img.height, img.channels, img.img, img.width*img.channels);
}

float RGBbrightness(int r, int g, int b) { // apparent brightness formula for the human eye
    return 0.299*r + 0.587*g + 0.114*b;
}
float brightnessAt(Image img, int x, int y) {
    int idx = (y*img.width + x)*img.channels;
    return RGBbrightness(img.img[idx], img.img[idx+1], img.img[idx+2]);
}

void printPixel(Image img, int x, int y) {
    int idx = ((img.width*y) + x)*img.channels;
    if (idx < img.height*img.width*img.channels) {
        if (img.channels == 3) {
            printf("[%d, %d, %d] (%f)\n", img.img[idx], img.img[idx+1], img.img[idx+2], RGBbrightness(img.img[idx], img.img[idx+1], img.img[idx+2]));
            return;
        } else {
            printf("\n[");
            for (int i = 0; i < img.channels-1; i++) {
                printf("%d, ", img.img[idx+i]);
            }
            printf("%d]", img.img[idx+img.channels-1]);
        }
    } else {
        printf("\ncoord [%d, %d] is not valid for image of shape: (%d, %d, %d)", x, y, img.height, img.width, img.channels);
    }
}

Image rgbToBrightness(Image img) {
    Image brightness = {.width=img.width, .height=img.height, .channels=1, .img=NULL};
    brightness.img = (unsigned char*)malloc(img.height*img.width);
    for (int i = 0; i < img.height*img.width; i++) {
        brightness.img[i] = floor(RGBbrightness(img.img[i*3], img.img[i*3 + 1], img.img[i*3 + 2]));
    }
    return brightness;
}

float getPixelMean(const Image img, const int channel) { // gives mean of pixels on a particular channel
    float mean = 0.0;
    for (int i = channel; i < (img.height*img.width); i += img.channels) {
        mean += img.img[i];
    }
    return mean/(img.height*img.width);
}
float getPixelVar(const Image img, const float mean, const int channel) { // gives mean of pixels on a particular channel
    float var = 0.0;
    for (int i = channel; i < (img.height*img.width); i += img.channels) {
        var += pow(img.img[i] - mean, 2);
    }
    return var/(img.height*img.width);
}
typedef struct {float min; float max;} MinMax;
MinMax getPixelMinMax(const Image img, const int channel) { // gives mean of pixels on a particular channel
    float min = img.img[channel];
    float max = img.img[channel];
    for (int i = channel; i < (img.height*img.width); i += img.channels) {
        if (img.img[i] < min) {
            min = img.img[i];
        }
        if (img.img[i] > max) {
            max = img.img[i];
        }
    }
    return (MinMax){.min = min, .max = max};
}