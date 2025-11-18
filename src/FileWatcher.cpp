#include "FileWatcher.h"

FileWatcher::FileWatcher(const std::filesystem::path& pathToWatch, std::chrono::duration<int, std::milli> delay)
	: m_PathToWatch(pathToWatch), m_Delay(delay), m_Running(true)
{
	for (const auto& file : std::filesystem::recursive_directory_iterator(pathToWatch))
	{
		m_Paths[file.path().string()] = std::filesystem::last_write_time(file);
	}
}

void FileWatcher::Start(const std::function<void(const std::string&, FileStatus)>& action)
{
	while (m_Running)
	{
		std::this_thread::sleep_for(m_Delay);

		auto it = m_Paths.begin();
		while (it != m_Paths.end())
		{
			if (!std::filesystem::exists(it->first))
			{
				action(it->first, FileStatus::ERASED);
				it = m_Paths.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto& file : std::filesystem::recursive_directory_iterator(m_PathToWatch))
		{
			auto currentFileLastWriteTime = std::filesystem::last_write_time(file);
			const std::string& pathString = file.path().string();

			if (!m_Paths.contains(pathString))
			{
				m_Paths[pathString] = currentFileLastWriteTime;
				action(pathString, FileStatus::CREATED);
			}
			else
			{
				if (m_Paths[pathString] != currentFileLastWriteTime)
				{
					m_Paths[pathString] = currentFileLastWriteTime;
					action(pathString, FileStatus::MODIFIED);
				}
			}
		}

	}
}
