// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/profiler.h"
#include "../io/path.h"
#include "file.h"
#include "file_system.h"
#include "file_watcher.h"
#include "log.h"

#ifdef _WIN32
#include "../common/win_wrapped.h"
#elif __linux__
#include <sys/inotify.h>
extern "C"
{
// Need read/close for inotify
#include "unistd.h"
}
#endif

using namespace std;

namespace dviglo
{

static const unsigned BUFFERSIZE = 4096;


FileWatcher::FileWatcher() :
    delay_(1.0f),
    watchSubDirs_(false)
{
#ifdef DV_FILEWATCHER
#ifdef __linux__
    watchHandle_ = inotify_init();
#endif
#endif
}

FileWatcher::~FileWatcher()
{
    stop_watching();
#ifdef DV_FILEWATCHER
#ifdef __linux__
    close(watchHandle_);
#endif
#endif
}

bool FileWatcher::start_watching(const String& pathName, bool watchSubDirs)
{
    // Stop any previous watching
    stop_watching();

#if defined(DV_FILEWATCHER) && defined(DV_THREADING)
#ifdef _WIN32
    String nativePath = to_native(trim_end_slash(pathName));

    dirHandle_ = (void*)CreateFileW(
        WString(nativePath).c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (dirHandle_ != INVALID_HANDLE_VALUE)
    {
        path_ = add_trailing_slash(pathName);
        watchSubDirs_ = watchSubDirs;
        Run();

        DV_LOGDEBUG("Started watching path " + pathName);
        return true;
    }
    else
    {
        DV_LOGERROR("Failed to start watching path " + pathName);
        return false;
    }
#elif defined(__linux__)
    int flags = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO;
    int handle = inotify_add_watch(watchHandle_, pathName.c_str(), (unsigned)flags);

    if (handle < 0)
    {
        DV_LOGERROR("Failed to start watching path " + pathName);
        return false;
    }
    else
    {
        // Store the root path here when reconstructed with inotify later
        dirHandle_[handle] = "";
        path_ = add_trailing_slash(pathName);
        watchSubDirs_ = watchSubDirs;

        if (watchSubDirs_)
        {
            Vector<String> subDirs;
            DV_FILE_SYSTEM->scan_dir(subDirs, pathName, "*", SCAN_DIRS, true);

            for (unsigned i = 0; i < subDirs.Size(); ++i)
            {
                String subDirFullPath = add_trailing_slash(path_ + subDirs[i]);

                // Don't watch ./ or ../ sub-directories
                if (!subDirFullPath.EndsWith("./"))
                {
                    handle = inotify_add_watch(watchHandle_, subDirFullPath.c_str(), (unsigned)flags);
                    if (handle < 0)
                        DV_LOGERROR("Failed to start watching subdirectory path " + subDirFullPath);
                    else
                    {
                        // Store sub-directory to reconstruct later from inotify
                        dirHandle_[handle] = add_trailing_slash(subDirs[i]);
                    }
                }
            }
        }
        Run();

        DV_LOGDEBUG("Started watching path " + pathName);
        return true;
    }
#else
    DV_LOGERROR("FileWatcher not implemented, can not start watching path " + pathName);
    return false;
#endif
#else
    DV_LOGDEBUG("FileWatcher feature not enabled");
    return false;
#endif
}

void FileWatcher::stop_watching()
{
    if (handle_)
    {
        shouldRun_ = false;

        // Create and delete a dummy file to make sure the watcher loop terminates
        // This is only required on Windows platform
        // TODO: Remove this temp write approach as it depends on user write privilege
#ifdef _WIN32
        String dummyFileName = path_ + "dummy.tmp";
        File file(dummyFileName, FILE_WRITE);
        file.Close();
        DV_FILE_SYSTEM->Delete(dummyFileName);
#endif

#ifdef _WIN32
        CloseHandle((HANDLE)dirHandle_);
#elif defined(__linux__)
        for (HashMap<int, String>::Iterator i = dirHandle_.Begin(); i != dirHandle_.End(); ++i)
            inotify_rm_watch(watchHandle_, i->first_);
        dirHandle_.Clear();
#endif

        Stop();

        DV_LOGDEBUG("Stopped watching path " + path_);
        path_.Clear();
    }
}

void FileWatcher::SetDelay(float interval)
{
    delay_ = Max(interval, 0.0f);
}

void FileWatcher::ThreadFunction()
{
#ifdef DV_FILEWATCHER
    DV_PROFILE_THREAD("FileWatcher Thread");

#ifdef _WIN32
    unsigned char buffer[BUFFERSIZE];
    DWORD bytesFilled = 0;

    while (shouldRun_)
    {
        if (ReadDirectoryChangesW((HANDLE)dirHandle_,
            buffer,
            BUFFERSIZE,
            watchSubDirs_,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesFilled,
            nullptr,
            nullptr))
        {
            unsigned offset = 0;

            while (offset < bytesFilled)
            {
                FILE_NOTIFY_INFORMATION* record = (FILE_NOTIFY_INFORMATION*)&buffer[offset];

                if (record->Action == FILE_ACTION_MODIFIED || record->Action == FILE_ACTION_RENAMED_NEW_NAME)
                {
                    String fileName;
                    const wchar_t* src = record->FileName;
                    const wchar_t* end = src + record->FileNameLength / 2;
                    while (src < end)
                        fileName.AppendUTF8(String::decode_utf16(src));

                    fileName = to_internal(fileName);
                    AddChange(fileName);
                }

                if (!record->NextEntryOffset)
                    break;
                else
                    offset += record->NextEntryOffset;
            }
        }
    }
#elif defined(__linux__)
    unsigned char buffer[BUFFERSIZE];

    while (shouldRun_)
    {
        int i = 0;
        auto length = (int)read(watchHandle_, buffer, sizeof(buffer));

        if (length < 0)
            return;

        while (i < length)
        {
            auto* event = (inotify_event*)&buffer[i];

            if (event->len > 0)
            {
                if (event->mask & IN_MODIFY || event->mask & IN_MOVE)
                {
                    String fileName;
                    fileName = dirHandle_[event->wd] + event->name;
                    AddChange(fileName);
                }
            }

            i += sizeof(inotify_event) + event->len;
        }
    }
#endif
#endif
}

void FileWatcher::AddChange(const String& fileName)
{
    scoped_lock lock(changes_mutex_);

    // Reset the timer associated with the filename. Will be notified once timer exceeds the delay
    changes_[fileName].Reset();
}

bool FileWatcher::GetNextChange(String& dest)
{
    scoped_lock lock(changes_mutex_);

    auto delayMsec = (unsigned)(delay_ * 1000.0f);

    if (changes_.Empty())
        return false;
    else
    {
        for (HashMap<String, Timer>::Iterator i = changes_.Begin(); i != changes_.End(); ++i)
        {
            if (i->second_.GetMSec(false) >= delayMsec)
            {
                dest = i->first_;
                changes_.Erase(i);
                return true;
            }
        }

        return false;
    }
}

} // namespace dviglo
