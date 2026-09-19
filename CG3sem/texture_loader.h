#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <string>

using namespace Microsoft::WRL;

struct TextureResource
{
    ComPtr<ID3D12Resource> Resource;
    ComPtr<ID3D12Resource> UploadBuffer;
};

class TextureLoader
{
public:
    static TextureResource Load(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& filename,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);
};
