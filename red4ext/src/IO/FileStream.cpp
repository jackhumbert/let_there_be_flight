#include "stdafx.hpp"
#include "FileStream.hpp"
#include "Utils.hpp"

FileStream::FileStream(const std::filesystem::path& aPath, uint32_t aDesiredAccess, uint32_t aShareMode,
                       uint32_t aCreationDisposition, uint32_t aFlagsAndAttributes)
    : BaseStream(0xA)
    , m_path(aPath)
{
    m_file = CreateFile(aPath.c_str(), aDesiredAccess, aShareMode, nullptr, aCreationDisposition, aFlagsAndAttributes,
                        nullptr);
}

FileStream::~FileStream()
{
    if (m_file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_file);
    }
}

bool FileStream::IsOpen() const
{
    return m_file != INVALID_HANDLE_VALUE;
}

void* FileStream::ReadWrite(void* aBuffer, uint32_t aLength)
{
    DWORD numberOfBytesRead;
    if (!ReadFile(m_file, aBuffer, aLength, &numberOfBytesRead, nullptr))
    {
        auto fileName = m_path.stem();
        spdlog::warn(L"[{}] read error: requested_bytes={}, read={}, errno={:#x}", fileName.c_str(), aLength,
                     numberOfBytesRead, GetLastError());

        return nullptr;
    }

    return aBuffer;
}

size_t FileStream::GetPointerPosition()
{
    LARGE_INTEGER filePointer{};
    if (!SetFilePointerEx(m_file, {0}, &filePointer, FILE_CURRENT))
    {
        filePointer.QuadPart = -1;
    }

    return filePointer.QuadPart;
}

size_t FileStream::GetLength()
{
    LARGE_INTEGER result{};
    if (IsOpen() && GetFileSizeEx(m_file, &result))
    {
        return result.QuadPart;
    }
    else
    {
        result.QuadPart = 0;
    }

    return result.QuadPart;
}

bool FileStream::Seek(size_t aDistance)
{
    return Seek(aDistance, FILE_CURRENT);
}

bool FileStream::Seek(size_t aDistance, uint32_t aMoveMethod)
{
    LARGE_INTEGER distance;
    distance.QuadPart = aDistance;

    if (!SetFilePointerEx(m_file, distance, nullptr, aMoveMethod))
    {
        auto fileName = m_path.stem();
        auto length = GetLength();

        spdlog::warn(L"[{}] seek error: distance={}, method={}, file size={}, errno={:#x}", fileName.c_str(), aDistance,
                     aMoveMethod, length, GetLastError());

        return false;
    }

    return true;
}

bool FileStream::Flush()
{
    return true;
}

std::filesystem::path FileStream::GetPath() const
{
    return m_path;
}
