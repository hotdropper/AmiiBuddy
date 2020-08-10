//
// Created by Jacob Mather on 7/31/20.
//

#ifndef AMIIBUDDY_FSTOOLS_NATIVE_FS_H
#define AMIIBUDDY_FSTOOLS_NATIVE_FS_H

#include <memory>
#include <string>

typedef std::string String;
typedef bool boolean;

#define FSTools_Native

namespace fs
{

#define FILE_READ       "r"
#define FILE_WRITE      "w"
#define FILE_APPEND     "a"

    class File;

    class FileImpl;
    typedef std::shared_ptr<FileImpl> FileImplPtr;
    class FSImpl;
    typedef std::shared_ptr<FSImpl> FSImplPtr;

    enum SeekMode {
        SeekSet = 0,
        SeekCur = 1,
        SeekEnd = 2
    };

    class File
    {
    public:
        File(FileImplPtr p = FileImplPtr()) : _p(p) {
            _timeout = 0;
        }

        size_t write(uint8_t);
        size_t write(const uint8_t *buf, size_t size);
        int available();
        int read();
        int peek();
        void flush();
        size_t read(uint8_t* buf, size_t size);
        size_t readBytes(char *buffer, size_t length)
        {
            return read((uint8_t*)buffer, length);
        }

        bool seek(uint32_t pos, SeekMode mode);
        bool seek(uint32_t pos)
        {
            return seek(pos, SeekSet);
        }
        size_t position() const;
        size_t size() const;
        void close();
        operator bool() const;
        time_t getLastWrite();
        const char* name() const;

        bool isDirectory(void);
        File openNextFile(const char* mode = FILE_READ);
        void rewindDirectory(void);

    protected:
        FileImplPtr _p;
        int _timeout = 0;
    };

    class FS
    {
    public:
        FS(FSImplPtr impl) : _impl(impl) { }

        File open(const char* path, const char* mode = FILE_READ);
        File open(const String& path, const char* mode = FILE_READ);

        bool exists(const char* path);
        bool exists(const String& path);

        bool remove(const char* path);
        bool remove(const String& path);

        bool rename(const char* pathFrom, const char* pathTo);
        bool rename(const String& pathFrom, const String& pathTo);

        bool mkdir(const char *path);
        bool mkdir(const String &path);

        bool rmdir(const char *path);
        bool rmdir(const String &path);


    protected:
        FSImplPtr _impl;
    };

} // namespace fs


#endif //AMIIBUDDY_FSTOOLS_NATIVE_FS_H
