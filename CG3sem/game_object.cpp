#include "game_object.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

GameObj::GameObj(UINT cbvIndex) : cbvIndex(cbvIndex) {}

void GameObj::Update(float dt)
{
    rot.y += rotSpeed * dt;
    isWorldDirty = true;

    UpdateWorldMatrix();
}

void GameObj::UpdateWorldMatrix()
{
    if (!isWorldDirty) return;

    WrldMatrix = Matrix::CreateScale(scl) *
        Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z) *
        Matrix::CreateTranslation(pos);

    isWorldDirty = false;
}

void GameObj::SetPosition(const Vector3& position) { pos = position; isWorldDirty = true; }
void GameObj::SetRotation(const Vector3& rotation) { rot = rotation; isWorldDirty = true; }
void GameObj::SetScale(const Vector3& scale) { scl = scale; isWorldDirty = true; }
void GameObj::SetColor(const Vector4& color) { Col = color; }
void GameObj::SetRotationSpeedY(float speed) { rotSpeed = speed; }


MeshObject::MeshObject(
    UINT cbvIndex,
    const D3D12_VERTEX_BUFFER_VIEW& vbv,
    const D3D12_INDEX_BUFFER_VIEW& ibv,
    UINT indexCount)
    : GameObj(cbvIndex), m_vbv(vbv), m_ibv(ibv), m_indexCount(indexCount)
{
}

void MeshObject::Draw(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapStart,
    UINT cbvDescriptorSize) const
{
    cmdList->IASetVertexBuffers(0, 1, &m_vbv);
    cmdList->IASetIndexBuffer(&m_ibv);

    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(cbvHeapStart);
    cbvHandle.Offset(static_cast<INT>(cbvIndex), cbvDescriptorSize);

    cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}