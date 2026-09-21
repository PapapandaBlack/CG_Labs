#include <d3dx12.h>
#include <stdexcept>

#include "gbuffer.h"
#include "fail.h"

DXGI_FORMAT GBuffer::Format(UINT index)
{
    switch (index)
    {
    case 0:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case 1:
    case 2:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    default:
        throw std::out_of_range("Invalid GBuffer target index");
    }
}

void GBuffer::Initialize(ID3D12Device* device, UINT width, UINT height, D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart, UINT srvDescriptorSize)
{
    if (device == nullptr || width == 0 || height == 0 || srvDescriptorSize == 0)
    {
        throw std::runtime_error("Invalid GBuffer initialization parameters");
    }

    FirstSRV = srvGpuStart;
    RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = TargetCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    Fail(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(RTVHeap.ReleaseAndGetAddressOf())));

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    for (UINT i = 0; i < TargetCount; ++i)
    {
        const DXGI_FORMAT format = Format(i);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = format;

        Fail(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(Targets[i].ReleaseAndGetAddressOf())));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        device->CreateRenderTargetView(Targets[i].Get(), &rtvDesc, RTV(i));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvCpuStart, static_cast<INT>(i), srvDescriptorSize);

        device->CreateShaderResourceView(Targets[i].Get(), &srvDesc, srvHandle);
    }
}

ID3D12Resource* GBuffer::Resource(UINT index) const
{
    return Targets.at(index).Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GBuffer::RTV(UINT index) const
{
    if (index >= TargetCount)
    {
        throw std::out_of_range("Invalid GBuffer RTV index");
    }

    if (!RTVHeap)
    {
        throw std::runtime_error("GBuffer is not initialized");
    }

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(index), RTVDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE GBuffer::SRVStart() const
{
    return FirstSRV;
}

void GBuffer::BeginGeometry(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil)
{
    for (UINT i = 0; i < TargetCount; ++i)
    {
        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(Targets[i].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ResourceBarrier(1, &barrier);
    }

    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[TargetCount];

    for (UINT i = 0; i < TargetCount; ++i)
    {
        renderTargets[i] = RTV(i);

        commandList->ClearRenderTargetView(renderTargets[i], clearColor, 0, nullptr);
    }

    commandList->ClearDepthStencilView(depthStencil, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    commandList->OMSetRenderTargets(TargetCount, renderTargets, FALSE, &depthStencil);
}

void GBuffer::EndGeometry(ID3D12GraphicsCommandList* commandList)
{
    for (UINT i = 0; i < TargetCount; ++i)
    {
        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(Targets[i].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->ResourceBarrier(1, &barrier);
    }
}