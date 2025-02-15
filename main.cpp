/* Notes:

    Tools:
    Hexadecimal Calculator & Converter:
    https://www.inchcalculator.com/hex-calculator/

    diffchecker:
    https://www.diffchecker.com/text-compare/


    2/3/2025
    Update: MJPEG USB Fragmented packets has been deciphered for packet re-encoding (This is only to reconstruct the fragments into a proper MJPEG frame, decoding is the next logic update to work on)...

    V2 - 2/3/2025 - 4:33 AM
    Program reconstruct's 1st packet to disk.

    v3- 2/3/2025 - 12:37 PM:
    Program is able to reconstruct about 3 packets, until it stops for a false FFD9. and then reads next packet skipping many previous packets.

    v4- 2/4/2025 - 6 AM:
    Program is able to reconstruct all packets, from FFD8 to Body, however stoping at FFD9 is not confirmed and logic to continue during live stream not implemented yet.

    v5- 2/4/2025 - 12:20 PM:
    Program is able to reconstruct all packets, from FFD8 to Body for low latency and proper offset retrieval. 
    However stoping at FFD9 is not confirmed and logic to continue during live stream not implemented yet.
    Need to include: 
        Logic such that it does not retrieve FFD8 again for the first MJPEG body.
        Able to iterate and properly stitch MJPEG body from the previous iteration.
        Confirm if FFD8 to FFD9 is one 1080P frame or it is fragmented at the MJPEG level.
        Program also seems to not print the offset values correctly after the 1st transfer.
        Confirm buffer location of proceeding transfers.
        Prevent copying zeros.

    v5- 2/4/2025 - 6 PM:
        View what tags are crashing the MJPEG stitching.

    v6 - 2/6/2025 - 10PM:
        Program is able to stitch all MJPEG frames, only at perfect intervals.
        Need to figure out how many intervals/frames creates a new FFD8.
        At 16ms, the UVC USB device sends two legit FFD8 and one legit FFD9... there will be plenty of garbage FFD8s and few FFD9s.
        Legit FFD9 contains: FF D9 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

    v6 - 2/7/2025 - 12PM:
        31 USB frames makes 1 legit MJPEG Picture 1080P frame.
        Camera seems to output mismatched body data as legit data...
        Algo needs to somehow know which specific body data are favorable.  <------ Not possible, USB device seems to provide crap data, could be USB host controller or synchronization issue. Seems to be 99% caused by hardware related data processing of mismatched output.
        432 Bytes / per microframe - Max allowable in USB CAM descriptor.

    v7 - 2/8/2025 - 8PM:
        Program is able to take snapshots, filter out crap data.
        Program is able to behave like a camera now, but it is ultra slow... tune it.
        For FFD9, filter for searching `0C 0E 00 00 00` UVC tag and eliminate requirement for searching FFD9 tag use the length parameter to copy the data. (Actually this wont work...)

    v8 - 2/9/2025 - 5PM:
        The last two bytes is the SCR, little endian, it seems to increment every 1ms, thus it indicates the packets are in correct order and is part of the MJPEG frame container.
        Example: Packet_0: SCR-1601, 
                Packet_1: SCR-1602, 
                Packet_2: SCR 1603,
                Packet_3: SCR 1604,
                Packet_4: SCR 1605... etc.
        
        All UVC packets start with 0x0C tag
        The following 2nd byte is the Frame ID identifier, which means the 2nd byte indicates whether or not the packet is the continuation or end of the MJPEG packet stream for the MJPEG single container frame.
        Example: Packet_0: FID: 0C 0C   <------------ 2nd Byte is 0C, in binary it is: 1100
                Packet_1: FID: 0C 0C                                                    ^------------ 2nd bit field, is the FID toggle bit. It is zero, thus it is indicating it is EITHER the start or continuation of the MJPEG packet frames.
                Packet_2: FID: 0C 0C
                Packet_3: FID: 0C 0C
                Packet_4: FID: 0C 0E   <------------ 2nd Byte is 0E, in binary it is: 1110
                                                                                        ^------------ 2nd bit field, is the FID toggle bit. It is one, thus it is indicating it is the END of MJPEG packet frames.
                
                Next new MJPEG pic frame:
                Packet_0: FID: 0C 0D   <------------ 2nd Byte is 0C, in binary it is: 1101
                Packet_1: FID: 0C 0C                                                    ^------------ 2nd bit field, is the FID toggle bit. It is zero, thus it is indicating it is EITHER the start or continuation of the MJPEG packet frames.
                Packet_2: FID: 0C 0C
                Packet_3: FID: 0C 0C
                Packet_4: FID: 0C 0F   <------------ 2nd Byte is 0E, in binary it is: 1111
                                                                                        ^------------ 2nd bit field, is the FID toggle bit. It is one, thus it is indicating it is the END of MJPEG packet frames.

    v9 - 2/9/2025 - 9PM - Algo is much more efficient, does not iterates through MJPEG body. Uses only UVC USB Tags to reconstruct as intended.
        Program needs to utilize the UVC USB's SCR tags for better filtering.
        Program needs to parse the BFH bit fields, to read the error bit if the packets contains errors and simply break the parser and request for another `transfer read` to obtain a transfer for which it's continuous packets doesn't have error bits.

    v36 - 2/9/2025 - 9PM - Algo is much more efficient, does not iterates through MJPEG body. Uses only UVC USB Tags to reconstruct as intended.
        Program needs to utilize the UVC USB's SCR tags for better filtering. Use two buffers and asynchronous method.

    v39 - 2/11/2025 - 10:44 PM - Program is now able to perform DMA asynchronously, using overlapped I/O to submit both buffers using OVERLAPPED structures, then use WaitForMultipleObjects to wait for them to complete. You process that one, then send it right back down.
        Program now needs to access the asynchronous data and perform parsing to reconstruct fragmented JPEGs.
        For lower latency, do not use function calls as it introduces latencies.
        Reduce the code base such that it is short enough to be stored in CPU cache.
        Temp buffer might be needed to mitigate high latency. This is right before reading in the main buffer data, such that copying main buffer to a temp buffer, the program then can quickly right away request another DMA read in the main buffer. The CPU can then perform MJPEG parsing in the temp buffer.
        Unless more buffers are used. Right before read of the data in the buffer, request another DMA to another buffer location... thus this prevents CPU utilization of copying.

        Cache Locality: Duplicated code may sometimes improve cache locality if the duplicated sections of code are small and fit well within the CPU cache. This can lead to faster execution times due to reduced cache misses.
        Inlining and Function Call Overhead: If the duplicated code eliminates the need for function calls, it can reduce the overhead associated with calling functions. In some performance-critical sections, minimizing function call overhead can lead to lower latencies.
        Compiler Optimizations: In certain scenarios, having code duplicated and inlined can allow the compiler to perform more aggressive optimizations, such as loop unrolling or constant folding, which can reduce execution time.
        AMD FX-8350 Cache: Cache L1: 384 KB, Cache L2: 8 MB, Cache L3: 8 MB (shared).
        16KB L1 data cache per core.
        64KB L1 instruction cache shared per two cores (per module).
        2MB L2 cache shared per two cores (per module).

    v43 - 2/15/2025 - 2:35 AM - Program is optimized, can capture, parse and display in realtime. Low latency. Uses three buffers.
        Version v40 uses two buffers, however, it is CPU intensive, and causes frames to be dropped. v40 is extremely low latency, not useful for multi-camera requirements. Only one camera per USB host controller, else it will hog the bus.
        v43, uses three buffers, introduces slight latency, however it is able to process frames without dropping them. 
        Features to add:
            Implement more checks to remove frames with errors.
            Implement multithreading.
            Use assembly code.
            Refactor could much further.


    Work on:

    USB packet transfer packet offsets are the same... only the buffer is unique and incremented. Possibly use algo to increment...
    Isopacket offset does not align with read buffer allocation.
    On the 1st transfer of the 5th packet, offset is being misaligned.
    Change all the terms used, doesn't relate to the actual meaning of the unit.

    Need to properly understand how data is specifically written to memory and extracted.
    Seems like there are two buffers:
    1) overlapped... <--- to get the transaction ASAP.
    2) readbuffer... <---- to get the transaction after all pending work is completed.

    How to compile:
    cl /EHsc /std:c++17 mjpeg_ffd8.cpp

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

    cl /EHsc /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" main.cpp /link /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64" /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64" /LIBPATH:"F:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.42.34433\lib\x64" cudart.lib winusb.lib SetupAPI.lib cfgmgr32.lib

    1 sec = 1,000 ms
    camera is 60 FPS.

    1,000ms/60FPS = 16.666 ms / camera frame input stream.
    16.66 ms = 16 frames + .66 (another 1 frame?) = 17 Frames.

    16 x 8 x .125ms

        1
        <--->
        ^   ^
        |---|---- .125 ms.


        1  2  3  4  5  6  7  8
    .6 x 0, 0, 0, 0, 0, 3, 3, 3
                ^  ^  ^
                |  |  |------------ Exactly in the .625th ms mark (6th usb micro frame) the next new camera frame starts.
                |  |
                |  |-- .625 ms
                |
                |---- .500 ms


    The host will need to wait if data / camera frame takes less time than 16 ms to transfer, the wait time is the difference between the time full frame transferred and the remaining of the 16 ms.
    This is called the idel time.
    Let's say your images are 100kB. That occupies 4 full transfers and part of a fifth transfer (which will have "3k 1k 0 0 0 0 0 0" packets). 
    Thus 100KB image takes 5 Frames or 5ms. Next "camera frame" will be on the 16th ms mark.
    Now we have 11ms of idle time (5ms - 16ms - 11ms) until the next frame is available. 
    So, you'll get 11 transfers that are completely empty. During the idel time. This would be the gap.


    Isochronous pipe, you get one header per PACKET.
    That header includes bits that tell you whether the timestamps are included, and whether this packet represents the beginning, middle, or end of a frame.
    24k buffer is divided into 8 3k packets. 
    Each packet corresponds to one microframe. 
    If your device sends nothing or the packet is dropped, there will be a gap in the buffer.

    SOI (Start of Image): FFD8
    EOI (End of Image): FFD9
    SOS (Start of Scan): FFDA
    DHT (Define Huffman Tables): FFC4
    DQT (Define Quantization Tables): FFDB
    SOF (Start of Frame): FFC0 to FFC3

    ---------------

    Parsing:

    Example 1:

    Sequence 1: 0C 0D 00 00 00 00 00 00 00 00 BE 04 FF D8 FF E0
    Header Length (HLE): 0C

    This indicates the header is 12 bytes long.
    Bit Field Header (BFH): 0D

    FID: 0 (Frame Identifier toggles at each frame start boundary)
    EOF: 0 (End of Frame)
    PTS: 0 (Presentation Time Stamp not present)
    SCR: 0 (Source Clock Reference not present)
    RES: 0 (Reserved)
    STI: 0 (Still Image)
    ERR: 0 (Error Bit)
    EOH: 1 (End of Header)
    Reserved Fields: 00 00 00 00 00 00 00 00

    These are reserved fields and are set to 0.
    JPEG Data: BE 04 FF D8 FF E0

    FF D8: Start of Image (SOI) marker.
    FF E0: Application Marker (APP0), typically used for JFIF header.
    Sequence 2: 0C 0D 00 00 00 00 00 00 00 00 E2 05 FF D8 FF E0
    Header Length (HLE): 0C

    This indicates the header is 12 bytes long.
    Bit Field Header (BFH): 0D

    These are reserved fields and are set to 0.
    JPEG Data: E2 05 FF D8 FF E0

    FF D8: Start of Image (SOI) marker.
    FF E0: Application Marker (APP0), typically used for JFIF header.



    Example 2:


    To identify where the header starts in the UVC USB MJPEG dump, we need to look for the specific markers and fields defined in the MJPEG payload header format. Based on the documentation you provided, the header should include fields like the header length (HLE), bit field header (BFH), and optional fields like PTS and SCR if they are present.

    Let's break down the dump and identify the header:

    ### Dump Analysis

    ```
    00 14 62 81 58 28 A0 FF D8 FF E0
    ```

    ### Identifying the Header

    1. **Header Length (HLE)**: The first byte should indicate the length of the header. However, the first byte `7F` does not seem to match the expected header length. We need to look for a pattern that matches the expected header structure.

    2. **Bit Field Header (BFH)**: The second byte should be the bit field header. We need to look for a byte that matches the expected bit field header structure.

    3. **JPEG Markers**: The JPEG markers `FF D8` (Start of Image) and `FF E0` (Application Marker) should follow the header.

    ### Searching for the Header

    Let's scan the dump for the JPEG markers `FF D8` and `FF E0`:

    ```
    Example:
    00 14 62 81 58 28 A0 FF D8 FF E0
    ```

    We find the markers `FF D8` and `FF E0` at the end of the dump. This suggests that the header should be just before these markers.

    ### Extracting the Header

    Let's extract the bytes just before `FF D8 FF E0`:

    ```
    14 62 81 58 28 A0
    ```

    ### Breakdown of the Header

    1. **Header Length (HLE)**: `14` (20 bytes)
    2. **Bit Field Header (BFH)**: `62`
    - **FID**: 0
    - **EOF**: 0
    - **PTS**: 1 (Presentation Time Stamp present)
    - **SCR**: 1 (Source Clock Reference present)
    - **RES**: 0 (Reserved)
    - **STI**: 0 (Still Image)
    - **ERR**: 0 (Error Bit)
    - **EOH**: 0 (End of Header not set)
    3. **PTS**: `81 58 28 A0` (0xA0285881)
    4. **SCR**: (Not fully present in the dump)

    ### Conclusion

    The header starts at `14 62 81 58 28 A0` and is followed by the JPEG markers `FF D8 FF E0`. The header length is 20 bytes, and the bit field header indicates the presence of PTS and SCR fields. The PTS field is `0xA0285881`.

    This analysis assumes that the header is correctly aligned and that the dump provides enough context to identify the header. If the dump is incomplete or contains additional data, further analysis may be required.


    ******************************************************
    How to re-encode the UVC USB MJPEG fragmented packets:
    ******************************************************
    Need to have the CPU find the FFD8 mark, then copy start of offset and end till the length.
    The `PUSBD_ISO_PACKET_DESCRIPTOR` object structure is the crucial key in reconstructing the UVC USB MJPEG packets.
    The USB protocol makes the USB hardware host controller to fragment and SPAN the data through out the buffer.
    The size and locations for each of the fragmented packets are detailed in the `PUSBD_ISO_PACKET_DESCRIPTOR`.

    Program Logic:
    I need to have the CPU to perform USB fragmented packet reconstruction on the fly. Maybe using SMID might be useful.
    The camera data are fragmented in the buffer and was sent to RAM directly via DMA (Isochronous Transfer).
    Once the transfer has been completed, the CPU needs to open the transferred packet's `PUSBD_ISO_PACKET_DESCRIPTOR` and find the offset and length.
    The CPU then goes to the specific offset. The CPU needs to confirm that the first byte in the offset is "0x0C", else it should throw an error and move to the next packet's offset in it's `PUSBD_ISO_PACKET_DESCRIPTOR`. 
    If the CPU does find "0x0C" in the first byte, it needs to skip 12 bytes and start searching for "0xFFD8" header mark, if it finds it, it will start copying data from "0xFFD8" till the end of the length specified in the `PUSBD_ISO_PACKET_DESCRIPTOR` to another buffer called reconstruct-buffer.
    Next the CPU will then look for the next packet's `PUSBD_ISO_PACKET_DESCRIPTOR` details, it will go to that offset location, skip 12 bytes, and start copying all bytes till the end of the length specified in the `PUSBD_ISO_PACKET_DESCRIPTOR`.
    If a length was specified, the CPU will need to start searching in that packet, else it will search for the next packet's `PUSBD_ISO_PACKET_DESCRIPTOR` details.
    The CPU should start concatenating the data as it goes along.
    The CPU should then look for the end termination "0xFFD9" header mark, if it finds it, it needs to stop and concatenate this to the reconstruct-buffer and extract the reconstruct-buffer to a file called 001.jpeg
    Basically the CPU has parsed from "0xFFD8" header mark which is the start of MJPEG picture will the end which is "0xFFD9".
    If all transfers are complete and no more packets to search for in each transfers, if the CPU only found "0xFFD8" header mark and copied it's data but did not find the end of 0xFFD9, the CPU should report it and then manually place an end bytes 0xFFD9 and extract the reconstruct-buffer to a file called 001.jpeg.


    Prompt build:
    Step-1
    Here is what I need the program to be updated to do now:
    It first needs to create a buffer called `reconstruct-buffer`.
    The size of the buffer will be `totalTransferSize` (which is a ULONG datatype).
    It needs to look in the `USBD_ISO_PACKET_DESCRIPTOR` after all the transfers has been completed in 'WinUsb_ReadIsochPipe()'.
    Once completed, the program will find the first transfer's first packet's offset and length. The CPU will need to determine if the length has any value, if not, it needs to search the next packet.
    If a length was specified, the CPU will go to that packet's offset specified in it's `USBD_ISO_PACKET_DESCRIPTOR`.
    It then needs to confirm if the first byte in the offset is 0C.
    If the first byte is 0x0C, the CPU needs to skep 12 bytes (0x0C) and then it needs to start searching for "0xFFD8" header tag.
    The CPU then needs to copy all data from "0xFFD8" till the end of the length specified in the `USBD_ISO_PACKET_DESCRIPTOR` to another buffer called reconstruct-buffer.

    Now the CPU needs to go to the next packet and look for first byte 0x0C, skip 12 bytes, and then start concatenating the data to the reconstruct-buffer.
    The program will do this for each packet.
    As it goes along concatenating data to the reconstruct buffer, the program then needs to look for the end termination tag '0xFFD9'.
    If the program found the termination tag '0xFFD9', the program should stop concatenating till '0xFFD9' to the reconstruct-buffer and save it to disk.

    //If there are no more transfers and packets to look for, and if it also did not find the '0xFFD9' tag, the program should 




    Step-2: Then place it in a pinned buffer.

    Step-3: Then copy to disk and perform testing wether or not the data is legit.

    Then have the GPU to copy the data and perform MJPEG decoding and inference.

    -------------------------------------------------

    MJPEG byte tags which is 1080:
    0x FF C0 00 11 08 04 38 07 80

    MJPEG byte tags which is 720:
    0x FF C0 00 11 08 02 D0 05 00

    All MJPEG FFD9 EOF (end of frame) must have and UVC header tag as such:
    0C 0E 00 00 00 00 00 00 00 00 73 06

    Where the header bit filed is 0E, this indicates EoF fragmented chunk.

    --------------------------------

    -----------------------
    Bit field header field (BFH):
    -----------------------

    8 7 6 5  4 3 2 1    <----- BFH.
    0 0 0 0  0 0 0 0    <----- In binary.


    FID: Frame Identifier - 1
    This bit toggles at each frame start boundary and stays constant for the rest of the frame.

    EOF: End of Frame - 2
    This bit indicates the end of a video frame and is set in the last video sample that belongs to a
    frame.

    PTS: Presentation Time Stamp - 3
    This bit, when set, indicates the presence of a PTS field.

    SCR: Source Clock Reference - 4
    This bit, when set, indicates the presence of a SCR field

    RES: Reserved. - 5
    Set to 0.

    STI: Still Image - 6
    This bit, when set, identifies a video sample that belongs to a still image.

    ERR: Error Bit - 7
    This bit, when set, indicates an error in the device streaming.

    EOH: End of Header - 8
    This bit, when set, indicates the end of the BFH fields.

    Example:
    UVC Packet header: 0C 0D 00 00 00 00 00 00 00 00 E8 01
                        ^
                        |------ Where "0D" is the BFH, in binary it is: 0000 1101
                                                                                ^
                                                                                |-------------- 1st bit field, of the LSB, is the FID.          


                        0C 0C 00 00 00 00 00 00 00 00 1E 03
                        ^
                        |------ Where "0C" is the BFH, in binary it is: 0000 1100
                                                                                ^
                                                                                |-------------- 2nd bit field, of the LSB, is the EOF, which is not triggered, thus it is not enabled.


                        0C 4C 00 00 00 00 00 00 00 00 1E 03
                        ^
                        |------ Where "4C" is the BFH, in binary it is: 0100 1100
                                                                            ^
                                                                            |-------------- 7th (6th in 0-based index) bit field, of the MSB, is the ERR, which is triggered, thus there is an error.


                        0C 4E 00 00 00 00 00 00 00 00 1E 03
                        ^
                        |------ Where "4E" is the BFH, in binary it is: 0100 1110
                                                                            ^
                                                                            |-------------- 7th (6th in 0-based index) bit field, of the MSB, is the ERR, which is triggered, thus there is an error.
                        

                        0C 0E 00 00 00 00 00 00 00 00 1E 03
                        ^
                        |------ Where "0E" is the BFH, in binary it is: 0000 1110
                                                                                ^
                                                                                |-------------- 2nd bit field, of the LSB, is the EOF, which is triggered, thus it is enabled.

    ---------------------------------------------------------------------------------------

    Program functionality:

    FFPLAY needs to connect to the named pipe for the program to continue operating, else it will stall until connected.:
    ffplay -f mjpeg -i \\.\pipe\jpeg_pipe


    ------------------------------------------------------------------------------------------
*/


