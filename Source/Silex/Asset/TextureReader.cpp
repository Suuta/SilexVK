
#include "PCH.h"
#include "Asset/TextureReader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


namespace Silex
{
    static void* _Read(const char* path, bool hdr, bool flipOnRead, TextureReader* reader)
    {
        bool isHDR = stbi_is_hdr(path);
        if (isHDR != hdr)
        {
            SL_LOG_ERROR("{} は破損しているか、期待しているファイル形式ではありません", path);
            return nullptr;
        }

        stbi_set_flip_vertically_on_load(flipOnRead);

        reader->data.pixels = isHDR?
            (void*)stbi_loadf(path, &reader->data.width, &reader->data.height, &reader->data.channels, 4):
            (void*)stbi_load(path,  &reader->data.width, &reader->data.height, &reader->data.channels, 4);

        if (!reader->data.pixels)
        {
            SL_LOG_ERROR("{} が見つからなかったか、データが破損しています", path);
        }

        reader->data.byteSize = reader->data.width * reader->data.height * 4 * (isHDR? sizeof(float) : sizeof(byte));
        return reader->data.pixels;
    }


    //=====================================================================
    // TextureReader
    //=====================================================================
    TextureReader::TextureReader()
    {
    }

    // 読み込んだテクスチャデータは、変数のスコープ内のみ有効
    // 自動で解放されるが、Unloadで明示的に解放もできる
    TextureReader::~TextureReader()
    {
        Unload(data.pixels);
    }

    byte* TextureReader::Read(const char* path, bool flipOnRead)
    {
        return (byte*)_Read(path, false, flipOnRead, this);
    }

    float* TextureReader::ReadHDR(const char* path, bool flipOnRead)
    {
        return (float*)_Read(path, true, flipOnRead, this);
    }

    bool TextureReader::IsHDR(const char* path)
    {
        return stbi_is_hdr(path);

    }
    void TextureReader::Unload(void* pixelData)
    {
        if (pixelData)
        {
            stbi_image_free(pixelData);
            pixelData = nullptr;
            data.pixels = nullptr;
        }
    }


    //=====================================================================
    // TextureWriter
    //=====================================================================
    TextureWriter::TextureWriter()
    {
    }

    TextureWriter::~TextureWriter()
    {
    }

    bool TextureWriter::WritePNG(const char* filePath, uint32 width, uint32 height, const void* data)
    {
        bool result = stbi_write_png(filePath, width, height, 4, data, 0);
        if (!result)
        {
            SL_LOG_ERROR("{} は破損しているか、期待しているファイル形式ではありません", filePath);
            return false;
        }

        return true;
    }
}