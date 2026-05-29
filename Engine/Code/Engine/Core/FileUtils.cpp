#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <filesystem>


int FileReadToBuffer(std::vector<uint8_t>& out_Buffer, std::string const& fileName, bool ignoreErrors)
{
    FILE* newFile = new FILE;
    if (fopen_s(&newFile, fileName.c_str(), "rb") != 0)
    {
        if (!ignoreErrors)
            ERROR_RECOVERABLE(Stringf("Could not open file: \"%s\"", fileName.c_str()));

        return 0;
    }

    if (fseek(newFile, 0, SEEK_END) != 0)
    {
        if(!ignoreErrors)
            ERROR_RECOVERABLE(Stringf("File position indicator was not moved on file: \"%s\"", fileName.c_str()));

        return 0;
    }

    size_t size = ftell(newFile);
    out_Buffer.resize(size);

    fseek(newFile, 0, SEEK_SET);

    if (fread(out_Buffer.data(), sizeof(uint8_t), size, newFile) != size)
    {
        if (!ignoreErrors)
            ERROR_RECOVERABLE(Stringf("Could not read file: \"%s\"", fileName.c_str()));

        return 0;
    }

    fclose(newFile);
    
    return (int)size;
}

int FileWriteFromBuffer(std::vector<uint8_t> const& buffer, std::string const& fileName, bool ignoreErrors)
{
    FILE* newFile = nullptr;
    if (fopen_s(&newFile, fileName.c_str(), "wb") != 0 || newFile == nullptr)
    {
		if (!ignoreErrors)
			ERROR_RECOVERABLE(Stringf("Could not open file for writing: \"%s\"", fileName.c_str()));

		return 0;
    }

    size_t written = fwrite(buffer.data(), sizeof(uint8_t), buffer.size(), newFile);
    fclose(newFile);

    if (written != buffer.size())
    {
		if (!ignoreErrors)
			ERROR_RECOVERABLE(Stringf("Could not write full buffer to file: \"%s\"", fileName.c_str()));
		return (int)written;
    }

    return (int)written;
}

int FileReadToString(std::string& out_String, std::string const& fileName, bool ignoreErrors)
{
    std::vector <uint8_t> outBuffer;
    int success = FileReadToBuffer(outBuffer, fileName, ignoreErrors);
    outBuffer.push_back(0); //add null terminator

    out_String = std::string(outBuffer.begin(), outBuffer.end());
    return success;
}

std::string GetFileName(std::string const& filePathWithExtension)
{
	size_t pos = filePathWithExtension.find_last_of("/\\");
	return (pos == std::string::npos) ? filePathWithExtension : filePathWithExtension.substr(pos + 1);
}

std::string GetFileExtension(std::string const& filePath)
{
	std::string filename = GetFileName(filePath);
	size_t pos = filename.find_last_of('.');
	return (pos == std::string::npos) ? "" : filename.substr(pos);
}

std::string GetParentPath(std::string const& filePath)
{
	size_t pos = filePath.find_last_of("/\\");
	return (pos == std::string::npos) ? "" : filePath.substr(0, pos);
}

bool FileExists(std::string const& fileName)
{
	FILE* file = nullptr;
	if (fopen_s(&file, fileName.c_str(), "rb") != 0)
		return false;
	fclose(file);
	return true;
}

bool FolderExists(std::string const& folderName)
{
    return std::filesystem::is_directory(folderName);
}

bool CreateFolder(std::string const& folderName)
{
    if (!std::filesystem::create_directory(folderName))
    {
        ERROR_RECOVERABLE(Stringf("Could not create folder: '%s'", folderName.c_str()));
        return false;
    }
    return true;
}
