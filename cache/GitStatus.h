#pragma once
class CGitFileStatus
{
private:
	wstring m_filePath;
	/**
	 * File modification status. Added/Removed etc.
	 * TODO:
	 */
	int m_status;
public:
	CGitFileStatus(wstring path, int status)
		: m_filePath(path),
		m_status(status)
	{
		
	}

	~CGitFileStatus()
	{

	}
};


class CGitStatus
{
public:

	enum git_status {
		GS_NORMAL,
		GS_MODIFIED,
		GS_ERROR,
	};

private:
	wstring m_startDir;
	git_status m_status;
	wstring m_repoPath;
	vector<CGitFileStatus> m_fileList;

	void SetStatus(CGitStatus::git_status status);
	static int GitStatus_Callack(const char *path, unsigned int status_flags, void *payload);

public:
	CGitStatus(const wstring& startDir);
	~CGitStatus();

	/**
	 * Returns the Repository root.
	 */
	wstring& GetPath();

	/**
	 * Load the status. 
	 */
	void Load();

	/**
	 *  Returns the overall status of the repository.
	 */
	git_status GetStatus();
};
