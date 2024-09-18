//
// Created by Sebastian Amann on 16.09.24.
//

#include "../header/UsbContext.h"
#include <iostream>
#include <stdexcept>

UsbContext::UsbContext(uint16_t vid, uint16_t pid) {
    libusb_context *ctx = nullptr;
    int result = libusb_init(&ctx);
    if (result < 0) {
        throw std::runtime_error("Failed to initialize libusb");
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, vid, pid);
    if (!dev_handle) {
        libusb_exit(ctx);
        throw std::runtime_error("Device not found");
    }

    libusb_reset_device(dev_handle);
    result = libusb_set_configuration(dev_handle, 1);
    if (result < 0) {
        libusb_close(dev_handle);
        libusb_exit(ctx);
        throw std::runtime_error("Failed to set configuration");
    }

    result = libusb_claim_interface(dev_handle, 0);
    if (result < 0) {
        libusb_close(dev_handle);
        libusb_exit(ctx);
        throw std::runtime_error("Failed to claim interface");
    }

    // Finding the OUT and IN endpoints
    libusb_config_descriptor *config;
    result = libusb_get_active_config_descriptor(libusb_get_device(dev_handle), &config);
    if (result < 0) {
        libusb_close(dev_handle);
        libusb_exit(ctx);
        throw std::runtime_error("Failed to get configuration descriptor");
    }

    const libusb_interface *interface = &config->interface[0];
    const libusb_interface_descriptor *interface_desc = &interface->altsetting[0];

    for (int i = 0; i < interface_desc->bNumEndpoints; ++i) {
        const libusb_endpoint_descriptor *ep = &interface_desc->endpoint[i];
        if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
            _out = *ep;
        } else if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
            _in = *ep;
        }
    }

    if (!_out.bEndpointAddress) {
        libusb_close(dev_handle);
        libusb_exit(ctx);
        throw std::runtime_error("Output endpoint not found");
    }
    if (!_in.bEndpointAddress) {
        libusb_close(dev_handle);
        libusb_exit(ctx);
        throw std::runtime_error("Input endpoint not found");
    }

    libusb_free_config_descriptor(config);
}

UsbContext::~UsbContext() {
    libusb_close(dev_handle);
    libusb_exit(nullptr);
}

int UsbContext::read(unsigned char *data, int length, unsigned int timeout) const {
    int transferred;
    int result = libusb_bulk_transfer(dev_handle, _in.bEndpointAddress, data, length, &transferred, timeout);
    if (result < 0) {
        throw std::runtime_error("Read failed");
    }
    return transferred;
}

void UsbContext::write(const unsigned char *data, int length, unsigned int timeout) const {
    int transferred;
    int result = libusb_bulk_transfer(dev_handle, _out.bEndpointAddress, const_cast<unsigned char *>(data), length, &transferred, timeout);
    if (result < 0) {
        throw std::runtime_error("Write failed");
    }
}

