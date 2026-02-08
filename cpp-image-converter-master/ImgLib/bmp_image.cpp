#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t type = 0x4D42; 
    uint32_t file_size = 0;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;
    uint32_t offset = 0;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40;
    int32_t width = 0;
    int32_t height = 0;
    uint16_t planes = 1;
    uint16_t bit_count = 24;
    uint32_t compression = 0;
    uint32_t image_size = 0;
    int32_t x_pels_per_meter = 11811;
    int32_t y_pels_per_meter = 11811;
    uint32_t colors_used = 0;
    uint32_t colors_important = 0x1000000;
}
PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) return false;

    const int width = image.GetWidth();
    const int height = image.GetHeight();
    const int stride = GetBMPStride(width);
    
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    
    file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * height;
    file_header.offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    
    info_header.width = width;
    info_header.height = height;
    info_header.image_size = stride * height;
    
    if (!out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header))) {
        return false;
    }
    if (!out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header))) {
        return false;
    }
    
    vector<char> buffer(stride);
    
    for (int y = height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        for (int x = width * 3; x < stride; ++x) {
            buffer[x] = 0;
        }
        if (!out.write(buffer.data(), stride)) {
            return false;
        }
    }
    
    return true;
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) return {};
    
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    
    if (!in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header))) {
        return {};
    }
    if (!in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header))) {
        return {};
    }
    
    if (file_header.type != 0x4D42 || 
        info_header.header_size != 40 ||
        info_header.bit_count != 24 || 
        info_header.compression != 0) {
        return {};
    }
    
    const int width = info_header.width;
    int real_height = info_header.height;
    bool top_down = false;
    
    if (real_height < 0) {
        top_down = true;
        real_height = -real_height;
    }
    
    const int stride = GetBMPStride(width);
    
    Image result(width, real_height, Color::Black());
    vector<char> buffer(stride);
    
    if (top_down) {
        for (int y = 0; y < real_height; ++y) {
            if (!in.read(buffer.data(), stride)) {
                return {};
            }
            Color* line = result.GetLine(y);
            for (int x = 0; x < width; ++x) {
                line[x].r = static_cast<byte>(buffer[x*3+2]);
                line[x].g = static_cast<byte>(buffer[x*3+1]);
                line[x].b = static_cast<byte>(buffer[x*3]);
            }
        }
    } else {
        for (int y = real_height - 1; y >= 0; --y) {
            if (!in.read(buffer.data(), stride)) {
                return {};
            }
            Color* line = result.GetLine(y);
            for (int x = 0; x < width; ++x) {
                line[x].r = static_cast<byte>(buffer[x*3+2]);
                line[x].g = static_cast<byte>(buffer[x*3+1]);
                line[x].b = static_cast<byte>(buffer[x*3]);
            }
        }
    }
    
    return result;
}

}  // namespace img_lib
