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

	enum GitStatus {
		GS_UNINITIALIZED,
		GS_LOADED,
		GS_ERROR,
	};

	typedef enum {
		GIT_REPOSITORY_STATE_NONE,
		GIT_REPOSITORY_STATE_MERGE,
		GIT_REPOSITORY_STATE_REVERT,
		GIT_REPOSITORY_STATE_CHERRY_PICK,
		GIT_REPOSITORY_STATE_BISECT,
		GIT_REPOSITORY_STATE_REBASE,
		GIT_REPOSITORY_STATE_REBASE_INTERACTIVE,
		GIT_REPOSITORY_STATE_REBASE_MERGE,
		GIT_REPOSITORY_STATE_APPLY_MAILBOX,
		GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE,
	} GitRepoState; 

public:
	CGitStatus(const wstring& startDir);
	~CGitStatus();

	static bool CGitStatus::GetRepoRoot(const wstring& path, wstring& repoRoot_out);

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
	GitStatus GetStatus();

	wstring& GetBranch();

	int GetAddedFileCount();

	int GetModifiedFileCount();

	int GetDeletedFileCount();

	GitRepoState GetRepoStatus();

private:
	wstring m_startDir;

	GitRepoState m_repoState;
	GitStatus m_status;
	wstring m_repoPath;
	wstring m_branch;
	vector<CGitFileStatus> m_fileList;
	int m_added;
	int m_deleted;
	int m_modified;

	void SetStatus(CGitStatus::GitStatus status);
	static int GitStatus_Callack(const char *path, unsigned int status_flags, void *payload);
};
