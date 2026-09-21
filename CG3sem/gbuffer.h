#ifndef GBUFFER
#define GBUFFER

#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <array>

using namespace Microsoft::WRL;

class GBuffer
{
public:
    static constexpr UINT TargetCount = 3;

    void Initialize(ID3D12Device* device, UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart, UINT srvDescriptorSize);

    ID3D12Resource* Resource(UINT index) const;

    D3D12_CPU_DESCRIPTOR_HANDLE RTV(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE SRVStart() const;

    static DXGI_FORMAT Format(UINT index);

    void BeginGeometry(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil);
    void EndGeometry(ID3D12GraphicsCommandList* commandList);

private:
    std::array<ComPtr<ID3D12Resource>,TargetCount> Targets;

    ComPtr<ID3D12DescriptorHeap> RTVHeap;

    UINT RTVDescriptorSize = 0;

    D3D12_GPU_DESCRIPTOR_HANDLE FirstSRV{};
};

#endif