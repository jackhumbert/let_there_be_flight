#pragma once

class FileStream : public RED4ext::BaseStream
{
public:
    FileStream(const std::filesystem::path& aPath, uint32_t aDesiredAccess, uint32_t aShareMode,
               uint32_t aCreationDisposition, uint32_t aFlagsAndAttributes);
    ~FileStream();

    bool IsOpen() const;

    virtual void* ReadWrite(void* aBuffer, uint32_t aLength) override;
    virtual size_t GetPointerPosition() override;
    virtual size_t GetLength() override;

    virtual bool Seek(size_t aDistance) override;
    bool Seek(size_t aDistance, uint32_t aMoveMethod);

    virtual bool Flush() override;
    std::filesystem::path GetPath() const;

private:
    HANDLE m_file;
    std::filesystem::path m_path;
};
