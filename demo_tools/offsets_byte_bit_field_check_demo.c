/*
What the demo program does:
It checks the 2nd byte's bit field in a set of an array.
It specifically checks the 7th bit field in the 2nd byte.

In the 4th set of the array, has a 2nd byte with the hex value of `0x4E`
0x4E in binary is: 0100 1110

Such that the 7th bit in `0100 1110` is: `1`.
                           ^
                           |---------------------- 7th bit field.

To check if this bit is `1`, the below program implements an algo to do so using bitwise operation.

This is useful, such as in UVC USB MJPEG Bit Field Header parsing. 

-------------------

How to compile:

F:
cd F:\AI_Componets\WinUSB\main_test_legit

"F:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat" x64

set PATH=%PATH%;F:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin
set INCLUDE=%INCLUDE%;F:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\include
set INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um
set INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared
set INCLUDE=%INCLUDE%;F:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.42.34433\include
set LIB=%LIB%;F:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\lib\x64
set LIB=%LIB%;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64

cl /EHsc /std:c++17 offsets_byte_bit_field_check_demo.c

*/

#include <stdio.h>
#include <stdint.h>

int main() 
{
    // Array of offsets (each offset is 12 bytes long)
    uint8_t offsets[5][12] = 
    {
        {0x0C, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x01},       // set 1, index 0
        {0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x03},       // set 2, index 1
        {0x0C, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x03},       // set 3, index 2
        {0x0C, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x03},       // set 4, index 3
        {0x0C, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x03}        // set 5, index 4
// Bytes: 1     2     3     4     5     6     7     8     9     10    11    12
// Index  0     1     2     3     4     5     6     7     8     9     10    11    

    };

    // Access the 4th set of offsets (index 3)
    uint8_t secondByte = offsets[3][1];

    // Check if the 7th bit (bit 6, counting from 0) is set
    if (secondByte & (1 << 6))                                      // it can be coded as such:  ((secondByte & (1 << 6)) != 0)
    {
        printf("The error bit is triggered in the 4th set.\n");
    } 
    else 
    {
        printf("The error bit is not triggered in the 4th set.\n");
    }

    return 0;
}