#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <Winusb.h>
#include <Usb.h>
#include <cfgmgr32.h>
#include <fstream>
#include <stdio.h>      // For printf
#include <iostream>     // For creating a pipe
#include <sstream>      // For creating a pipe
#include <string>
#include <initguid.h>
#include <sstream>
#include <emmintrin.h> // SSE2 header



#define ISOCH_DATA_SIZE_MS 41             // How long to capture? (Do not use this as any midflight errors will throw off the entire previous data stream and need to start over streaming again).
#define ISOCH_TRANSFER_COUNT 1            // How many instances of requested capture time to be repeated.
int frame_iterator = 2;                   // Ideal timings: ISOCH_DATA_SIZE_MS 28 && frame_iterator = 2, ISOCH_DATA_SIZE_MS 34 && frame_iterator = 2, ISOCH_DATA_SIZE_MS 41 && frame_iterator = 2


DEFINE_GUID(GUID_DEVINTERFACE_USBApplication1, 0xD3CACD43, 0xD2AF, 0x4589, 0x8A, 0x20, 0x7B, 0x1B, 0xB2, 0x08, 0xC4, 0x75); // Device Interface GUID.


// Global variables:-----------------------------------------------------------------------------------------------------

    ULONG totalTransferSize = NULL;

    DWORD lastError = 0;
    LPOVERLAPPED overlapped = NULL;
    LPOVERLAPPED overlapped_2 = NULL;
    LPOVERLAPPED temp_overlapped = NULL;
    DWORD NumberOfBytesTransferred;

    PUCHAR readBuffer = NULL;
    PUCHAR readBuffer_2 = NULL;
    PUCHAR reconstructBuffer = NULL; // New buffer for reconstructed data
    PUCHAR tempBuffer = NULL; // New buffer for reconstructed data
    
    WINUSB_ISOCH_BUFFER_HANDLE isochReadBufferHandle;
    WINUSB_ISOCH_BUFFER_HANDLE isochReadBufferHandle_2;

    PUSBD_ISO_PACKET_DESCRIPTOR isochPackets;
    PUSBD_ISO_PACKET_DESCRIPTOR isochPackets_2;
    PUSBD_ISO_PACKET_DESCRIPTOR temp_isochPackets;

    HANDLE events[2];             // An array of overlapped object handles for WaitForMultipleObjects(). // Here 2 buffers are used, so 2 OVERLAPPED objects are used. These two overlapped stuctures are then fed to the WaitForMultipleObjects().

    //PUCHAR packetStart = NULL; // used for MJPEG frame extraction.
    PUCHAR dataStart = NULL;
    ULONG bufferTracker = NULL;
    ULONG copyLength = NULL;

    bool sof = 1;
    bool nof = 0; // not end of frame.
    bool eof = 0;
    bool first_iteration = 1;
    int file_counter = 1;
    BOOL result = FALSE;
    int bytes_ = 0;

    ULONG i;
    ULONG j;
    ULONG k;
    ULONG frameNumber;
    ULONG startFrame;
    LARGE_INTEGER timeStamp;

    // Start reading from the isoch pipe
    int key_num = 0; // Reset the counter, each count is 1ms of data.
    bool start = 0;
    bool key = 1;
    bool asap = 1;

    int legit_packet = 0;   // iterator to count exactly if the MJPEG frames PARSED is 31 USB frames, else it is a false capture and recapture again.
    bool ffd9 = 0;          // If a ffd9 was parsed, means end of frame.
    bool ffd8 = 0;          // If a ffd8 was parsed, means start of frame.
    bool crap = 0;          // If the capture was crap, then recapture.
    bool recap = 1;         // Recapture key if the cap was crap.

    // A "Named Pipe" instance variables, used for piping JPEGs to FFPLAY:
    DWORD bytesWritten;

