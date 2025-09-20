#pragma once

extern "C" {

#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    unsigned char bfType[2];       // File type
    unsigned int bfSize;           // File size in bytes
    unsigned short bfReserved1;    // Reserved
    unsigned short bfReserved2;    // Reserved
    unsigned int bfOffBits;        // Offset to image data, bytes
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;           // Header size in bytes
    int biWidth;                   // Width of image
    int biHeight;                  // Height of image
    unsigned short biPlanes;       // Number of color planes
    unsigned short biBitCount;     // Bits per pixel
    unsigned int biCompression;    // Compression type
    unsigned int biSizeImage;      // Image size in bytes
    int biXPelsPerMeter;           // Pixels per meter X
    int biYPelsPerMeter;           // Pixels per meter Y
    unsigned int biClrUsed;        // Number of colors
    unsigned int biClrImportant;   // Important colors
} BITMAPINFOHEADER;
#pragma pack(pop)

void write_bmp(const char *filename, const unsigned char *data, int width, int height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("cannot open image file");
        exit(1);
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    int row_padded = (width * 3 + 3) & (~3);
    int imageSize = row_padded * height;

    // Fill file header
    fileHeader.bfType[0] = 'B';
    fileHeader.bfType[1] = 'M';
    fileHeader.bfSize = 54 + imageSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = 54;

    // Fill info header
    infoHeader.biSize = 40;
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = imageSize;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    fwrite(&fileHeader, sizeof(fileHeader), 1, file);
    fwrite(&infoHeader, sizeof(infoHeader), 1, file);

    // Write the pixel data
    for (int i = height-1; i >= 0; i--) {
        fwrite(data + (width * 3 * i), 3, width, file);
        for (int j = 0; j < row_padded - width * 3; j++) {
            fputc(0, file);
        }
    }

    fclose(file);
}

}
