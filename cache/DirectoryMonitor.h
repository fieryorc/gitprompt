#pragma once
class CDirectoryMonitor
{
public: 
	enum ChangeType
	{
		FILE_ADDED,
		FILE_DELETED,
		FILE_MODIFIED
	};
	typedef void(*ChangeCallback)(ChangeType kind, void *context);

private:

	wstring m_path;
	ChangeCallback m_callback;
	void *m_callbackContext;
	OVERLAPPED m_overlapped;

	// Result of calling CreateFile().
	HANDLE		m_hDirectory;

	// Data buffer for the request.
	// Since the memory is allocated by malloc, it will always
	// be aligned as required by ReadDirectoryChangesW().
	vector<BYTE> m_Buffer;

	// Double buffer strategy so that we can issue a new read
	// request before we process the current buffer.
	vector<BYTE> m_BackupBuffer;

	bool OpenDirectory();
	void BeginRead();
	static DWORD WINAPI ThreadStart(LPVOID lpvParam);
	void Notify(bool isSucceeded);

public:

	CDirectoryMonitor(const wstring& path);
	~CDirectoryMonitor();
	bool Monitor(ChangeCallback callback, void *context);
};