// GLobal variables:------------------------------------------------------------------------------------------------------


HANDLE createNamedPipe()    // Initialize windows pipe, so that parsed and extracted JPEGs can be piped to it, such that ffplay can use the named pipe to play the MJPEG stream from the pipe: ----------------------------------------------------
{
    HANDLE hPipe = CreateNamedPipe(
        TEXT("\\\\.\\pipe\\jpeg_pipe"), // Pipe name
        PIPE_ACCESS_OUTBOUND,            // Write access
        PIPE_TYPE_BYTE | PIPE_WAIT,      // Byte-stream pipe, blocking mode
        1,                               // Max instances
        0,                               // Output buffer size
        0,                               // Input buffer size
        0,                               // Default timeout
        NULL                             // Default security attributes
    );

    if (hPipe == INVALID_HANDLE_VALUE) 
    {
        std::cerr << "Failed to create named pipe.\n";
        exit(1);
    }

    return hPipe;
}
// -------------------------------------------------------------------------------------------------------------------------------

typedef struct _DEVICE_DATA {
    BOOL                    HandlesOpen;
    WINUSB_INTERFACE_HANDLE WinusbHandle;
    WINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle;
    HANDLE                  DeviceHandle;
    TCHAR                   DevicePath[MAX_PATH];
    UCHAR                   IsochInPipe;
    ULONG                   IsochInTransferSize;
    ULONG                   IsochInPacketCount;
} DEVICE_DATA, * PDEVICE_DATA;


