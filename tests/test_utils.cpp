#include "test_utils.h"

#include "core_init.h"

void compareFilesBytewise(const char* fileA, const char* fileB) {
    auto fileADesc = core::Unpack(
        core::fileOpen(fileA, core::OpenMode::Read),
        "Failed to open file '{}'",
        fileA
    );
    defer { core::Expect(core::fileClose(fileADesc)); };

    auto fileBDesc = core::Unpack(
        core::fileOpen(fileB, core::OpenMode::Read),
        "Failed to open file '{}'",
        fileB
    );
    defer { core::Expect(core::fileClose(fileBDesc)); };

    addr_size sizeA = core::Unpack(
        core::fileSize(fileADesc),
        "Failed to get size for file '{}'",
        fileA
    );
    addr_size sizeB = core::Unpack(
        core::fileSize(fileBDesc),
        "Failed to get size for file '{}'",
        fileB
    );

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
        addr_size readA = core::Unpack(
            core::fileRead(fileADesc, bufferA, chunk),
            "Failed to read file '{}'",
            fileA
        );
        addr_size readB = core::Unpack(
            core::fileRead(fileBDesc, bufferB, chunk),
            "Failed to read file '{}'",
            fileB
        );

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
