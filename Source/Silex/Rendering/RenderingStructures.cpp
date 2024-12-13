
#include "PCH.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingStructures.h"


namespace Silex
{
    //==============================================================
    // テクスチャ2D
    //==============================================================
    Texture2D::Texture2D(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // テクスチャ2D配列
    //==============================================================
    Texture2DArray::Texture2DArray(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // テクスチャキューブ
    //==============================================================
    TextureCube::TextureCube(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // テクスチャビュー
    //==============================================================
    TextureView::TextureView(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // サンプラー
    //==============================================================
    Sampler::Sampler(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // バッファ
    //==============================================================
    Buffer::Buffer(uint32 frames)
    {
        handle.resize(frames);
    }

    void* Buffer::GetMappedPointer(uint32 frameIndex)
    {
        return Renderer::Get()->GetMappedPointer(handle[frameIndex]);
    }

    //==============================================================
    // ユニフォームバッファ
    //==============================================================
    UniformBuffer::UniformBuffer(uint32 frames)
    {
        handle.resize(frames);
    }

    void UniformBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    //==============================================================
    // ストレージバッファ
    //==============================================================
    StorageBuffer::StorageBuffer(uint32 frames)
    {
        handle.resize(frames);
    }

    void StorageBuffer::SetData(const void* data, uint64 writeByteSize)
    {
        uint32 frameindex = Renderer::Get()->GetCurrentFrameIndex();
        Renderer::Get()->UpdateBufferData(handle[frameindex], data, writeByteSize);
    }

    //==============================================================
    // 頂点バッファ
    //==============================================================
    VertexBuffer::VertexBuffer(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // インデックスバッファ
    //==============================================================
    IndexBuffer::IndexBuffer(uint32 frames)
    {
        handle.resize(frames);
    }

    //==============================================================
    // デスクリプターセット
    //==============================================================
    DescriptorSet::DescriptorSet(uint32 frames)
    {
        handle.resize(frames);
        descriptorSetInfo.resize(frames);
    }

    void DescriptorSet::Flush()
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            Renderer::Get()->UpdateDescriptorSet(handle[i], descriptorSetInfo[i]);
        }
    }

    void DescriptorSet::SetResource(uint32 binding, TextureView* view, Sampler* sampler)
    {
        // 現状、サンプラーはCPUからの書き込みをせず、ダブルバッファリングをしていないが、
        // 他のデスクリプター（ubo, ssbo）がダブルバッファリングでCPUからの書き込みを行っているので
        // 空の参照デスクリプターを含む状態でデスクリプターを更新できないので、
        // 他のデスクリプターに合わせる形で、サンプラーも同じデスクリプターの参照で更新する
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindTexture(binding, DESCRIPTOR_TYPE_IMAGE_SAMPLER, view->GetHandle(0), sampler->GetHandle(0));
        }
    }

    void DescriptorSet::SetResource(uint32 binding, UniformBuffer* uniformBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(binding, DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer->GetHandle(i));
        }
    }

    void DescriptorSet::SetResource(uint32 binding, StorageBuffer* storageBuffer)
    {
        for (uint32 i = 0; i < descriptorSetInfo.size(); i++)
        {
            descriptorSetInfo[i].BindBuffer(binding, DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBuffer->GetHandle(i));
        }
    }
}
