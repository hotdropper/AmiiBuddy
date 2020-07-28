//
// Created by hotdropper on 7/19/20.
//

#ifndef AMIIBUDDY_AMIITOOL_FS_ADAPTER_H
#define AMIIBUDDY_AMIITOOL_FS_ADAPTER_H

#include <FS.h>

class AmiitoolFSAdapter {
public:
    virtual int read(const char *path, uint8_t *data, int dataLength);
    virtual int write(const char *path, uint8_t *data, int dataLength);
};

#endif //AMIIBUDDY_AMIITOOL_FS_ADAPTER_H