HRESULT GetIsochPipes(_Inout_ PDEVICE_DATA DeviceData)  // This function obtains and configures the USB device for preparation communication.
{
    BOOL result;
    USB_INTERFACE_DESCRIPTOR usbInterface;
    WINUSB_PIPE_INFORMATION_EX pipe;
    HRESULT hr = S_OK;
    UCHAR i;

    printf("Initiating: `WinUsb_GetAssociatedInterface`\n");

    result = WinUsb_GetAssociatedInterface(DeviceData->WinusbHandle, 0, &DeviceData->AssociatedInterfaceHandle);

    if (result == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        std::cerr << "WinUsb_GetAssociatedInterface failed to get handle for interface 0." << std::endl;
        CloseHandle(DeviceData->WinusbHandle);
        return hr;
    }
    else if (result)
    {
        printf("`WinUsb_GetAssociatedInterface` has been Initiated.\n\n");
    }

    printf("Initiating: `WinUsb_QueryInterfaceSettings`\n");

    result = WinUsb_QueryInterfaceSettings(DeviceData->AssociatedInterfaceHandle, 1, &usbInterface);

    if (result == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        std::cerr << "WinUsb_QueryInterfaceSettings failed to get USB interface for Alternate Setting 1." << std::endl;
        CloseHandle(DeviceData->AssociatedInterfaceHandle);
        return hr;
    }
    else if (result)
    {
        printf("`WinUsb_QueryInterfaceSettings` has been Initiated.\n\n");
    }


    printf("Initiating: `WinUsb_SetCurrentAlternateSetting`\n");

    // Select the alternate setting for the video stream interface
    result = WinUsb_SetCurrentAlternateSetting(DeviceData->AssociatedInterfaceHandle, 1);

    if (result == FALSE)
    {
        printf("`WinUsb_SetCurrentAlternateSetting`: Failed to set alternate setting.\n");
        //WinUsb_Free(&DeviceData->AssociatedInterfaceHandle);
        CloseHandle(&DeviceData->AssociatedInterfaceHandle);
        return hr;
    }
    else if (result)
    {
        printf("`WinUsb_SetCurrentAlternateSetting` has been Initiated.\n\n");
    }


    for (i = 0; i < usbInterface.bNumEndpoints; i++) {
        result = WinUsb_QueryPipeEx(
            DeviceData->AssociatedInterfaceHandle,
            1,
            (UCHAR)i,
            &pipe);

        if (result == FALSE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            printf("WinUsb_QueryPipeEx failed to get USB pipe.\n");
            CloseHandle(DeviceData->DeviceHandle);
            return hr;
        }


        if (pipe.PipeType == UsbdPipeTypeIsochronous)
        {
            // Check if this is an IN endpoint (bit 7 set)
            if ((pipe.PipeId & 0x80) != 0)
            {
                DeviceData->IsochInPipe = pipe.PipeId;
                wprintf(L"Isochronous `IN` Pipe ID: [ %d ]\n", DeviceData->IsochInPipe);
            }
            else
            {
                wprintf(L"Isochronous OUT pipe found (ID: %d), skipping.\n", pipe.PipeId);
                return hr;
            }

            if (pipe.MaximumBytesPerInterval == 0 || (pipe.Interval == 0)) {
                hr = E_INVALIDARG;
                wprintf(L"Isoch Out: MaximumBytesPerInterval or Interval value is 0.\n");
                CloseHandle(DeviceData->DeviceHandle);
                return hr;
            }

            else
            {
                std::wcout << "Isochronous `IN` MaximumBytesPerInterval Size: " << pipe.MaximumBytesPerInterval << std::endl;
                
                // 1ms = 1 frame.
                // (ISOCH_DATA_SIZE_MS = per ms, pipe.MaximumBytesPerInterval = 3072 bytes per microframe, x 8 microframes) = total size of data in terms of total size of Frames.
                // Calculating, how large of data in terms of total amount of time (frames) requested:
                DeviceData->IsochInTransferSize = ISOCH_DATA_SIZE_MS * pipe.MaximumBytesPerInterval * (8 / pipe.Interval);
                std::wcout << "Isochronous `IN` Pipe Total Transfer Size (with requested capture time): " << DeviceData->IsochInTransferSize << std::endl;

                // 1 frame = 8 microframes or (8 x 125 microseconds).
                // Calculating how many microframes in total size of data transfer of time (frames) requested:
                DeviceData->IsochInPacketCount = DeviceData->IsochInTransferSize / pipe.MaximumBytesPerInterval;
                std::wcout << "Isochronous `IN` Pipe Total Packet Count (Microframes Per Transfer): " << DeviceData->IsochInPacketCount << std::endl;
                std::wcout << "Isochronous `IN` Pipe Total Packet Count (Total Microframes): " << DeviceData->IsochInPacketCount * ISOCH_TRANSFER_COUNT<< std::endl;

                // Calculate total transfer size of how many instances should the requested capture time should be repeated:
                totalTransferSize = DeviceData->IsochInTransferSize * ISOCH_TRANSFER_COUNT;
                printf("Total Transfer Size WITH requested repeat amount: %lu\n\n", totalTransferSize);

                std::wcout << "Isochronous Requested Capture Time: " << ISOCH_DATA_SIZE_MS << "ms." << std::endl;
                std::wcout << "Isochronous Requested repeated amount: " << ISOCH_TRANSFER_COUNT << std::endl;

            }
        }
    }
    return hr;
}


VOID Transfer_Ini(_Inout_ PDEVICE_DATA DeviceData)      // This function initializes important variables needed to store, read and transfer data from USB device to system RAM.
{
    // Allocate memory for the read buffer: ---------------------------------------------------------------------------------------------------------
        
        printf("\n\nInitializing readBuffer:\n\n");

        try 
        {
            readBuffer = new UCHAR[totalTransferSize];
            readBuffer_2 = new UCHAR[totalTransferSize];
            tempBuffer = new UCHAR[totalTransferSize];
        } 
        catch (const std::bad_alloc& e) 
        {
            printf("Unable to allocate memory: %s\n", e.what());

            // No need to delete anything here, since the allocation failed

            return; // Exit the program if memory allocation fails
        }

        // Initialize the allocated memory to zero
        ZeroMemory(readBuffer, totalTransferSize);
        ZeroMemory(readBuffer_2, totalTransferSize);
        ZeroMemory(tempBuffer, totalTransferSize);

    // -------------------------------------------------------------------------------------------------------------------------------

    // Allocate memory for overlapped and overlapped_2: ---------------------------------------------------------------------------------

        overlapped = new OVERLAPPED;                                
        overlapped_2 = new OVERLAPPED;                             
        
        // Initialize the allocated memory to zero
        ZeroMemory(overlapped, sizeof(OVERLAPPED));
        ZeroMemory(overlapped_2, sizeof(OVERLAPPED));

        overlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        overlapped_2->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (overlapped->hEvent == NULL || overlapped_2->hEvent == NULL)
        {
            printf("Unable to set event for overlapped operation.\n");
            if (overlapped->hEvent != NULL) CloseHandle(overlapped->hEvent);
            if (overlapped_2->hEvent != NULL) CloseHandle(overlapped_2->hEvent);
            return; // Exit the function if event creation fails
        }
        
        events[0] = overlapped->hEvent;          // An array of overlapped object handles for WaitForMultipleObjects().
        events[1] = overlapped_2->hEvent;
    // -------------------------------------------------------------------------------------------------------------------------------

    // Allocate memory for the reconstruct buffer ---------------------------------------------------------------------------------

        reconstructBuffer = new UCHAR[totalTransferSize];

        if (reconstructBuffer == NULL) 
        {
            printf("Unable to allocate memory for reconstruct buffer.\n");
            if (reconstructBuffer != NULL) 
            {
                delete[] reconstructBuffer;
            }
        }

        ZeroMemory(reconstructBuffer, totalTransferSize);

    // -------------------------------------------------------------------------------------------------------------------------------

    // Isoch packets ---------------------------------------------------------------------------------

        // Allocate memory for the 1st isoch packets:
        isochPackets = new USBD_ISO_PACKET_DESCRIPTOR[DeviceData->IsochInPacketCount];
        ZeroMemory(isochPackets, DeviceData->IsochInPacketCount);


        // Allocate memory for the 2nd isoch packets:
        isochPackets_2 = new USBD_ISO_PACKET_DESCRIPTOR[DeviceData->IsochInPacketCount];
        ZeroMemory(isochPackets_2, DeviceData->IsochInPacketCount);

        // Allocate memory for the temp isoch packets:
        temp_isochPackets = new USBD_ISO_PACKET_DESCRIPTOR[DeviceData->IsochInPacketCount];
        ZeroMemory(temp_isochPackets, DeviceData->IsochInPacketCount);

    // -------------------------------------------------------------------------------------------------------------------------------

    // Register the Isoch Buffer: ------------------------------------------------------------------------------------------------------------------------------------------------------

        printf("Calling: `WinUsb_RegisterIsochBuffer`\n");

        result = WinUsb_RegisterIsochBuffer(
            DeviceData->AssociatedInterfaceHandle,
            DeviceData->IsochInPipe,
            readBuffer,
            DeviceData->IsochInTransferSize,     // Total transfer size of how many instances should the requested capture time should be repeated:
            &isochReadBufferHandle);                                    // PWINUSB_ISOCH_BUFFER_HANDLE IsochBufferHandle

        if (!result)
        {
            lastError = GetLastError();
            printf("Isoch buffer registration failed. Error: %x\n", lastError);
        }
        else if (result)
        {
            printf("`WinUsb_RegisterIsochBuffer` has been Initiated.\n\n");
        }


        result = WinUsb_RegisterIsochBuffer(
        DeviceData->AssociatedInterfaceHandle,
        DeviceData->IsochInPipe,
        readBuffer_2,
        DeviceData->IsochInTransferSize,     // Total transfer size of how many instances should the requested capture time should be repeated:
        &isochReadBufferHandle_2);                                    // PWINUSB_ISOCH_BUFFER_HANDLE IsochBufferHandle

        if (!result)
        {
            lastError = GetLastError();
            printf("Isoch buffer-2 registration failed. Error: %x\n", lastError);
        }
        else if (result)
        {
            printf("`WinUsb_RegisterIsochBuffer_2` has been Initiated.\n\n");
        }
    // -------------------------------------------------------------------------------------------------------------------------------
}


// ------------------------------------------------------------------------------------------------------------------------------------------------------


