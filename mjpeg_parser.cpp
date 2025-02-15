#include <iostream>
#include <cstring>
#include <cstdint> // for uint64_t

int main() {
    // Sample buffer containing some data
    uint8_t buffer[] = {
        0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x87, 0x00, 0x1D, 0xA9, 0x68, 0x01,
        0x08, 0xA4, 0xA0, 0x04, 0x22, 0x92, 0x81, 0x58,
        0x39, 0xA2, 0x81, 0xEA, 0x14, 0x75, 0xA0, 0x18,
        0xB4, 0x1C, 0xD3, 0x10, 0xDC, 0x52, 0x60, 0x50,
        0x16, 0x1B, 0x47, 0x6A, 0xFF, 0xD8
    };

    size_t bufferSize = sizeof(buffer);
    
    // Ensure buffer size is a multiple of 8 for this example
    if (bufferSize % 8 != 0) {
        std::cerr << "Buffer size is not a multiple of 8." << std::endl;
        return 1;
    }

    // Process the buffer in chunks of 8 bytes
    for (size_t i = 0; i < bufferSize; i += 8) {
        uint64_t chunk;
        std::memcpy(&chunk, buffer + i, sizeof(chunk));
        
        std::cout << "Chunk " << i / 8 << ": 0x" << std::hex << chunk << std::dec << std::endl;
    }

    return 0;
}