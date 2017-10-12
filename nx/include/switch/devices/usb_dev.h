/// Switch-as-device<>host USB comms for serial, uses usbDs. Do not directly use usbDs when using this.

/// usbDevInitialize will not return until the newline data transfer to the host finishes, aka when the host reads that data.
Result usbDevInitialize(void);
void usbDevExit(void);

/// These will throw fatal-error when any errors occur. These return the actual transfer size.
/// Note that if you use usbDevRead() where size is <0x200(wMaxPacketSize), any data after that in an USB packet will be discarded. That remaining data in a packet won't be readable by calling usbDevRead again.
/// These will not return until the data transfer finishes.
size_t usbDevRead(void* buffer, size_t size);
size_t usbDevWrite(const void* buffer, size_t size);