VOID SendIsochInTransfer(_Inout_ PDEVICE_DATA DeviceData)
{

    // Create a pipe instance: --------------------------------------------------------------------------------------------

        HANDLE hPipe = createNamedPipe();

        // Wait for the client to connect
        std::cout << "Waiting for client to connect to the pipe...\n";
        if (!ConnectNamedPipe(hPipe, NULL)) 
        {
            std::cerr << "Failed to connect to client.\n";
            CloseHandle(hPipe);
            return;
        }
        std::cout << "Client connected.\n";

    //---------------------------------------------------------------------------------------------------------------------------------


    // DMA: --------------------------------------------------------------------------------------------------------

    // 1st Initial UVC USB capture request to DMA Buffer:

        result = WinUsb_GetCurrentFrameNumber(DeviceData->AssociatedInterfaceHandle, &frameNumber, &timeStamp);
        if (!result)
        {
            printf("WinUsb_GetCurrentFrameNumber failed.\n");
        }

        startFrame = frameNumber + frame_iterator;   // Need to request after 2 frames ahead such that the camera does not provide skewed frames.

        printf("Transfer starting at frame %d.\n", startFrame);
        printf("Calling: `WinUsb_ReadIsochPipe`\n\n");

        result = WinUsb_ReadIsochPipe(                              // Fetches 1 frame per call. 0x0 (packet 0) to 0x5400 (packet_7).
            isochReadBufferHandle,
            0,                                                                  // Offset to where should the data be placed in the buffer location.
            DeviceData->IsochInTransferSize,                                    // this parameter is the length PER transfer.
            &startFrame,
            DeviceData->IsochInPacketCount,
            &isochPackets[0],
            &overlapped[0]);

        printf("`WinUsb_ReadIsochPipe` has been Initiated.\n");
        printf("Next transfer frame %d.\n\n", startFrame);

        lastError = GetLastError();

        if (!result && lastError != ERROR_IO_PENDING)       // Error check for `WinUsb_ReadIsochPipe()` If results fail AND there is no pending I/O, then there is an error.
        {
            printf("`WinUsb_ReadIsochPipe` failed. Error: %x\n\n", lastError);

            // Log detailed information about the parameters
            printf("Parameters:\n");
            printf("IsochReadBufferHandle: %p\n", isochReadBufferHandle);
            printf("Offset: %lu\n", DeviceData->IsochInTransferSize * key_num);
            printf("IsochInPacketCount: %lu\n", DeviceData->IsochInPacketCount);
            printf("IsochPackets: %p\n\n", &isochPackets[key_num * DeviceData->IsochInPacketCount]);

            recap = 0;
            return;
        }

    //----------------------------------------------------------------------------------

    // 2nd Initial UVC USB capture request to DMA Buffer:

        result = WinUsb_GetCurrentFrameNumber(DeviceData->AssociatedInterfaceHandle, &frameNumber, &timeStamp);
        if (!result)
        {
            printf("WinUsb_GetCurrentFrameNumber failed.\n");
        }

        startFrame = frameNumber + frame_iterator;   // Need to request after 2 frames ahead such that the camera does not provide skewed frames.

        printf("Transfer starting at frame %d.\n", startFrame);
        printf("Calling: `WinUsb_ReadIsochPipe`\n\n");

        result = WinUsb_ReadIsochPipe(                              // Fetches 1 frame per call. 0x0 (packet 0) to 0x5400 (packet_7).
            isochReadBufferHandle_2,
            0,                                                                  // Offset to where should the data be placed in the buffer location.
            DeviceData->IsochInTransferSize,                                    // this parameter is the length PER transfer.
            &startFrame,
            DeviceData->IsochInPacketCount,
            &isochPackets_2[0],
            &overlapped_2[0]);

        printf("`WinUsb_ReadIsochPipe` has been Initiated.\n");
        printf("Next transfer frame %d.\n\n", startFrame);


        lastError = GetLastError();

        if (!result && lastError != ERROR_IO_PENDING)       // Error check for `WinUsb_ReadIsochPipe()` If results fail AND there is no pending I/O, then there is an error.
        {
            printf("`WinUsb_ReadIsochPipe` failed. Error: %x\n\n", lastError);

            // Log detailed information about the parameters
            printf("Parameters:\n");
            printf("IsochReadBufferHandle: %p\n", isochReadBufferHandle);
            printf("Offset: %lu\n", DeviceData->IsochInTransferSize * key_num);
            printf("IsochInPacketCount: %lu\n", DeviceData->IsochInPacketCount);
            printf("IsochPackets: %p\n\n", &isochPackets[key_num * DeviceData->IsochInPacketCount]);

            recap = 0;
            return;
        }
    
    //----------------------------------------------------------------------------------

    // Asynchronous DMA Requests and Extractions: ----------------------------------------------------------------------------------

    while (true) 
    {
        // Determine which buffer completed: ----------------------------------------------------------------------------------

            DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);  //  Waits for one or all of the specified objects (e.g., events, in this case the overlapped structures) to be in the signaled state. This is used for signaling when one buffer is filled and ready to be extracted while the other buffer is being filled, thus asynchronous operation. 
            if (waitResult == WAIT_FAILED) 
            {
                printf("Wait failed.\n");
                break;
            }
        //----------------------------------------------------------------------------------

        // Process buffer 1 *********************************************************************************************************************************************************************************************************************************************************************************************************************************************

            if (waitResult == WAIT_OBJECT_0)         // Process buffer1 .....................................................................................
            {
                
                //  Need to call WinUsb_GetOverlappedResult() before calling WinUsb_ReadIsochPipe(). This is because data needs to be extracted before overwriting the overlapped structures for a new DMA read fill request. Thus code base at this phase must be short as possible, at least shorter than 16ms. Possibly copy the dma buffer to temp buffer... then perform algo logic to the temp buffer. 

                    // result = WinUsb_GetOverlappedResult(            // Obtain the results from the overlapped stucture for buffer 1.
                    // DeviceData->AssociatedInterfaceHandle,          // An opaque handle to the first interface on the device, which is returned by WinUsb_Initialize.
                    // &overlapped[0],                                 // A pointer to an OVERLAPPED structure that was specified when the overlapped operation was started.
                    // &NumberOfBytesTransferred,                      // A pointer to a variable that receives the number of bytes that were actually transferred by a read or write operation.
                    // TRUE);                                          // If this parameter is TRUE, the function does not return until the operation has been completed. If this parameter is FALSE and the operation is still pending, the function returns FALSE and the GetLastError function returns ERROR_IO_INCOMPLETE.

                    // lastError = GetLastError();

                    // if (!result && lastError != ERROR_IO_PENDING && lastError != ERROR_IO_INCOMPLETE)   // Error check for `WinUsb_GetOverlappedResult()`.
                    // {
                    //     printf("`WinUsb_GetOverlappedResult` - BUFFER=[1] - Failed to read with error code: %x\n", lastError);
                    //     printf("Parameters:\n");
                    //     printf("AssociatedInterfaceHandle: %p\n", DeviceData->AssociatedInterfaceHandle);
                    //     printf("IsochReadBufferHandle: %p\n", isochReadBufferHandle);
                    //     printf("Offset: %lu\n", DeviceData->IsochInTransferSize);
                    //     printf("IsochInPacketCount: %lu\n", DeviceData->IsochInPacketCount);
                    //     printf("IsochPackets: %p\n", &isochPackets[0]);

                    //     // return;
                    //     // continue;
                    // }

                //----------------------------------------------------------------------------------

                // Print the overlapped results for Buffer 1 - Debugging - Obtain the USB Packet table for buffer 1: ------------------------------------------------------------------------------------------      

                    // bytes_ = 0;
                    // for (j = 0; j < DeviceData->IsochInPacketCount; j++)
                    // {   
                    //     bytes_ = bytes_ + isochPackets[j].Length;

                    //     // Print the values of Offset, Length, and Status for each packet, Access each array's variables:
                    //     printf("--------------------------- BUFFER-[1]- Packet %d:\n", j);
                    //     printf("Offset: %lx\n", isochPackets[j].Offset);      // Display packet's offset in main buffer as in Hex form.
                    //     printf("Length: %lu\n", isochPackets[j].Length);
                    //     printf("Status: %lu\n", isochPackets[j].Status);
                    // }
                    // printf("Requested %d bytes in %d packets per transfer.\n", DeviceData->IsochInTransferSize, DeviceData->IsochInPacketCount);
                    // printf("Transfer 1 completed. Read %d bytes. \n\n", bytes_);
                
                // -----------------------------------------------------------------------------

                // Request Buffer-1 for another UVC USB Read

                    // Copy data from isochPackets to temp_isochPackets. This is needed so that we can quickly start capturing camera JPEGs and not have the camera to drop JPEG due to high latency of no buffer ready. 
                    memcpy(temp_isochPackets, isochPackets, DeviceData->IsochInPacketCount * sizeof(USBD_ISO_PACKET_DESCRIPTOR));

                    // Copy data from readBuffer to tempBuffer. This is needed so that we can quickly start capturing camera JPEGs and not have the camera to drop JPEG due to high latency of no buffer ready. The reconstruction algo introduces latency, thus stalling the main buffer due to utilzing it. Thus having the main buffer copied to a temp buffer, frees the main buffer and have the reconstruction algo to use the temp buffer to parse the UVS USB camera fragmented MJPEG data. 
                    memcpy(tempBuffer, readBuffer, totalTransferSize);

                    result = WinUsb_GetCurrentFrameNumber(DeviceData->AssociatedInterfaceHandle, &frameNumber, &timeStamp);     // Get the current frame number for resubmitting buffer 1
                    if (!result)
                    {
                        printf("WinUsb_GetCurrentFrameNumber failed, buffer_1.\n");
                    }

                    startFrame = frameNumber + frame_iterator;

                    result = WinUsb_ReadIsochPipe(                              // Fetches 1 frame per call. 0x0 (packet 0) to 0x5400 (packet_7).
                        isochReadBufferHandle,
                        0,                                                      // Offset to where should the data be placed in the buffer location.
                        DeviceData->IsochInTransferSize,                        // this parameter is the length PER transfer.
                        &startFrame,
                        DeviceData->IsochInPacketCount,
                        &isochPackets[0],
                        &overlapped[0]);

                    if (!result && GetLastError() != ERROR_IO_PENDING) 
                    {
                        printf("Failed to resubmit read request for buffer 1.\n");
                        break;
                    }

                // -----------------------------------------------------------------------

                // Reconstruction Algo - MJPEG Parser - Buffer 1: ------------------------------------------------------------------------------------------------

                    for (int i = 0; i < DeviceData->IsochInPacketCount; i++) { // Level-1, iterate each packet in the transfer:
                        if (temp_isochPackets[i].Length == 0) { // If first byte has no data, Skip packet.
                            continue;
                        }

                        uint8_t* dataStart = tempBuffer + temp_isochPackets[i].Offset + 12; // Get the packet's offset and increment by 12 bytes to avoid reading the tag headers.

                        if (ffd8 == 1 && (tempBuffer[temp_isochPackets[i].Offset + 1] & (1 << 6))) { // Check if 2nd byte has header tag of errors.
                            break; // Terminate the for loop @ level 1.
                        }

                        if (eof && (tempBuffer[temp_isochPackets[i].Offset + 1] & (1 << 1))) { // Check if 7th byte has header tag's bit field has the EOF triggered.
                            memcpy(reconstructBuffer + bufferTracker, dataStart, temp_isochPackets[i].Length - 12);
                            bufferTracker += temp_isochPackets[i].Length - 12;
                            eof = 0;
                            ffd9 = 1;
                            ffd8 = 0;
                            goto LEVEL_0_BUFFER_1;
                        }

                        if (ffd8) { // This scope block only contains legit packets, this count it.
                            legit_packet++;
                        }

                        if (ffd8 != 1) { // Search for 0xFFD8 data:
                            for (int k = 0; k < temp_isochPackets[i].Length - 12; k++) { // Search through the packet's data. Prevent buffer overflow `-12`.
                                if (dataStart[k] == 0xFF && dataStart[k + 1] == 0xD8) { // Find the Start of Frame tag (0xFFD8).
                                    bufferTracker = temp_isochPackets[i].Length - 12 - k;
                                    memcpy(reconstructBuffer, dataStart + k, bufferTracker);
                                    eof = 1;
                                    ffd8 = 1;
                                    goto LEVEL_1_BUFFER_1;
                                }
                            }
                        } else if (ffd8 == 1 && ffd9 != 1) { // Copy the MJPEG body data only after it found the first 0xFFD8 MJPEG SoF tag.
                            memcpy(reconstructBuffer + bufferTracker, dataStart, temp_isochPackets[i].Length - 12);
                            bufferTracker += temp_isochPackets[i].Length - 12;
                        }

                        LEVEL_1_BUFFER_1:;
                    }

                    LEVEL_0_BUFFER_1:;

                // --------------------------------------------------------------------------------------------------------

                // MJPEG Exporter: ------------------------------------------------------------------------------------------------

                    if (ffd9 == 1)  // Perform buffer extraction to JPEG pic, if only data captured are legit, the specific UVC USB cam used, only provides 31 USB Frames to produce a complete MJPEG pic frame.
                    {
                        //printf("\n\n ***** Buffer-1 - Extracting data to JPEG:... ***** \n\n");
                        
                        //printf("`ffd9` is: [%d]\n" , ffd9);
                        //printf("`legit_packet` count is: [%d]\n\n" , legit_packet);

                        // Write JPEGs to a named Pipe: ----------------------------------------------------------------------------------------

                            if (!WriteFile(hPipe, reconstructBuffer, totalTransferSize, &bytesWritten, NULL)) 
                            {
                                std::cerr << "Failed to write to named pipe.\n";
                            } 
                            else 
                            {
                                //std::cout << "Successfully wrote " << bytesWritten << " bytes to the pipe.\n\n";
                                legit_packet = 0;   // Reset the legit packet count.
                                eof = 0;
                                ffd9 = 0;
                                ffd8 = 0;
                            }
                        // --------------------------------------------------------------------------------------------------------

                        // Write JPEGs to a local disk file - debugging: ----------------------------------------------------------------------------------------

                            // std::stringstream ss;
                            // ss << "reconstructed_buffer_1_" << file_counter << ".JPEG";
                            // std::string filename = ss.str();

                            // // Dump the reconstructed data to a file
                            // std::ofstream outFile(filename, std::ios::binary);
                            // if (outFile) 
                            // {
                            //     std::cerr << "Attempting to write JPEG data to file:\n";
                            //     outFile.write(reinterpret_cast<char*>(reconstructBuffer), totalTransferSize);
                            //     outFile.close();
                            //     std::cerr << "Reconstructed data has been written to file successfully:\n\n";
                            // }
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }

                        // --------------------------------------------------------------------------------------------------------

                        // Dump the data to a file - debugging: ----------------------------------------------------------------------

                            // std::ofstream outFile_2("isoch_data_buffer_1.bin", std::ios::binary);
                            // if (outFile_2) 
                            // {
                            //     std::cerr << "Attempting to write buffer data to file:\n";
                            //     outFile_2.write(reinterpret_cast<char*>(tempBuffer), totalTransferSize);
                            //     outFile_2.close();
                            //     std::cerr << "Main Buffer data has been written to file successfully:\n\n";
                            // } 
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }

                        // ---------------------------------------------------------------------------------------------------

                        // Configs/Switches: ------------------------------------------------------------------------------------------------------------

                            // legit_packet = 0;   // Reset the legit packet count.
                            // key = 1;            // Make key 1 so that it can do another usb data retrival.    
                            // crap = 1;           // 
                            // recap = 1;          // continue for another recap.          <---------------------- Main toggle. Check the if condition for error check in UVC bit fields.
                            // sof = 1;            // enable for parser to serach for ffd8.
                            // eof = 0;
                            // ffd9 = 0;
                            // ffd8 = 0;
                            // Sleep(10);
                            // break;
                        //------------------------------------------------------------------------------------------------------------------------------------
                    }

                    else
                    {
                        // printf("\n\n Buffer-1 UVC USB Cam Captured crap data. Retry again...\n\n");
                        // printf("`ffd9` is: [%d]\n" , ffd9);
                        // printf("`legit_packet` count is: [%d]\n\n" , legit_packet);
                        
                        legit_packet = 0;   // Reset the legit packet count.
                        // key = 1;            // Make key 1 so that it can do another usb data retrival.    
                        // crap = 1;           // 
                        // recap = 1;          // continue for another recap.          <---------------------- Main toggle. Check the if condition for error check in UVC bit fields.
                        //sof = 1;            // enable for parser to serach for ffd8.
                        eof = 0;
                        ffd9 = 0;
                        ffd8 = 0;

                        // Dump the data to a file - debugging: ----------------------------------------------------------------------

                            // Dump the data to a file
                            // std::ofstream outFile_2("isoch_data_buffer_1.bin", std::ios::binary);
                            // if (outFile_2) 
                            // {
                            //     std::cerr << "Attempting to write buffer data to file:\n";
                            //     outFile_2.write(reinterpret_cast<char*>(tempBuffer), totalTransferSize);
                            //     outFile_2.close();
                            //     std::cerr << "Main Buffer data has been written to file successfully:\n\n";
                            // } 
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }
                        
                        // ---------------------------------------------------------------------------------------------------
                    
                    }

                // -----------------------------------------------------------------------------

            }   // Buffer 1 logic if () scope.

        //*********************************************************************************************************************************************************************************************************************************************************************************************************************************************


        // Process buffer 2 *********************************************************************************************************************************************************************************************************************************************************************************************************************************************

            else if (waitResult == WAIT_OBJECT_0 + 1)   // Process buffer 2 .....................................................................................
            {

                //  Need to call WinUsb_GetOverlappedResult() before calling WinUsb_ReadIsochPipe(). This is because data needs to be extracted before overwritting the overlapped sturctures for a new DMA read fill request. Thus code base at this phase must be short as possible, at least shorter than 16ms. Possibly copy the dma buffer to temp buffer... then perform algo logic to the temp buffer. 

                    // result = WinUsb_GetOverlappedResult(            // Obtain the results from the overlapped stucture for buffer 1.
                    // DeviceData->AssociatedInterfaceHandle,          // An opaque handle to the first interface on the device, which is returned by WinUsb_Initialize.
                    // &overlapped_2[0],                               // A pointer to an OVERLAPPED structure that was specified when the overlapped operation was started.
                    // &NumberOfBytesTransferred,                      // A pointer to a variable that receives the number of bytes that were actually transferred by a read or write operation.
                    // TRUE);                                          // If this parameter is TRUE, the function does not return until the operation has been completed. If this parameter is FALSE and the operation is still pending, the function returns FALSE and the GetLastError function returns ERROR_IO_INCOMPLETE.

                    // lastError = GetLastError();

                    // if (!result && lastError != ERROR_IO_PENDING && lastError != ERROR_IO_INCOMPLETE)   // Error check for `WinUsb_GetOverlappedResult()`.
                    // {
                    //     printf("`WinUsb_GetOverlappedResult` - BUFFER=[2] - Failed to read with error code: %x\n", lastError);
                    //     printf("Parameters:\n");
                    //     printf("AssociatedInterfaceHandle: %p\n", DeviceData->AssociatedInterfaceHandle);
                    //     printf("IsochReadBufferHandle: %p\n", isochReadBufferHandle_2);
                    //     printf("Offset: %lu\n", DeviceData->IsochInTransferSize);
                    //     printf("IsochInPacketCount: %lu\n", DeviceData->IsochInPacketCount);
                    //     printf("IsochPackets: %p\n", &isochPackets_2[0]);

                    //     //    return;
                    //     //    continue;
                    // }

                //----------------------------------------------------------------------------------

                // Print the overlapped results for Buffer 2 - Debugging - Obtain the USB Packet table for buffer 2: ------------------------------------------------------------------------------------------      
                
                    // bytes_ = 0;
                    // for (j = 0; j < DeviceData->IsochInPacketCount; j++)
                    // {   
                    //     bytes_ = bytes_ + isochPackets_2[j].Length;

                    //     // Print the values of Offset, Length, and Status for each packet, Access each array's variables:
                    //     printf("--------------------------- BUFFER-[2]- Packet %d:\n", j);
                    //     printf("Offset: %lx\n", isochPackets_2[j].Offset);      // Display packet's offset in main buffer as in Hex form.
                    //     printf("Length: %lu\n", isochPackets_2[j].Length);
                    //     printf("Status: %lu\n", isochPackets_2[j].Status);
                    // }
                    // printf("Requested %d bytes in %d packets per transfer.\n", DeviceData->IsochInTransferSize, DeviceData->IsochInPacketCount);
                    // printf("Transfer 1 completed. Read %d bytes. \n\n", bytes_);

                //---------------------------------------------------------------------

                // Now Request Buffer-2 for another UVC USB Read.

                    // Copy data from isochPackets_2 to temp_isochPackets. This is needed so that we can quickly start capturing camera JPEGs and not have the camera to drop JPEG due to high latency of no buffer ready. 
                    memcpy(temp_isochPackets, isochPackets_2, DeviceData->IsochInPacketCount * sizeof(USBD_ISO_PACKET_DESCRIPTOR));

                    // Copy data from readBuffer_2 to tempBuffer. This is needed so that we can quickly start capturing camera JPEGs and not have the camera to drop JPEG due to high latency of no buffer ready. The reconstruction algo introduces latency, thus stalling the main buffer due to utilzing it. Thus having the main buffer copied to a temp buffer, frees the main buffer and have the reconstruction algo to use the temp buffer to parse the UVS USB camera fragmented MJPEG data. 
                    memcpy(tempBuffer, readBuffer_2, totalTransferSize);

                    result = WinUsb_GetCurrentFrameNumber(DeviceData->AssociatedInterfaceHandle, &frameNumber, &timeStamp);
                    if (!result)
                    {
                        printf("WinUsb_GetCurrentFrameNumber failed, buffer_2.\n");
                    }

                    startFrame = frameNumber + frame_iterator;

                    result = WinUsb_ReadIsochPipe(                              // Fetches 1 frame per call. 0x0 (packet 0) to 0x5400 (packet_7).
                        isochReadBufferHandle_2,
                        0,                                                      // Offset to where should the data be placed in the buffer location.
                        DeviceData->IsochInTransferSize,                        // this parameter is the length PER transfer.
                        &startFrame,
                        DeviceData->IsochInPacketCount,
                        &isochPackets_2[0],
                        &overlapped_2[0]);

                    if (!result && GetLastError() != ERROR_IO_PENDING) 
                    {
                        printf("Failed to resubmit read request for buffer 2.\n");
                        break;
                    }

                // --------------------------------------------------------------------------------------------------------

                // Reconstruction Algo - MJPEG Parser - Buffer 2: ------------------------------------------------------------------------------------------------

                    for (int i = 0; i < DeviceData->IsochInPacketCount; i++) { // Level-1, iterate each packet in the transfer:
                        if (temp_isochPackets[i].Length == 0) { // If first byte has no data, Skip packet.
                            continue;
                        }

                        uint8_t* dataStart = tempBuffer + temp_isochPackets[i].Offset + 12; // Get the packet's offset and increment by 12 bytes to avoid reading the tag headers.

                        if (ffd8 == 1 && (tempBuffer[temp_isochPackets[i].Offset + 1] & (1 << 6))) { // Check if 2nd byte has header tag of errors.
                            break; // Terminate the for loop @ level 1.
                        }

                        if (eof && (tempBuffer[temp_isochPackets[i].Offset + 1] & (1 << 1))) { // Check if 7th byte has header tag's bit field has the EOF triggered.
                            memcpy(reconstructBuffer + bufferTracker, dataStart, temp_isochPackets[i].Length - 12);
                            bufferTracker += temp_isochPackets[i].Length - 12;
                            eof = 0;
                            ffd9 = 1;
                            ffd8 = 0;
                            goto LEVEL_0_BUFFER_2;
                        }

                        if (ffd8) { // This scope block only contains legit packets, this count it.
                            legit_packet++;
                        }

                        if (ffd8 != 1) { // Search for 0xFFD8 data:
                            for (int k = 0; k < temp_isochPackets[i].Length - 12; k++) { // Search through the packet's data. Prevent buffer overflow `-12`.
                                if (dataStart[k] == 0xFF && dataStart[k + 1] == 0xD8) { // Find the Start of Frame tag (0xFFD8).
                                    bufferTracker = temp_isochPackets[i].Length - 12 - k;
                                    memcpy(reconstructBuffer, dataStart + k, bufferTracker);
                                    eof = 1;
                                    ffd8 = 1;
                                    goto LEVEL_1_BUFFER_2;
                                }
                            }
                        } else if (ffd8 == 1 && ffd9 != 1) { // Copy the MJPEG body data only after it found the first 0xFFD8 MJPEG SoF tag.
                            memcpy(reconstructBuffer + bufferTracker, dataStart, temp_isochPackets[i].Length - 12);
                            bufferTracker += temp_isochPackets[i].Length - 12;
                        }

                        LEVEL_1_BUFFER_2:;
                    }

                    LEVEL_0_BUFFER_2:;


                // --------------------------------------------------------------------------------------------------------

                // MJPEG Exporter: ------------------------------------------------------------------------------------------------

                    if (ffd9 == 1)                             // Perform buffer extraction to JPEG pic, if only data captured are legit, the specific UVC USB cam used, only provides 31 USB Frames to produce a complete MJPEG pic frame.
                    {
                        //printf("\n\n ***** Buffer-2 - Extracting data to JPEG:... ***** \n\n");
                        
                        //printf("`ffd9` is: [%d]\n" , ffd9);
                        //printf("`legit_packet` count is: [%d]\n\n" , legit_packet);

                        // Write JPEGs to a named Pipe: ----------------------------------------------------------------------------------------

                            if (!WriteFile(hPipe, reconstructBuffer, totalTransferSize, &bytesWritten, NULL)) 
                            {
                                std::cerr << "Failed to write to named pipe.\n";
                            } 
                            else 
                            {
                                // std::cout << "Successfully wrote " << bytesWritten << " bytes to the pipe.\n\n";
                                legit_packet = 0;   // Reset the legit packet count.
                                eof = 0;
                                ffd9 = 0;
                                ffd8 = 0;
                            }
                        // --------------------------------------------------------------------------------------------------------

                        // Write JPEGs to a local disk file - debugging: ----------------------------------------------------------------------------------------

                            // std::stringstream ss;
                            // ss << "reconstructed_buffer_2_" << file_counter << ".JPEG";
                            // std::string filename = ss.str();

                            // // Dump the reconstructed data to a file
                            // std::ofstream outFile(filename, std::ios::binary);
                            // if (outFile) 
                            // {
                            //     std::cerr << "Attempting to write JPEG data to file:\n";
                            //     outFile.write(reinterpret_cast<char*>(reconstructBuffer), totalTransferSize);
                            //     outFile.close();
                            //     std::cerr << "Reconstructed data has been written to file successfully:\n\n";
                            // }
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }

                        // --------------------------------------------------------------------------------------------------------

                        // Dump the data to a file - debugging: ----------------------------------------------------------------------

                            // std::ofstream outFile_2("isoch_data_buffer_2.bin", std::ios::binary);
                            // if (outFile_2) 
                            // {
                            //     std::cerr << "Attempting to write buffer data to file:\n";
                            //     outFile_2.write(reinterpret_cast<char*>(tempBuffer), totalTransferSize);
                            //     outFile_2.close();
                            //     std::cerr << "Main Buffer data has been written to file successfully:\n\n";
                            // } 
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }

                        // --------------------------------------------------------------------------------------------------------

                        // Configs/Switches: ------------------------------------------------------------------------------------------------------------

                            // legit_packet = 0;   // Reset the legit packet count.
                            // key = 1;            // Make key 1 so that it can do another usb data retrival.    
                            // crap = 1;           // 
                            // recap = 1;          // continue for another recap.          <---------------------- Main toggle. Check the if condition for error check in UVC bit fields.
                            // sof = 1;            // enable for parser to serach for ffd8.
                            // eof = 0;
                            // ffd9 = 0;
                            // ffd8 = 0;
                            // Sleep(10);
                            // break;

                        // --------------------------------------------------------------------------------------------------------

                    }

                    else
                    {
                        // printf("\n\n Buffer-2 - UVC USB Cam Captured crap data. Retry again...\n\n");
                        // printf("`ffd9` is: [%d]\n" , ffd9);
                        // printf("`legit_packet` count is: [%d]\n\n" , legit_packet);
                        
                        legit_packet = 0;   // Reset the legit packet count.
                        // key = 1;            // Make key 1 so that it can do another usb data retrival.    
                        // crap = 1;           // 
                        // recap = 1;          // continue for another recap.          <---------------------- Main toggle. Check the if condition for error check in UVC bit fields.
                        // sof = 1;            // enable for parser to serach for ffd8.
                        eof = 0;
                        ffd9 = 0;
                        ffd8 = 0;

                        // Dump the data to a file - debugging: ----------------------------------------------------------------------

                            // std::ofstream outFile_2("isoch_data_buffer_2.bin", std::ios::binary);
                            // if (outFile_2) 
                            // {
                            //     std::cerr << "Attempting to write buffer data to file:\n";
                            //     outFile_2.write(reinterpret_cast<char*>(tempBuffer), totalTransferSize);
                            //     outFile_2.close();
                            //     std::cerr << "Main Buffer data has been written to file successfully:\n\n";
                            // } 
                            // else 
                            // {
                            //     std::cerr << "Failed to open file for writing.\n";
                            // }

                        // -----------------------------------------------------------------------

                    }

                // -----------------------------------------------------------------------

            }   // Buffer 2 else () scope.

        //*********************************************************************************************************************************************************************************************************************************************************************************************************************************************


    }   // Main While () loop.


} // Main function call.


