#pragma once
#include "Utils\SmartHandle.h"
#include "DirectoryMonitor.h"
#include "git2.h"
#include "Utils\Mutex.h"
#include "Message.h"

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
		// Not initialized.
		GS_NOTLOADED,
		// Currently loading in progress.
		GS_LOADING,
		// Valid state. 
		GS_LOADED,
		// Error during loading
		GS_ERROR,
		// Invalidated by directory watcher. However, the current content may still be usable.
		GS_INVALIDATED
	};

public:
	CGitStatus(const wstring& startDir);
	~CGitStatus();

	static bool CGitStatus::GetRepoRoot(const wstring& path, wstring& repoRoot_out);

	/**
	 * Returns the Repository root.
	 */
	wstring& GetPath();


	/**
	 *  Gets the status of the repository.
	 *  Waits if the status is not loaded yet. 
	 *  Returns true on success, false otherwise.
	 */
	bool GetRepoStatus(CacheServiceResponse &status);

	GitStatus GetStatus();
	
private:
	CComCriticalSection m_critSec;
	CDirectoryMonitor *m_dirMonitor;
	CDirectoryMonitor *m_dirMonitorGitDir;
	wstring m_startDir;

	/**
	 * Libgit2 isn't multi threaded (yet!). For now, just allow one request to go through.
	 */
	static CMutex m_mutex;

	/**
	 * .git directory.
	 */
	wstring m_gitDir;
	wstring m_repoRoot;
	
	/** Enum indicating the repository state. (refer to: git_repository_state_t)
	 * This indicates if any merge/rebase/etc operation in progress.
	 */
	DWORD m_repoState;

	GitStatus m_status;
	wstring m_branch;
	int m_addedIndex;
	int m_deletedIndex;
	int m_modifiedIndex;
	int m_addedWorkDir;
	int m_deletedWorkDir;
	int m_modifiedWorkDir;
	vector<CGitFileStatus> m_fileList;

	CAutoGeneralHandle m_waitHandle;

	/**
	* Load the status.
	*/
	void Load();

	void InitState();
	void SetStatus(CGitStatus::GitStatus status);
	static int GitStatus_Callack(const char *path, unsigned int status_flags, void *payload);
	static bool CGitStatus::GetRepoRootInternal(const wstring& path, wstring& repoRoot_out, git_buf &buf, git_repository *&repo);
	static void DirectoryChangedCallback(CDirectoryMonitor::ChangeType type, void *context);
	void MonitorForChanges();

	void CleanupDirectoryMonitors()
	{
		if (this->m_dirMonitor != nullptr)
			delete this->m_dirMonitor;
		if (this->m_dirMonitorGitDir != nullptr)
			delete this->m_dirMonitorGitDir;
		this->m_dirMonitor = nullptr;
		this->m_dirMonitorGitDir = nullptr;
	}

};
