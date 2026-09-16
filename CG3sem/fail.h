#ifndef FAIL
#define FAIL

#include <stdexcept>
#include <string>

template<typename T>
inline void Fail(T hr, const char* message = "DirectX operation failed")
{
    if (FAILED(hr))
    {
        throw std::runtime_error(message);
    }
}

#endif