//--------------------------------------

    HRESULT OpenDevice(
        _Out_ PDEVICE_DATA DeviceData,
        _Out_opt_ PBOOL FailureDeviceNotFound);

    VOID CloseDevice(
        _Inout_ PDEVICE_DATA DeviceData);


    HRESULT RetrieveDevicePath(
        _Out_bytecap_(BufLen) LPTSTR DevicePath,
        _In_                  ULONG  BufLen,
        _Out_opt_             PBOOL  FailureDeviceNotFound);


    HRESULT OpenDevice(
        _Out_     PDEVICE_DATA DeviceData,
        _Out_opt_ PBOOL        FailureDeviceNotFound)
    {
        HRESULT hr = S_OK;
        BOOL    bResult;

        DeviceData->HandlesOpen = FALSE;

        hr = RetrieveDevicePath(DeviceData->DevicePath,
            sizeof(DeviceData->DevicePath),
            FailureDeviceNotFound);

        if (FAILED(hr)) {
            return hr;
        }

        DeviceData->DeviceHandle = CreateFile(DeviceData->DevicePath,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);

        if (INVALID_HANDLE_VALUE == DeviceData->DeviceHandle) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            return hr;
        }

        bResult = WinUsb_Initialize(DeviceData->DeviceHandle,
            &DeviceData->WinusbHandle);

        if (FALSE == bResult) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(DeviceData->DeviceHandle);
            return hr;
        }

        DeviceData->HandlesOpen = TRUE;
        return hr;
    }

    VOID CloseDevice(_Inout_ PDEVICE_DATA DeviceData)
    {
        if (FALSE == DeviceData->HandlesOpen) 
        {
            return;
        }

        WinUsb_Free(DeviceData->WinusbHandle);
        CloseHandle(DeviceData->DeviceHandle);
        DeviceData->HandlesOpen = FALSE;

        return;
    }

    HRESULT RetrieveDevicePath(
        _Out_bytecap_(BufLen) LPTSTR DevicePath,
        _In_                  ULONG  BufLen,
        _Out_opt_             PBOOL  FailureDeviceNotFound)

    {
        CONFIGRET cr = CR_SUCCESS;
        HRESULT   hr = S_OK;
        PTSTR     DeviceInterfaceList = NULL;
        ULONG     DeviceInterfaceListLength = 0;

        if (NULL != FailureDeviceNotFound) {
            *FailureDeviceNotFound = FALSE;
        }

        do {
            cr = CM_Get_Device_Interface_List_Size(&DeviceInterfaceListLength,
                (LPGUID)&GUID_DEVINTERFACE_USBApplication1,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

            if (cr != CR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
                break;
            }

            DeviceInterfaceList = (PTSTR)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                DeviceInterfaceListLength * sizeof(TCHAR));

            if (DeviceInterfaceList == NULL) {
                hr = E_OUTOFMEMORY;
                break;
            }

            cr = CM_Get_Device_Interface_List((LPGUID)&GUID_DEVINTERFACE_USBApplication1,
                NULL,
                DeviceInterfaceList,
                DeviceInterfaceListLength,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

            if (cr != CR_SUCCESS) {
                HeapFree(GetProcessHeap(), 0, DeviceInterfaceList);

                if (cr != CR_BUFFER_SMALL) {
                    hr = HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
                }
            }
        } while (cr == CR_BUFFER_SMALL);

        if (FAILED(hr)) {
            return hr;
        }

        if (*DeviceInterfaceList == TEXT('\0')) {
            if (NULL != FailureDeviceNotFound) {
                *FailureDeviceNotFound = TRUE;
            }

            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            HeapFree(GetProcessHeap(), 0, DeviceInterfaceList);
            return hr;
        }

        hr = StringCbCopy(DevicePath,
            BufLen,
            DeviceInterfaceList);

        HeapFree(GetProcessHeap(), 0, DeviceInterfaceList);

        return hr;
    }

//------------------------------------------------------------------------

int __cdecl wmain(int     Argc, wchar_t* Argv[])
{
    DEVICE_DATA           deviceData;
    HRESULT               hr;
    USB_DEVICE_DESCRIPTOR deviceDesc;
    BOOL                  bResult;
    BOOL                  noDevice;
    ULONG                 lengthReceived;

    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

    hr = OpenDevice(&deviceData, &noDevice);

    if (FAILED(hr)) 
    {
        if (noDevice) 
        {
            wprintf(L"Device not connected or driver not installed\n");
        }
        else 
        {
            wprintf(L"Failed looking for device, HRESULT 0x%x\n", hr);
        }

        return 0;
    }

    bResult = WinUsb_GetDescriptor(deviceData.WinusbHandle,
        USB_DEVICE_DESCRIPTOR_TYPE,
        0,
        0,
        (PBYTE)&deviceDesc,
        sizeof(deviceDesc),
        &lengthReceived);

    if (FALSE == bResult || lengthReceived != sizeof(deviceDesc)) 
    {
        wprintf(L"Error among LastError %d or lengthReceived %d\n",
        FALSE == bResult ? GetLastError() : 0,
        lengthReceived);
        CloseDevice(&deviceData);
        return 0;
    }

    printf("\nDevice found: VID_%04X&PID_%04X; bcdUsb %04X\n\n",
        deviceDesc.idVendor,
        deviceDesc.idProduct,
        deviceDesc.bcdUSB);

    hr = GetIsochPipes(&deviceData);    // Get UVS USB device info.

    Transfer_Ini(&deviceData); // Initialize all parameters needed to call WinUSB call functions.

    //while (recap)

    while (true)
    {
        printf("\nIn while loop in main scope block. \n\n");

        if (SUCCEEDED(hr))
        {
            // Transfer_Ini(&deviceData); // Initialize all parameters needed to call WinUSB call functions.
            SendIsochInTransfer(&deviceData);
        }
        else 
        {
            wprintf(L"GetIsochPipes failed with HRESULT: 0x%08X\n", hr);
        }
    }

   printf("\nPerforming cleanup. \n\n");

    delete[] readBuffer;
    delete[] readBuffer_2;
    delete[] tempBuffer;

    result = WinUsb_UnregisterIsochBuffer(isochReadBufferHandle);    // When using Windows WinUSB to capture data from a UVC USB camera with overlapped structures and WaitForMultipleObjects(), you typically do not need to unregister the buffer every time it is filled with data. Instead, you can register the buffer once and reuse it for multiple read operations. Unregistering and re-registering buffers on each read would add unnecessary overhead.
    if (result = 0)
    {
        printf("Failed to unregister buffer handle.\n\n");
    }

    if (reconstructBuffer != NULL) 
    {
        delete[] reconstructBuffer;
        reconstructBuffer = nullptr;
    }

        if (isochPackets != NULL) 
    {
        delete[] isochPackets;
        isochPackets = nullptr;
    }

    if (overlapped->hEvent != NULL) 
    {
        CloseHandle(overlapped->hEvent);
    }

    if (overlapped_2->hEvent != NULL) 
    {
        CloseHandle(overlapped_2->hEvent);
    }

    if (overlapped != NULL) 
    {
        delete[] overlapped;
        overlapped = nullptr;
    }

    printf("\nClosing Device. \n\n");

    CloseDevice(&deviceData);
    return 0;
}