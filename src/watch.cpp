#include "watch.hpp"
#include <Windows.h>
#include <iostream>
#include <chrono>

FileWatcher::FileWatcher(const std::string& watchPath, const std::string& buildCommand)
    : m_watchPath(watchPath)
    , m_buildCommand(buildCommand)
    , m_running(false)
    , m_directoryHandle(nullptr)
{
}

FileWatcher::~FileWatcher()
{
    stop();
}

void FileWatcher::start()
{
    if (m_running)
    {
        std::cout << "File watcher is already running." << std::endl;
        return;
    }

    m_running = true;
    m_thread = std::thread(&FileWatcher::watchThread, this);
    std::cout << "[FileWatcher] Started watching: " << m_watchPath << std::endl;
}

void FileWatcher::stop()
{
    if (!m_running)
        return;

    m_running = false;
    
    // Cancel the I/O operation to unblock ReadDirectoryChangesW
    if (m_directoryHandle)
    {
        CancelIoEx((HANDLE)m_directoryHandle, nullptr);
    }
    
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    
    if (m_directoryHandle)
    {
        CloseHandle((HANDLE)m_directoryHandle);
        m_directoryHandle = nullptr;
    }
    
    std::cout << "[FileWatcher] Stopped." << std::endl;
}

void FileWatcher::watchThread()
{
    // Open the directory for monitoring
    HANDLE hDir = CreateFileA(
        m_watchPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (hDir == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[FileWatcher] Failed to open directory: " << m_watchPath << std::endl;
        m_running = false;
        return;
    }

    m_directoryHandle = hDir;

    char buffer[4096];
    DWORD bytesReturned;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    auto lastBuildTime = std::chrono::steady_clock::now();
    const auto debounceDelay = std::chrono::milliseconds(1000); // 1 second debounce

    while (m_running)
    {
        // Read directory changes
        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE, // Watch subdirectories
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned,
            &overlapped,
            nullptr
        );

        if (!success)
        {
            if (GetLastError() == ERROR_OPERATION_ABORTED)
            {
                // Expected when stopping
                break;
            }
            std::cerr << "[FileWatcher] ReadDirectoryChangesW failed." << std::endl;
            break;
        }

        // Wait for the event or timeout
        DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 100);
        
        if (waitStatus == WAIT_OBJECT_0)
        {
            // Get the result
            if (!GetOverlappedResult(hDir, &overlapped, &bytesReturned, FALSE))
            {
                if (GetLastError() == ERROR_OPERATION_ABORTED)
                    break;
                continue;
            }

            // Process the changes
            if (bytesReturned > 0)
            {
                FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;
                
                // Check if enough time has passed since last build (debounce)
                auto now = std::chrono::steady_clock::now();
                if (now - lastBuildTime >= debounceDelay)
                {
                    // Extract filename for logging
                    std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
                    std::wcout << L"[FileWatcher] Change detected: " << filename << std::endl;
                    
                    lastBuildTime = now;
                    executeBuild();
                }
            }

            // Reset the event for next iteration
            ResetEvent(overlapped.hEvent);
        }
    }

    CloseHandle(overlapped.hEvent);
}

void FileWatcher::executeBuild()
{
    std::cout << "[FileWatcher] Building plugin..." << std::endl;
    
    // Execute the build command
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hide the console window

    // Create a copy of the command string (CreateProcessA modifies it)
    char cmdLine[512];
    strcpy_s(cmdLine, m_buildCommand.c_str());

    BOOL success = CreateProcessA(
        nullptr,
        cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (success)
    {
        // Wait for the build to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        if (exitCode == 0)
        {
            std::cout << "[FileWatcher] Build successful! Press 'R' to reload." << std::endl;
        }
        else
        {
            std::cout << "[FileWatcher] Build failed (exit code: " << exitCode << ")" << std::endl;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        std::cerr << "[FileWatcher] Failed to start build process." << std::endl;
    }
}
