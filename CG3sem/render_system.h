#ifndef RENDER
#define RENDER

#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <memory>
#include <vector>

#include "gbuffer.h"

using namespace Microsoft::WRL;

class GameObj;

class RenderingSystem
{
private:
    GBuffer GeometryBuffer;

    ComPtr<ID3D12RootSignature> LightingRootSignature;
    ComPtr<ID3D12PipelineState> LightingPSO;

public:
    void Initialize(ID3D12Device* device, UINT width, UINT height, ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, UINT gBufferSrvStart);
    void DrawOpaque(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, const std::vector<std::unique_ptr<GameObj>>& objects);

    void CreateLightingPipeline(ID3D12Device* device, DXGI_FORMAT backBufferFormat);
    void BeginGeometry(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil);
    void EndGeometry(ID3D12GraphicsCommandList* commandList);
    void DrawLighting(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE backBuffer, D3D12_GPU_VIRTUAL_ADDRESS passBufferAddress);
};

#endif