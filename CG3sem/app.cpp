#include "app.h"

#include <d3dcompiler.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <limits>
#include <random>
#include <algorithm>
#include <cmath>

#include <DirectXColors.h>
#include <SimpleMath.h>

#include "utils.h"
#include "vertex.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void App::EnableDebug()
{
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    Fail(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();

    ComPtr<ID3D12Debug1> debugController1;
    if (SUCCEEDED(debugController.As(&debugController1)))
    {
        debugController1->SetEnableGPUBasedValidation(true);
    }
#endif
}

void App::InitializeDevice()
{
    BackBuffFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    EnableDebug();
    Fail(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

    Fail(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));
    Fail(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

    DescSizeRTV = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DescSizeDSV = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    DescSizeCbvSrvUav = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    QualityLevels.Format = BackBuffFormat;
    QualityLevels.SampleCount = 4;
    QualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    QualityLevels.NumQualityLevels = 0;

    Fail(
        Device->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &QualityLevels,
            sizeof(QualityLevels)));
}

void App::InitializeCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    Fail(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CmdQueue)));
    Fail(Device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&DirectCmdListAlloc)));

    Fail(
        Device->CreateCommandList(
            0,
            queueDesc.Type,
            DirectCmdListAlloc.Get(),
            nullptr,
            IID_PPV_ARGS(&CmdList)));

    Fail(CmdList->Close());
}

void App::CreateSwapChain(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC swDesc = {};
    swDesc.BufferDesc.Width = fWidth;
    swDesc.BufferDesc.Height = fHeight;
    swDesc.BufferDesc.RefreshRate.Numerator = 60;
    swDesc.BufferDesc.RefreshRate.Denominator = 1;
    swDesc.BufferDesc.Format = BackBuffFormat;
    swDesc.SampleDesc.Count = 1;
    swDesc.SampleDesc.Quality = 0;
    swDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swDesc.BufferCount = 2;
    swDesc.OutputWindow = hWnd;
    swDesc.Windowed = true;
    swDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    Fail(dxgiFactory->CreateSwapChain(CmdQueue.Get(), &swDesc, &SwapChain));
}

void App::CreateRTVAndDSVDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
    RTVHeapDesc.NumDescriptors = 2;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;

    Fail(Device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&HeapRTV)));

    D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;

    Fail(Device->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&HeapDSV)));
}

//void App::CreateCBVDescriptorHeap()
//{
//    D3D12_DESCRIPTOR_HEAP_DESC CBVHeapDesc = {};
//    CBVHeapDesc.NumDescriptors = ObjectsMax;
//    CBVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//    CBVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//    CBVHeapDesc.NodeMask = 0;
//
//    Fail(Device->CreateDescriptorHeap(&CBVHeapDesc, IID_PPV_ARGS(&HeapCBV)));
//}

void App::CreateCBVDescriptorHeap()
{
    if (objParser.GetMaterials().size() > TextureMax)
    {
        throw std::runtime_error(
            "Too many materials. Maximum: " +
            std::to_string(TextureMax));
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription = {};

    heapDescription.NumDescriptors =
        ObjectsMax + TextureMax;

    heapDescription.Type =
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    heapDescription.Flags =
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    heapDescription.NodeMask = 0;

    Fail(Device->CreateDescriptorHeap(
        &heapDescription,
        IID_PPV_ARGS(&HeapCBV)));

    CreateTextures();
}

void App::CreateTextures()
{
    const auto& materials = objParser.GetMaterials();

    if (materials.empty())
    {
        throw std::runtime_error(
            "Load the model before creating textures");
    }

    if (materials.size() > TextureMax)
    {
        throw std::runtime_error(
            "Too many materials for the texture descriptor heap");
    }

    FlushCmdQueue();

    Fail(DirectCmdListAlloc->Reset());

    Fail(CmdList->Reset(
        DirectCmdListAlloc.Get(),
        nullptr));

    Textures.clear();
    Textures.reserve(materials.size());

    for (UINT i = 0;
        i < static_cast<UINT>(materials.size());
        ++i)
    {

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
            HeapCBV->GetCPUDescriptorHandleForHeapStart(),
            ObjectsMax + i,
            DescSizeCbvSrvUav);

        try
        {
            Textures.push_back(
                TextureLoader::Load(
                    Device.Get(),
                    CmdList.Get(),
                    materials[i].TexturePath,
                    srvHandle));
        }
        catch (const std::exception& error)
        {
            throw std::runtime_error(
                "Failed to create texture for material '" +
                materials[i].Name +
                "', path '" +
                materials[i].TexturePath +
                "': " +
                error.what());
        }
    }

    Fail(CmdList->Close());

    ID3D12CommandList* commandLists[] =
    {
        CmdList.Get()
    };

    CmdQueue->ExecuteCommandLists(1, commandLists);

    FlushCmdQueue();

    for (TextureResource& texture : Textures)
    {
        texture.UploadBuffer.Reset();
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetBackBuffer() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        HeapRTV->GetCPUDescriptorHandleForHeapStart(),
        curBackBuff,
        DescSizeRTV);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetDSV() const
{
    return HeapDSV->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* App::CurrentBackBuffer() const
{
    return SwapChainBuff[curBackBuff].Get();
}

void App::CreateRTV()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(HeapRTV->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < 2; i++)
    {
        Fail(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuff[i])));
        Device->CreateRenderTargetView(SwapChainBuff[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, DescSizeRTV);
    }
}

