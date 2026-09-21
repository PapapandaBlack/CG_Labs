#include <stdexcept>
#include <d3dcompiler.h>

#include "render_system.h"
#include "game_object.h"
#include "utils.h"
#include "fail.h"

void RenderingSystem::Initialize(ID3D12Device* device, UINT width, UINT height, ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, UINT gBufferSrvStart)
{
    if (device == nullptr || descriptorHeap == nullptr || descriptorSize == 0)
    {
        throw std::runtime_error("RenderingSystem requires valid initialization parameters");
    }

    const D3D12_DESCRIPTOR_HEAP_DESC heapDesc = descriptorHeap->GetDesc();

    if (heapDesc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || (heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) == 0)
    {
        throw std::runtime_error("GBuffer requires a shader-visible CBV/SRV/UAV heap");
    }

    if (gBufferSrvStart > heapDesc.NumDescriptors || GBuffer::TargetCount > heapDesc.NumDescriptors - gBufferSrvStart)
    {
        throw std::runtime_error("Not enough descriptors for GBuffer");
    }

    const CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStart(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(gBufferSrvStart), descriptorSize);
    const CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStart(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(gBufferSrvStart), descriptorSize);

    GeometryBuffer.Initialize(device, width, height, cpuStart, gpuStart, descriptorSize);
}

void RenderingSystem::DrawOpaque(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, const std::vector<std::unique_ptr<GameObj>>& objects)
{
    if (commandList == nullptr || descriptorHeap == nullptr)
    {
        throw std::runtime_error("RenderingSystem requires a command list and a descriptor heap");
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE heapStart =
        descriptorHeap->GetGPUDescriptorHandleForHeapStart();

    for (const auto& object : objects)
    {
        object->Draw(commandList, heapStart, descriptorSize);
    }
}

void RenderingSystem::CreateLightingPipeline(ID3D12Device* device, DXGI_FORMAT backBufferFormat)
{
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBuffer::TargetCount, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootDesc(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    Microsoft::WRL::ComPtr<ID3DBlob> serialized;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;

    const HRESULT result = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, serialized.GetAddressOf(), errors.GetAddressOf());

    if (errors)
    {
        OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
    }

    Fail(result);
    Fail(device->CreateRootSignature( 0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(LightingRootSignature.ReleaseAndGetAddressOf())));

    const auto vertexShader = Utils::CompileShader(L"deferred_lighting.hlsl", nullptr, "VS", "vs_5_1");
    const auto pixelShader = Utils::CompileShader(L"deferred_lighting.hlsl",nullptr, "PS", "ps_5_1");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = LightingRootSignature.Get();
    psoDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    psoDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = backBufferFormat;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;

    Fail(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(LightingPSO.ReleaseAndGetAddressOf())));
}

void RenderingSystem::BeginGeometry(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil)
{
    GeometryBuffer.BeginGeometry(commandList, depthStencil);
}

void RenderingSystem::EndGeometry(ID3D12GraphicsCommandList* commandList)
{
    GeometryBuffer.EndGeometry(commandList);
}

void RenderingSystem::DrawLighting(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE backBuffer, D3D12_GPU_VIRTUAL_ADDRESS passBufferAddress)
{
    commandList->OMSetRenderTargets(1, &backBuffer, FALSE, nullptr);
    commandList->SetPipelineState(LightingPSO.Get());
    commandList->SetGraphicsRootSignature(LightingRootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(0, GeometryBuffer.SRVStart());
    commandList->SetGraphicsRootConstantBufferView(1, passBufferAddress);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}