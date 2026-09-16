#ifndef UTILS
#define UTILS
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <SimpleMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;


class Utils {
public:
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* CmdList,
		const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
	static UINT CalcConstantBufferSize(UINT byteSize);
	static ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
private:
};


#endif
