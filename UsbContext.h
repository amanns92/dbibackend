//
// Created by Sebastian Amann on 16.09.24.
//

#ifndef USBCONTEXT_H
#define USBCONTEXT_H

#include <libusb.h>

class UsbContext {
    private:
        libusb_device_handle *dev_handle;
        libusb_endpoint_descriptor _out{};
        libusb_endpoint_descriptor _in{};

    public:
        UsbContext(uint16_t vid, uint16_t pid);
        ~UsbContext();
        int read(unsigned char *data, int length, unsigned int timeout = 0) const;
        void write(const unsigned char *data, int length, unsigned int timeout = 0) const;
};



#endif //USBCONTEXT_H
