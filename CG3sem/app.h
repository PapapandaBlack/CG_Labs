#ifndef APP
#define APP

#include <Windows.h>
#include <vector>
#include <memory>
#include <string>
#include <d3d12.h>
#include <DirectXHelpers.h>
#include <dxgi1_6.h>
#include <d3dx12.h>

#include "buffer.h"
#include "vertex.h"
#include "game_timer.h"
#include "game_object.h"
#include "object_loader.h"
#include "texture_model.h"
#include "texture_loader.h"
#include "render_system.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

class App
{
public:
    void InitializeDevice();
    void InitializeCommandObjects();
    void CreateSwapChain(HWND hWnd);
    void CreateRTVAndDSVDescriptorHeaps();
    void CreateCBVDescriptorHeap();

    D3D12_CPU_DESCRIPTOR_HANDLE GetBackBuffer() const;
    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;

    void CreateRTV();
    void CreateDSV();
    void SetViewport();
    void SetScissor();

    void Stats(GameTimer& gt, HWND hWnd);
    void Draw(const GameTimer& gt);

    void FlushCmdQueue();

    void InitProjectionMatrix();
    void CreateVertexBuffer();
    void CreateIndexBuffer();

    void OnMouseDown(HWND hWnd);
    void OnMouseUp();
    void OnMouseMove(WPARAM btnState, int dx, int dy);

    void Update(const GameTimer& gt);

    void InitBuffer();
    void CreateConstantBufferView();

    void CreateRootSignature();
    void CompileShaders();
    void BuildLayout();

    void CreatePSO();

    void ParseFile();

    ComPtr<ID3D12Device> GetDevice() const { return Device; }
    ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return CmdList; }

private:
    void EnableDebug();
    void Scene();
    void BuildFallbackCube();

    void CreateTextures();

private:
    DXGI_FORMAT BackBuffFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int fWidth = 900;
    int fHeight = 600;

    ComPtr<IDXGIFactory4> dxgiFactory;
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12Fence> Fence;

    UINT64 curFence = 0;
    UINT DescSizeRTV = 0;
    UINT DescSizeDSV = 0;
    UINT DescSizeCbvSrvUav = 0;
    UINT cnt = 0;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels{};

    ComPtr<ID3D12CommandQueue> CmdQueue;
    ComPtr<ID3D12CommandAllocator> DirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> CmdList;

    ComPtr<IDXGISwapChain> SwapChain;

    ComPtr<ID3D12DescriptorHeap> HeapRTV;
    ComPtr<ID3D12DescriptorHeap> HeapDSV;
    ComPtr<ID3D12DescriptorHeap> HeapCBV;
    int curBackBuff = 0;

    ComPtr<ID3D12Resource> SwapChainBuff[2];
    ComPtr<ID3D12Resource> BuffDSV;
    ComPtr<ID3D12Resource> vBuffUploader = nullptr;
    ComPtr<ID3D12Resource> vBuffGPU = nullptr;
    ComPtr<ID3D12Resource> iBuffUploader = nullptr;
    ComPtr<ID3D12Resource> iBuffGPU = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vBuff[1]{};
    D3D12_INDEX_BUFFER_VIEW iBuff{};

    D3D12_VIEWPORT ViewPort{};
    D3D12_RECT sRect{};

    std::unique_ptr<Buffer<ObjectConstants>> Obj;
    std::unique_ptr<Buffer<EyeConstants>> Eye;

    ComPtr<ID3D12RootSignature> RootSign;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> Input;

    ComPtr<ID3D12PipelineState> PSO;

    Matrix View = Matrix::Identity;
    Matrix Proj = Matrix::Identity;

    POINT MousePos{};
    bool MouseLeft = false;
    Vector3 CameraPosition = Vector3(0.0f, 0.0f, 3.0f);
    float CameraX = 0.0f;
    float CameraY = 0.0f;
    float CameraMoveSpeed = 1.0f;

    std::vector<std::unique_ptr<GameObj>> Objects;
    static constexpr UINT ObjectsMax = 64;
    UINT indexCBV = ObjectsMax - 1;

    //ObjLoader objParser;
    TexturedModel objParser;
    std::vector<Vertex> VerticesCPU;
    std::vector<UINT> IndicesCPU;
    bool objLoader = false;
    std::string Path;

    std::vector<TextureResource> Textures;
    static constexpr UINT TextureMax = 256;

    static constexpr UINT GBufferSrvStart = ObjectsMax + TextureMax;
    static constexpr UINT DescriptorCount = GBufferSrvStart + GBuffer::TargetCount;

    RenderingSystem Renderer;
};

#endif
