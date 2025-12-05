#ifndef __WATCH_H__
#define __WATCH_H__

#include <string>
#include <functional>
#include <thread>
#include <atomic>

class FileWatcher
{
public:
    FileWatcher(const std::string& watchPath, const std::string& buildCommand);
    ~FileWatcher();

    // Start watching in a separate thread
    void start();
    
    // Stop watching
    void stop();
    
    // Check if watcher is running
    bool isRunning() const { return m_running; }

private:
    void watchThread();
    void executeBuild();
    
    std::string m_watchPath;
    std::string m_buildCommand;
    std::atomic<bool> m_running;
    std::thread m_thread;
    void* m_directoryHandle;  // HANDLE type (Windows)
};

#endif // __WATCH_H__