void App::CreateDSV()
{
    D3D12_RESOURCE_DESC dsDesc = {};
    dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Width = fWidth;
    dsDesc.Height = fHeight;
    dsDesc.DepthOrArraySize = 1;
    dsDesc.MipLevels = 1;
    dsDesc.Format = DepthStencilFormat;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.SampleDesc.Quality = 0;
    dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clrValue = {};
    clrValue.Format = DepthStencilFormat;
    clrValue.DepthStencil.Depth = 1.0f;
    clrValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    Fail(
        Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &dsDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clrValue,
            IID_PPV_ARGS(&BuffDSV)));

    Device->CreateDepthStencilView(BuffDSV.Get(), nullptr, GetDSV());
}

void App::SetViewport()
{
    ViewPort.TopLeftX = 0.0f;
    ViewPort.TopLeftY = 0.0f;
    ViewPort.Width = static_cast<float>(fWidth);
    ViewPort.Height = static_cast<float>(fHeight);
    ViewPort.MinDepth = 0.0f;
    ViewPort.MaxDepth = 1.0f;
}

void App::SetScissor()
{
    sRect = { 0, 0, fWidth, fHeight };
}

void App::Stats(GameTimer& gt, HWND hWnd)
{
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    if ((gt.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = static_cast<float>(frameCnt);
        float mspf = 1000.0f / fps;

        std::wstring windowText =
            L"WINDOW fps: " + std::to_wstring(fps) + L" mspf: " + std::to_wstring(mspf);

        SetWindowText(hWnd, windowText.c_str());

        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

void App::FlushCmdQueue()
{
    curFence++;
    Fail(CmdQueue->Signal(Fence.Get(), curFence));

    if (Fence->GetCompletedValue() < curFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        Fail(Fence->SetEventOnCompletion(curFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void App::BuildFallbackCube()
{
    VerticesCPU =
    {
        Vertex{ Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.f,  0.f, -1.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(-0.5f,  0.5f, -0.5f), Vector3(0.f,  0.f, -1.f), Vector2(0.f, 0.f) },
        Vertex{ Vector3(0.5f,  0.5f, -0.5f), Vector3(0.f,  0.f, -1.f), Vector2(1.f, 0.f) },
        Vertex{ Vector3(0.5f, -0.5f, -0.5f), Vector3(0.f,  0.f, -1.f), Vector2(1.f, 1.f) },

        Vertex{ Vector3(-0.5f, -0.5f,  0.5f), Vector3(0.f,  0.f,  1.f), Vector2(1.f, 1.f) },
        Vertex{ Vector3(0.5f, -0.5f,  0.5f), Vector3(0.f,  0.f,  1.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(0.5f,  0.5f,  0.5f), Vector3(0.f,  0.f,  1.f), Vector2(0.f, 0.f) },
        Vertex{ Vector3(-0.5f,  0.5f,  0.5f), Vector3(0.f,  0.f,  1.f), Vector2(1.f, 0.f) },

        Vertex{ Vector3(-0.5f, -0.5f,  0.5f), Vector3(-1.f,  0.f,  0.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(-0.5f,  0.5f,  0.5f), Vector3(-1.f,  0.f,  0.f), Vector2(0.f, 0.f) },
        Vertex{ Vector3(-0.5f,  0.5f, -0.5f), Vector3(-1.f,  0.f,  0.f), Vector2(1.f, 0.f) },
        Vertex{ Vector3(-0.5f, -0.5f, -0.5f), Vector3(-1.f,  0.f,  0.f), Vector2(1.f, 1.f) },

        Vertex{ Vector3(0.5f, -0.5f, -0.5f), Vector3(1.f,  0.f,  0.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(0.5f,  0.5f, -0.5f), Vector3(1.f,  0.f,  0.f), Vector2(0.f, 0.f) },
        Vertex{ Vector3(0.5f,  0.5f,  0.5f), Vector3(1.f,  0.f,  0.f), Vector2(1.f, 0.f) },
        Vertex{ Vector3(0.5f, -0.5f,  0.5f), Vector3(1.f,  0.f,  0.f), Vector2(1.f, 1.f) },

        Vertex{ Vector3(-0.5f,  0.5f, -0.5f), Vector3(0.f,  1.f,  0.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(-0.5f,  0.5f,  0.5f), Vector3(0.f,  1.f,  0.f), Vector2(0.f, 0.f) },
        Vertex{ Vector3(0.5f,  0.5f,  0.5f), Vector3(0.f,  1.f,  0.f), Vector2(1.f, 0.f) },
        Vertex{ Vector3(0.5f,  0.5f, -0.5f), Vector3(0.f,  1.f,  0.f), Vector2(1.f, 1.f) },

        Vertex{ Vector3(-0.5f, -0.5f,  0.5f), Vector3(0.f, -1.f,  0.f), Vector2(1.f, 0.f) },
        Vertex{ Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.f, -1.f,  0.f), Vector2(1.f, 1.f) },
        Vertex{ Vector3(0.5f, -0.5f, -0.5f), Vector3(0.f, -1.f,  0.f), Vector2(0.f, 1.f) },
        Vertex{ Vector3(0.5f, -0.5f,  0.5f), Vector3(0.f, -1.f,  0.f), Vector2(0.f, 0.f) }
    };

    IndicesCPU =
    {
        0,2,1,  0,3,2,
        4,6,5,  4,7,6,
        8,10,9, 8,11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,22,21, 20,23,22
    };

    objLoader = false;
    Path.clear();
}

void App::ParseFile()
{
    /*VerticesCPU.clear();
    IndicesCPU.clear();

    VerticesCPU = objParser.GetVertices();
    IndicesCPU = objParser.GetIndices();*/

    VerticesCPU.clear();
    IndicesCPU.clear();

    //Path = "../objs/American_flamingo.obj";
    Path = "../objs/sponza/sponza.obj";

    if (!objParser.Load(Path))
    {
        throw std::runtime_error(
            "Failed to load OBJ model: " + Path);
    }

    VerticesCPU = objParser.GetVertices();
    IndicesCPU = objParser.GetIndices();

    if (VerticesCPU.empty() || IndicesCPU.empty())
    {
        BuildFallbackCube();
        return;
    }

    Vector3 minP(
        (std::numeric_limits<float>::max)(),
        (std::numeric_limits<float>::max)(),
        (std::numeric_limits<float>::max)());

    Vector3 maxP(
        -(std::numeric_limits<float>::max)(),
        -(std::numeric_limits<float>::max)(),
        -(std::numeric_limits<float>::max)());

    for (auto& v : VerticesCPU)
    {
        minP.x = (std::min)(minP.x, v.pos.x);
        minP.y = (std::min)(minP.y, v.pos.y);
        minP.z = (std::min)(minP.z, v.pos.z);

        maxP.x = (std::max)(maxP.x, v.pos.x);
        maxP.y = (std::max)(maxP.y, v.pos.y);
        maxP.z = (std::max)(maxP.z, v.pos.z);

        if (v.normal.LengthSquared() < 0.000001f)
            v.normal = Vector3(0.f, 1.f, 0.f);
        else
            v.normal.Normalize();
    }

    const Vector3 center = 0.5f * (minP + maxP);
    const Vector3 extent = maxP - minP;
    float maxExtent = (std::max)(extent.x, (std::max)(extent.y, extent.z));
    if (maxExtent < 0.0001f)
        maxExtent = 1.0f;

    const float scale = 2.0f / maxExtent;

    for (auto& v : VerticesCPU)
    {
        v.pos = (v.pos - center) * scale;
    }

    objLoader = true;
}

void App::CreateVertexBuffer()
{
    if (VerticesCPU.empty())
        ParseFile();

    const UINT vertexBufferByteSize =
        static_cast<UINT>(sizeof(Vertex) * VerticesCPU.size());

    Fail(DirectCmdListAlloc->Reset());
    Fail(CmdList->Reset(DirectCmdListAlloc.Get(), nullptr));

    vBuffGPU = Utils::CreateDefaultBuffer(
        Device.Get(),
        CmdList.Get(),
        VerticesCPU.data(),
        vertexBufferByteSize,
        vBuffUploader);

    vBuff[0].BufferLocation = vBuffGPU->GetGPUVirtualAddress();
    vBuff[0].StrideInBytes = sizeof(Vertex);
    vBuff[0].SizeInBytes = vertexBufferByteSize;
}

void App::CreateIndexBuffer()
{
    if (IndicesCPU.empty())
        ParseFile();

    cnt = static_cast<UINT>(IndicesCPU.size());
    const UINT indexBufferByteSize =
        static_cast<UINT>(sizeof(UINT) * IndicesCPU.size());

    iBuffGPU = Utils::CreateDefaultBuffer(
        Device.Get(),
        CmdList.Get(),
        IndicesCPU.data(),
        indexBufferByteSize,
        iBuffUploader);

    iBuff.BufferLocation = iBuffGPU->GetGPUVirtualAddress();
    iBuff.Format = DXGI_FORMAT_R32_UINT;
    iBuff.SizeInBytes = indexBufferByteSize;

    Fail(CmdList->Close());

    ID3D12CommandList* cmdLists[] = { CmdList.Get() };
    CmdQueue->ExecuteCommandLists(1, cmdLists);
    FlushCmdQueue();

    Scene();
}

void App::OnMouseDown(HWND hwnd)
{
    MouseLeft = true;
    SetCapture(hwnd);
}

void App::OnMouseUp()
{
    MouseLeft = false;
    ReleaseCapture();
}

void App::OnMouseMove(WPARAM State, int dx, int dy)
{
    if (GetFocus() == nullptr)
    {
        MouseLeft = false;
        return;
    }

    if ((State & MK_LBUTTON) || MouseLeft)
    {
        CameraY += static_cast<float>(dx) * 0.003f;
        CameraX -= static_cast<float>(dy) * 0.003f;

        const float Limit = XM_PIDIV2 - 0.01f;

        if (CameraX > Limit)
            CameraX = Limit;

        if (CameraX < -Limit)
            CameraX = -Limit;
    }
}

void App::Update(const GameTimer& gt)
{
    const float cameraDt = (std::min)(gt.DeltaTime(), 0.05f);

    Vector3 forward(sinf(CameraY) * cosf(CameraX), sinf(CameraX), -cosf(CameraY) * cosf(CameraX));
    forward.Normalize();

    const Vector3 up(0.0f, 1.0f, 0.0f);

    Vector3 right = forward.Cross(up);
    right.Normalize();

    Vector3 movement = Vector3::Zero;

    if (GetFocus() != nullptr)
    {
        if (GetAsyncKeyState('W') & 0x8000)
            movement += forward;

        if (GetAsyncKeyState('S') & 0x8000)
            movement -= forward;

        if (GetAsyncKeyState('D') & 0x8000)
            movement += right;

        if (GetAsyncKeyState('A') & 0x8000)
            movement -= right;
    }

    if (movement.LengthSquared() > 0.000001f)
    {
        movement.Normalize();

        CameraPosition += movement * CameraMoveSpeed * cameraDt;
    }

    View = Matrix::CreateLookAt(CameraPosition, CameraPosition + forward, up);

    const float t = gt.TotalTime();
    Vector3 lightDir(std::cosf(t * 0.8f), -0.65f, std::sinf(t * 0.8f));
    lightDir.Normalize();

    Matrix viewProj = (View * Proj).Transpose();

    EyeConstants passData;
    passData.ViewProj = viewProj;

    passData.LightDir = Vector4(lightDir.x, lightDir.y, lightDir.z, 0.0f);
    passData.EyePos = Vector4(CameraPosition.x, CameraPosition.y, CameraPosition.z, 1.0f);
    passData.AmbientStrength = 0.22f;
    passData.SpecularStrength = 0.70f;
    passData.SpecularPower = 48.0f;
    passData.Time = gt.TotalTime();

    Eye->CopyData(0, passData);

    for (size_t i = 0; i < Objects.size(); ++i)
    {
        Objects[i]->Update(gt.DeltaTime());

        ObjectConstants objData{};
        objData.World = Objects[i]->WorldMatrix().Transpose();

        objData.Color = Objects[i]->Color();

        Obj->CopyData(static_cast<int>(i), objData);
    }
}

void App::InitBuffer()
{
    Obj = std::make_unique<Buffer<ObjectConstants>>(
        Device.Get(),
        (std::max)(1u, static_cast<UINT>(Objects.size())),
        true);

    Eye = std::make_unique<Buffer<EyeConstants>>(
        Device.Get(),
        1,
        true);
}

void App::CreateConstantBufferView()
{
    const UINT objCBByteSize = Utils::CalcConstantBufferSize(sizeof(ObjectConstants));
    const UINT passCBByteSize = Utils::CalcConstantBufferSize(sizeof(EyeConstants));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(HeapCBV->GetCPUDescriptorHandleForHeapStart());

    D3D12_GPU_VIRTUAL_ADDRESS objAddress = Obj->Resource()->GetGPUVirtualAddress();
    for (UINT i = 0; i < static_cast<UINT>(Objects.size()); ++i)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = objAddress + i * objCBByteSize;
        cbvDesc.SizeInBytes = objCBByteSize;
        Device->CreateConstantBufferView(&cbvDesc, cbvHandle);
        cbvHandle.Offset(1, DescSizeCbvSrvUav);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE passHandle(
        HeapCBV->GetCPUDescriptorHandleForHeapStart(),
        indexCBV,
        DescSizeCbvSrvUav);

    D3D12_CONSTANT_BUFFER_VIEW_DESC passDesc = {};
    passDesc.BufferLocation = Eye->Resource()->GetGPUVirtualAddress();
    passDesc.SizeInBytes = passCBByteSize;
    Device->CreateConstantBufferView(&passDesc, passHandle);
}

//void App::CreateRootSignature()
//{
//    CD3DX12_DESCRIPTOR_RANGE objCbvTable;
//    objCbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
//
//    CD3DX12_ROOT_PARAMETER slotRootParameter[2] = {};
//    slotRootParameter[0].InitAsDescriptorTable(1, &objCbvTable);
//    slotRootParameter[1].InitAsConstantBufferView(1);
//
//    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
//        2,
//        slotRootParameter,
//        0,
//        nullptr,
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    ComPtr<ID3DBlob> serializedRootSig = nullptr;
//    ComPtr<ID3DBlob> errorBlob = nullptr;
//
//    HRESULT hr = D3D12SerializeRootSignature(
//        &rootSigDesc,
//        D3D_ROOT_SIGNATURE_VERSION_1,
//        serializedRootSig.GetAddressOf(),
//        errorBlob.GetAddressOf());
//
//    if (errorBlob)
//        OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
//
//    Fail(hr);
//
//    Fail(
//        Device->CreateRootSignature(
//            0,
//            serializedRootSig->GetBufferPointer(),
//            serializedRootSig->GetBufferSize(),
//            IID_PPV_ARGS(&RootSign)));
//}

void App::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE objCbvTable;
    objCbvTable.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        1,
        0);

    CD3DX12_DESCRIPTOR_RANGE textureSrvTable;
    textureSrvTable.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,
        0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[4] = {};

    slotRootParameter[0].InitAsDescriptorTable(
        1,
        &objCbvTable);

    slotRootParameter[1].InitAsConstantBufferView(1);

    slotRootParameter[2].InitAsDescriptorTable(
        1,
        &textureSrvTable,
        D3D12_SHADER_VISIBILITY_PIXEL);

    slotRootParameter[3].InitAsConstants(
        4,
        2,
        0,
        D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC sampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        4,
        slotRootParameter,
        1,
        &sampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(),
        errorBlob.GetAddressOf());

    if (errorBlob)
    {
        OutputDebugStringA(
            static_cast<char*>(errorBlob->GetBufferPointer()));
    }

    Fail(hr);

    Fail(
        Device->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&RootSign)));
}

void App::CompileShaders()
{
    mvsByteCode = Utils::CompileShader(L"shaders.hlsl", nullptr, "VS", "vs_5_1");
    mpsByteCode = Utils::CompileShader(L"shaders.hlsl", nullptr, "PS", "ps_5_1");
}

void App::BuildLayout()
{
    Input =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void App::CreatePSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { Input.data(), static_cast<UINT>(Input.size()) };
    psoDesc.pRootSignature = RootSign.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
        mvsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
        mpsByteCode->GetBufferSize()
    };

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = BackBuffFormat;
    psoDesc.DSVFormat = DepthStencilFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    Fail(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
}

void App::InitProjectionMatrix()
{
    const float aspectRatio =
        static_cast<float>(fWidth) / static_cast<float>(fHeight);

    Proj = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(60.0f),
        aspectRatio,
        0.1f,
        100.0f);
}

void App::Draw(const GameTimer&)
{
    Fail(DirectCmdListAlloc->Reset());
    Fail(CmdList->Reset(DirectCmdListAlloc.Get(), PSO.Get()));

    CmdList->RSSetViewports(1, &ViewPort);
    CmdList->RSSetScissorRects(1, &sRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &barrier);

    CmdList->ClearRenderTargetView(GetBackBuffer(), Colors::LightSteelBlue, 0, nullptr);
    CmdList->ClearDepthStencilView(
        GetDSV(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE bb = GetBackBuffer();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSV();
    CmdList->OMSetRenderTargets(1, &bb, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = { HeapCBV.Get() };
    CmdList->SetDescriptorHeaps(1, descriptorHeaps);
    CmdList->SetGraphicsRootSignature(RootSign.Get());
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CmdList->SetGraphicsRootConstantBufferView(
        1,
        Eye->Resource()->GetGPUVirtualAddress());

    const D3D12_GPU_DESCRIPTOR_HANDLE cbvHeapStart =
        HeapCBV->GetGPUDescriptorHandleForHeapStart();

    for (const auto& obj : Objects)
    {
        obj->Draw(CmdList.Get(), cbvHeapStart, DescSizeCbvSrvUav);
    }

    CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &barrier2);

    Fail(CmdList->Close());

    ID3D12CommandList* cmdLists[] = { CmdList.Get() };
    CmdQueue->ExecuteCommandLists(1, cmdLists);

    Fail(SwapChain->Present(0, 0));
    curBackBuff = (curBackBuff + 1) % 2;

    FlushCmdQueue();
}

// Это сцена для 4ой лабы 1го семестра КГ

//void App::Scene()
//{
//    Objects.clear();
//
//    for (int x = 0; x < 3; x++) {
//        for (int y = 0; y < 3; y++) {
//            for (int z = 0; z < 3; z++) {
//                auto cube = std::make_unique<MeshObject>(x * 9 + y * 3 + z, vBuff[0], iBuff, cnt);
//                cube->SetPosition(Vector3(-0.6f + x * 0.6f, -0.6f + y * 0.6f, -0.6f + z * 0.6f));
//                cube->SetScale(Vector3(0.5f, 0.5f, 0.5f));
//                cube->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
//
//                cube->SetColor(Vector4(0.28f * x, 0.19f * y, 0.06f * z, 0.7f));
//
//                Objects.push_back(std::move(cube));
//            }
//        }
//    }
//}

void App::Scene()
{
    Objects.clear();

    auto mesh = std::make_unique<MeshObject>(0, vBuff[0], iBuff, cnt);

    mesh->SetMaterials(objParser.GetParts(), objParser.GetMaterials(), ObjectsMax);

    mesh->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    mesh->SetScale(Vector3(1.0f, 1.0f, 1.0f));
    // Это для отрисовки фламинго (он почему-то перевернулся)
    //mesh->SetRotation(Vector3(-XM_PIDIV2, 0.0f, 0.0f));
    mesh->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
    mesh->SetRotationSpeedY(0.0f);

    mesh->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    Objects.push_back(std::move(mesh));
}