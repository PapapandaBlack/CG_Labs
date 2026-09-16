#ifndef BUFFER
#define BUFFER

#include <iostream>
#include <SimpleMath.h>

#include "utils.h"
#include "fail.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

struct EyeConstants
{
    Matrix ViewProj = Matrix::Identity;
    Vector4 LightDir = Vector4(0.0f, 0.45f, 0.45f, 0.0f);
    Vector4 EyePos = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    float AmbientStrength = 0.2f;
    float SpecularStrength = 0.6f;
    float SpecularPower = 15.0f;
    float Padding = 0.0f;
};

struct ObjectConstants
{
    Matrix World = Matrix::Identity;
    Vector4 Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
};

template<typename T>
class Buffer
{
public:
    Buffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        IsConstantBuffer(isConstantBuffer)
    {
        ElementByteSize = sizeof(T);

        if (isConstantBuffer)
            ElementByteSize = Utils::CalcConstantBufferSize(sizeof(T));
        CD3DX12_HEAP_PROPERTIES heapPr = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(ElementByteSize * elementCount);
        Fail(device->CreateCommittedResource(
            &heapPr,
            D3D12_HEAP_FLAG_NONE,
            &rd,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&Buff)));

        Fail(Buff->Map(0, nullptr, reinterpret_cast<void**>(&MappedData)));
        std::cout << "Constant buffer  for " << elementCount << " element(s) is created...\n" ;
    }

    Buffer(const Buffer& rhs) = delete;
    Buffer& operator=(const Buffer& rhs) = delete;
    ~Buffer()
    {
        if (Buff != nullptr)
            Buff->Unmap(0, nullptr);

        MappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return Buff.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&MappedData[elementIndex * ElementByteSize], &data, sizeof(T));
    }

private:
    ComPtr<ID3D12Resource> Buff;
    BYTE* MappedData = nullptr;

    UINT ElementByteSize = 0;
    bool IsConstantBuffer = false;
};

#endif
