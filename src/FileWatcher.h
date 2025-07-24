#pragma once

#include <stdint.h>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <thread>

enum class FileStatus : uint8_t
{
	CREATED = 0,
	MODIFIED,
	ERASED
};

class FileWatcher
{
public:
	FileWatcher() = default;
	explicit FileWatcher(const std::filesystem::path& pathToWatch, std::chrono::duration<int, std::milli> delay);
	void Start(const std::function<void(const std::string&, FileStatus)>& action);

private:
	std::filesystem::path m_PathToWatch;
	std::chrono::duration<int, std::milli> m_Delay;

	std::unordered_map<std::string, std::filesystem::file_time_type> m_Paths;
	bool m_Running;
};

