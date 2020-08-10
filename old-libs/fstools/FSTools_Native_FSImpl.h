//
// Created by Jacob Mather on 7/31/20.
//

#ifndef AMIIBUDDY_FSTOOLS_NATIVE_FSIMPL_H
#define AMIIBUDDY_FSTOOLS_NATIVE_FSIMPL_H

#include <stddef.h>
#include <stdint.h>

typedef bool boolean;

namespace fs
{

    class FileImpl
    {
    public:
        virtual ~FileImpl() { }
        virtual size_t write(const uint8_t *buf, size_t size) = 0;
        virtual size_t read(uint8_t* buf, size_t size) = 0;
        virtual void flush() = 0;
        virtual bool seek(uint32_t pos, SeekMode mode) = 0;
        virtual size_t position() const = 0;
        virtual size_t size() const = 0;
        virtual void close() = 0;
        virtual time_t getLastWrite() = 0;
        virtual const char* name() const = 0;
        virtual boolean isDirectory(void) = 0;
        virtual FileImplPtr openNextFile(const char* mode) = 0;
        virtual void rewindDirectory(void) = 0;
        virtual operator bool() = 0;
    };

    class FSImpl
    {
    protected:
        const char * _mountpoint;
    public:
        FSImpl() : _mountpoint(NULL) { }
        virtual ~FSImpl() { }
        virtual FileImplPtr open(const char* path, const char* mode) = 0;
        virtual bool exists(const char* path) = 0;
        virtual bool rename(const char* pathFrom, const char* pathTo) = 0;
        virtual bool remove(const char* path) = 0;
        virtual bool mkdir(const char *path) = 0;
        virtual bool rmdir(const char *path) = 0;
        void mountpoint(const char *);
        const char * mountpoint();
    };

} // namespace fs

#endif //AMIIBUDDY_FSTOOLS_NATIVE_FSIMPL_H
