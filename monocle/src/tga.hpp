// Copyright(c) 2019 Nick Klingensmith (@koujaku). All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy of this license, see < https://opensource.org/licenses/MIT >

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <filesystem>

/// <summary> Writes an uncompressed 24 or 32 bit .tga image to the indicated file! </summary>
/// <param name='filename'>I'd recommended you add a '.tga' to the end of this filename.</param>
/// <param name='dataBGRA'>A chunk of color data, one channel per byte, ordered as BGRA. Size should be width*height*dataChanels.</param>
/// <param name='dataChannels'>The number of channels in the color data. Use 1 for grayscale, 3 for BGR, and 4 for BGRA.</param>
/// <param name='fileChannels'>The number of color channels to write to file. Must be 3 for BGR, or 4 for BGRA. Does NOT need to match dataChannels.</param>
void tga_write(const char* filename,
               uint32_t width,
               uint32_t height,
               uint8_t* dataBGRA,
               uint8_t dataChannels = 4,
               uint8_t fileChannels = 3)
{
    std::filesystem::path file_path{filename};
    if (!file_path.parent_path().empty())
        std::filesystem::create_directories(file_path.parent_path());
    std::ofstream file{file_path};

    // You can find details about TGA headers here: http://www.paulbourke.net/dataformats/tga/
    // clang-format off
	uint8_t header[18] = { 0,0,2,0,0,0,0,0,0,0,0,0, (uint8_t)(width%256), (uint8_t)(width/256), (uint8_t)(height%256), (uint8_t)(height/256), (uint8_t)(fileChannels*8), 0x20 };
    // clang-format on
    file.write((char*)header, sizeof header);

    for (uint32_t i = 0; i < width * height; i++)
        for (uint32_t b = 0; b < fileChannels; b++)
            file.put(dataBGRA[(i * dataChannels) + (b % dataChannels)]);
}