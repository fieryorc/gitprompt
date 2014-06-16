#pragma once
#include "Utils\SmartHandle.h"
#include "DirectoryMonitor.h"
#include "git2.h"
#include "Utils\Mutex.h"

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
	
	HANDLE GetHandle()
	{
		return this->m_waitHandle;
	}
	
private:
	CComCriticalSection m_critSec;
	CDirectoryMonitor *m_dirMonitor;
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
	GitRepoState m_repoState;
	GitStatus m_status;
	wstring m_branch;
	vector<CGitFileStatus> m_fileList;
	int m_added;
	int m_deleted;
	int m_modified;

	CAutoGeneralHandle m_waitHandle;
	void InitState();
	void SetStatus(CGitStatus::GitStatus status);
	static int GitStatus_Callack(const char *path, unsigned int status_flags, void *payload);
	static bool CGitStatus::GetRepoRootInternal(const wstring& path, wstring& repoRoot_out, git_buf &buf, git_repository *&repo);
	static void DirectoryChangedCallback(CDirectoryMonitor::ChangeType type, void *context);
	void MonitorForChanges();

};
