#ifndef OBJ
#define OBJ

#include <d3d12.h>
#include <d3dx12.h>
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

class GameObj
{
protected:
    UINT cbvIndex = 0;

    Vector3 pos = Vector3::Zero;
    Vector3 rot = Vector3::Zero;
    Vector3 scl = Vector3::One;

    Matrix WrldMatrix = Matrix::Identity;
    Vector4 Col = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    float rotSpeed = 0.0f;
    bool isWorldDirty = true;

    void UpdateWorldMatrix();

public:
    explicit GameObj(UINT cbvIndex);
    virtual ~GameObj() = default;

    virtual void Update(float dt);

    virtual void Draw(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapStart,
        UINT cbvDescriptorSize) const = 0;

    void SetPosition(const Vector3& position);
    void SetRotation(const Vector3& rotation);
    void SetScale(const Vector3& scale);
    void SetColor(const Vector4& color);
    void SetRotationSpeedY(float speed);

    const Matrix& WorldMatrix() const { return WrldMatrix; }
    const Vector4& Color() const { return Col; }
    UINT CBVIndex() const { return cbvIndex; }
};

class MeshObject final : public GameObj
{
private:
    D3D12_VERTEX_BUFFER_VIEW m_vbv{};
    D3D12_INDEX_BUFFER_VIEW m_ibv{};
    UINT m_indexCount = 0;

public:
    MeshObject(
        UINT cbvIndex,
        const D3D12_VERTEX_BUFFER_VIEW& vbv,
        const D3D12_INDEX_BUFFER_VIEW& ibv,
        UINT indexCount);

    void Draw(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapStart,
        UINT cbvDescriptorSize) const override;
};

#endif