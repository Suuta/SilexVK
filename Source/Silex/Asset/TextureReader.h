
#pragma once

#include "Core/Core.h"


namespace Silex
{
    struct TextureSourceData
    {
        int32 width    = 0;
        int32 height   = 0;
        int32 channels = 0;
        int64 byteSize = 0;

        void* pixels  = nullptr;
    };

    struct TextureReader
    {
        TextureReader();
        ~TextureReader();

        byte*  Read(const char* path, bool flipOnRead = false);
        float* ReadHDR(const char* path, bool flipOnRead = false);

        bool IsHDR(const char* path);
        void Unload(void* data);

        TextureSourceData data;
    };

    struct TextureWriter
    {
        TextureWriter();
        ~TextureWriter();

        bool WritePNG(const char* filePath, uint32 width, uint32 height, const void* data);
    };
}

