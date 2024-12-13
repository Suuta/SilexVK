#pragma once

#include "Asset/Asset.h"
#include "Rendering/RenderingCore.h"
#include "Rendering/Material.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


namespace Silex
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    struct MeshTexture
    {
        uint32      Albedo = 0;
        std::string Path;
    };

    //============================================
    // メッシュの頂点情報クラス
    //--------------------------------------------
    // 頂点データ・インデックスデータの管理
    //============================================
    class MeshSource : public Class
    {
        SL_CLASS(MeshSource, Class)

    public:

        MeshSource(uint64 numVertex, Vertex* vertices, uint64 numIndex, uint32* indices, uint32 materialIndex = 0);
        MeshSource(std::vector<Vertex>& vertices, std::vector<uint32>& indices, uint32 materialIndex = 0);
        ~MeshSource();

        void Bind()   const;
        void Unbind() const;

        bool      HasIndex()         const { return hasIndex;          }
        uint32    GetMaterialIndex() const { return materialIndex;     }
        glm::mat4 GetTransform()     const { return relativeTransform; }

        uint64        GetVertexCount()   const { return vertexCount;       }
        uint64        GetIndexCount()    const { return indexCount;        }
        VertexBuffer* GetVertexBuffer()  const { return vertexBuffer;      }
        IndexBuffer*  GetIndexBuffer()   const { return indexBuffer;       }

        void SetTransform(const glm::mat4& matrix) { relativeTransform = matrix; }

    private:

        bool          hasIndex          = false;
        uint32        materialIndex     = 0;
        uint32        vertexCount       = 0;
        uint32        indexCount        = 0;
        VertexBuffer* vertexBuffer      = nullptr;
        IndexBuffer*  indexBuffer       = nullptr;
        glm::mat4     relativeTransform = {};

    private:

        friend class Mesh;
    };


    //===========================================
    // メッシュソースのリスト保持するクラス
    //-------------------------------------------
    // Unity のようにインポート時に、メッシュソース単位で
    // エンティティの階層構造を構築して、エンティティ毎に
    // メッシュをアタッチし、描画を行うのが理想だが、
    // 現状は、1つのエンティティがソース全体を描画する
    //===========================================
    class Mesh : public Class
    {
        SL_CLASS(Mesh, Class)

    public:

        Mesh();
        Mesh(const Mesh&) = default;

        ~Mesh();

        void Load(const std::filesystem::path& filePath);
        void Unload();
        void AddSource(MeshSource* source);

        // プリミティブ
        // void SetPrimitiveType(rhi::PrimitiveType type) { primitiveType = type; }
        // rhi::PrimitiveType GetPrimitiveType()          { return primitiveType; }

        // サブメッシュ
        std::vector<MeshSource*>& GetMeshSources()  { return subMeshes;        }
        MeshSource* GetMeshSource(uint32 index = 0) { return subMeshes[index]; }

        // テクスチャ
        std::unordered_map<uint32, MeshTexture>& GetTextures() { return textures;        }
        MeshTexture& GetTexture(uint32 index)                  { return textures[index]; }

        uint32 GetMaterialSlotCount() const { return numMaterialSlot; };

    private:

        void        ProcessNode(aiNode* node, const aiScene* scene, const std::string& path);
        MeshSource* ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& path);
        void        LoadMaterialTextures(uint32 materialInddex, aiMaterial* mat, aiTextureType type, const std::string& path);

    private:

        std::unordered_map<uint32, MeshTexture> textures;
        std::vector<MeshSource*>                subMeshes;
        uint32                                  numMaterialSlot;

        //rhi::PrimitiveType primitiveType = rhi::PrimitiveType::Triangle;

        friend class MeshSource;
    };





    struct MeshFactory
    {
        static Mesh* Cube();
        static Mesh* Sphere();
        static Mesh* Monkey();
        static Mesh* Sponza();
    };
}
