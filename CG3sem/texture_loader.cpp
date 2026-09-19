#include <d3dx12.h>
#include <wincodec.h>
#include <vector>
#include <stdexcept>

#include "texture_loader.h"

//#pragma comment(lib, "windowscodecs.lib")
//#pragma comment(lib, "ole32.lib")

using namespace Microsoft::WRL;

static void CheckResult(HRESULT result, const char* operation)
{
    if (FAILED(result))
    {
        throw std::runtime_error(
            std::string(operation) +
            ". HRESULT: " +
            std::to_string(static_cast<long>(result)));
    }
}

static std::wstring ToWideString(const std::string& text)
{
    if (text.empty())
    {
        return L"";
    }

    const int length = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        -1,
        nullptr,
        0);

    if (length == 0)
    {
        throw std::runtime_error("Invalid UTF-8 texture path");
    }

    std::wstring result(length, L'\0');

    const int convertedLength = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        -1,
        &result[0],
        length);

    if (convertedLength == 0)
    {
        throw std::runtime_error(
            "Failed to convert texture path");
    }

    result.pop_back();

    return result;
}

static void ReadImage(
    const std::string& filename,
    UINT& width,
    UINT& height,
    std::vector<BYTE>& pixels)
{
    const HRESULT comResult = CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED);

    if (comResult != RPC_E_CHANGED_MODE)
    {
        CheckResult(comResult, "CoInitializeEx");
    }

    const bool mustUninitialize = SUCCEEDED(comResult);

    try
    {
        ComPtr<IWICImagingFactory> factory;

        CheckResult(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(factory.GetAddressOf())),
            "Create WIC factory");

        const std::wstring wideFilename =
            ToWideString(filename);

        ComPtr<IWICBitmapDecoder> decoder;

        CheckResult(
            factory->CreateDecoderFromFilename(
                wideFilename.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.GetAddressOf()),
            "Open texture image");

        ComPtr<IWICBitmapFrameDecode> frame;

        CheckResult(
            decoder->GetFrame(
                0,
                frame.GetAddressOf()),
            "Get image frame");

        CheckResult(
            frame->GetSize(&width, &height),
            "Get image size");

        if (width == 0 ||
            height == 0 ||
            width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
            height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        {
            throw std::runtime_error(
                "Invalid texture dimensions");
        }

        ComPtr<IWICFormatConverter> converter;

        CheckResult(
            factory->CreateFormatConverter(
                converter.GetAddressOf()),
            "Create image converter");

        CheckResult(
            converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom),
            "Convert image to RGBA");

        const UINT rowSize = width * 4;
        const UINT imageSize = rowSize * height;

        pixels.resize(imageSize);

        CheckResult(
            converter->CopyPixels(
                nullptr,
                rowSize,
                imageSize,
                pixels.data()),
            "Read image pixels");
    }
    catch (...)
    {
        if (mustUninitialize)
        {
            CoUninitialize();
        }

        throw;
    }

    if (mustUninitialize)
    {
        CoUninitialize();
    }
}

TextureResource TextureLoader::Load(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& filename,
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
    if (device == nullptr || commandList == nullptr)
    {
        throw std::runtime_error(
            "TextureLoader requires a device and command list");
    }

    UINT width = 1;
    UINT height = 1;

    std::vector<BYTE> pixels(4, 255);

    if (!filename.empty())
    {
        ReadImage(filename, width, height, pixels);
    }

    TextureResource texture;

    const auto textureDescription =
        CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            width,
            height,
            1,
            1);

    const CD3DX12_HEAP_PROPERTIES defaultHeap(
        D3D12_HEAP_TYPE_DEFAULT);

    CheckResult(
        device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &textureDescription,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(texture.Resource.GetAddressOf())),
        "Create GPU texture");

    const UINT64 uploadSize = GetRequiredIntermediateSize(
        texture.Resource.Get(),
        0,
        1);

    const auto uploadDescription =
        CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    const CD3DX12_HEAP_PROPERTIES uploadHeap(
        D3D12_HEAP_TYPE_UPLOAD);

    CheckResult(
        device->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &uploadDescription,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(texture.UploadBuffer.GetAddressOf())),
        "Create texture upload buffer");

    D3D12_SUBRESOURCE_DATA sourceData = {};

    sourceData.pData = pixels.data();
    sourceData.RowPitch = static_cast<LONG_PTR>(width) * 4;
    sourceData.SlicePitch = sourceData.RowPitch * height;

    const UINT64 copiedSize = UpdateSubresources(
        commandList,
        texture.Resource.Get(),
        texture.UploadBuffer.Get(),
        0,
        0,
        1,
        &sourceData);

    if (copiedSize == 0)
    {
        throw std::runtime_error(
            "Failed to upload texture pixels");
    }

    const auto barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            texture.Resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList->ResourceBarrier(1, &barrier);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription = {};

    srvDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    srvDescription.ViewDimension =
        D3D12_SRV_DIMENSION_TEXTURE2D;

    srvDescription.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(
        texture.Resource.Get(),
        &srvDescription,
        srvHandle);

    return texture;
}