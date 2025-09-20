#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

namespace _READ_BMP {

#pragma pack(push, 1)
	struct BMPFileHeader {
		uint16_t file_type;     // File identifier (should be 0x4D42 'BM')
		uint32_t file_size;     // Size of the file in bytes
		uint16_t reserved1;
		uint16_t reserved2;
		uint32_t pixel_offset;  // Offset to the start of the pixel data
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct BMPInfoHeader {
		uint32_t header_size;   // Size of this header (should be 40)
		int32_t width;          // Image width in pixels
		int32_t height;         // Image height in pixels
		uint16_t color_planes;  // Number of color planes (must be 1)
		uint16_t bits_per_pixel;// Bits per pixel (e.g., 24 for 24-bit color)
		uint32_t compression;   // Compression method (0 for uncompressed)
		uint32_t image_size;    // Image size in bytes (can be 0 for uncompressed)
		int32_t x_pixels_per_meter;
		int32_t y_pixels_per_meter;
		uint32_t total_colors;
		uint32_t important_colors;
	};
#pragma pack(pop)

};

void read_bmp(const char *filename, unsigned char **out, int *width, int *height) {
    std::ifstream file(filename, std::ios::binary);

    if(!file){
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

	_READ_BMP::BMPFileHeader file_header;
	_READ_BMP::BMPInfoHeader info_header;

    file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if(file_header.file_type != 0x4D42){
        std::cerr << "Error: Not a valid BMP file." << std::endl;
        file.close();
        return;
    }

    file.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if(info_header.bits_per_pixel != 24){
        std::cerr << "Error: Only 24-bit BMP images are supported." << std::endl;
        file.close();
        return;
    }

    *width = info_header.width;
    *height = info_header.height;

	unsigned char *data = new unsigned char[info_header.width*info_header.height*3];
	*out = data;

    int row_padding = (4-(info_header.width*3)%4)%4;
    int image_row_size = info_header.width*3;

    file.seekg(file_header.pixel_offset, std::ios::beg);

	for(int y=*height-1; y>=0; --y){
		for(int x=0; x<*width; ++x){
			for(int z=0; z<3; ++z){
				unsigned char c;
				file.read(reinterpret_cast<char*>(&c), 1);
				data[3*(x+y*(*width))+z] = c;
			}
        }

        file.seekg(row_padding, std::ios::cur);
    }
    
    file.close();
}
