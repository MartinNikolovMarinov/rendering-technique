#include "test_utils.h"

#include "core_init.h"

void compareFilesBytewise(const char* fileA, const char* fileB) {
    auto fileADesc = Unpack(core::fileOpen(fileA, core::OpenMode::Read));
    defer { Expect(core::fileClose(fileADesc)); };

    auto fileBDesc = Unpack(core::fileOpen(fileB, core::OpenMode::Read));
    defer { Expect(core::fileClose(fileBDesc)); };

    addr_size sizeA = Unpack(core::fileSize(fileADesc));
    addr_size sizeB = Unpack(core::fileSize(fileBDesc));

    AssertFmt(
        sizeA == sizeB,
        "Snapshot size mismatch: '{}' has {}, '{}' has {}",
        fileA, sizeA, fileB, sizeB
    );

    constexpr addr_size kChunkSize = 4096;
    u8 bufferA[kChunkSize];
    u8 bufferB[kChunkSize];
    addr_size remaining = sizeA;
    addr_size offset = 0;

    while (remaining > 0) {
        addr_size chunk = remaining < kChunkSize ? remaining : kChunkSize;
        addr_size readA = Unpack(core::fileRead(fileADesc, bufferA, chunk));
        addr_size readB = Unpack(core::fileRead(fileBDesc, bufferB, chunk));

        AssertFmt(
            readA == chunk,
            "Short read for '{}': expected {}, got {}",
            fileA, chunk, readA
        );
        AssertFmt(
            readB == chunk,
            "Short read for '{}': expected {}, got {}",
            fileB, chunk, readB
        );

        if (core::memcmp(bufferA, bufferB, chunk) != 0) {
            addr_size diffIdx = 0;
            for (; diffIdx < chunk; diffIdx++) {
                if (bufferA[diffIdx] != bufferB[diffIdx]) {
                    break;
                }
            }

            AssertFmt(
                false,
                "Snapshot mismatch at byte {}: '{}' has {}, '{}' has {}",
                offset + diffIdx,
                fileA,
                static_cast<u32>(bufferA[diffIdx]),
                fileB,
                static_cast<u32>(bufferB[diffIdx])
            );
        }

        remaining -= chunk;
        offset += chunk;
    }
}
