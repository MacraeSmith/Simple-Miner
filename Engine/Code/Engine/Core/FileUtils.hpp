#pragma once
#include <vector>
#include <string>

int FileReadToBuffer(std::vector<uint8_t>& out_Buffer, std::string const& fileName, bool ignoreErrors = false);
int FileWriteFromBuffer(std::vector<uint8_t> const& buffer, std::string const& fileName, bool ignoreErrors = false);
int FileReadToString(std::string& out_String, std::string const& fileName, bool ignoreErrors = false);
std::string GetFileName(std::string const& filePathWithExtension);
std::string GetFileExtension(std::string const& filePath);
std::string GetParentPath(std::string const& filePath);
bool FileExists(std::string const& fileName);
bool FolderExists(std::string const& folderName);
bool CreateFolder(std::string const& folderName);
