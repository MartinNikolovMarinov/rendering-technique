#include "core_init.h"

struct Surface;

namespace TGA
{

using TGALong = u32;
using TGAShort = u16;
using TGAByte = u8;

enum struct TGAError {
    Undefined,
    FailedToStatFile,
    FailedToReadFile,
    FailedToWriteFile,
    InvalidFileFormat,
    OldFormat,
    ApplicationBug,
    InvalidArgument,
    UnsupportedImageType,
    SENTINEL
};

PACK_PUSH
struct PACKED Header {
    // This field identifies the number of bytes contained in Field 6, the Image ID Field. The maximum number of
    // characters is 255. A value of zero indicates that no Image ID field is included with the image.
    TGAByte idLength;

    // This field indicates the type of color map (if any) included with the image. There are currently 2 defined values
    // for this field.
    //  * 0 - color map is NOT included.
    //  * 1 - color map IS included.
    //
    // True-Color images do not normally make use of the color map field, but some current applications store palette
    // information or developer-defined information in this field. It is best to check Field 3, Image Type, to make sure
    // you have a file which can use the data stored in the Color Map Field. Otherwise ignore the information.
    //
    // IMPORTANT: When saving or creating files for True-Color images do not use this field and set it to Zero to ensure
    // compatibility. Please refer to the Developer Area specification for methods of storing developer defined
    // information.
    TGAByte colorMapType;

    // The TGA File Format can be used to store Pseudo-Color, True-Color and Direct-Color images of various pixel
    // depths. Truevision has currently defined seven image types:
    //   * 0 - No Image Data Included
    //   * 1 - Uncompressed, Color-mapped Image
    //   * 2 - Uncompressed, True-color Image
    //   * 3 - Uncompressed, Black-and-white Image
    //   * 9 - Run-length encoded, Color-mapped Image
    //   * 10 - Run-length encoded, True-color Image
    //   * 11 - Run-length encoded, Black-and-white Image
    // Image Data Type codes 0 to 127 are reserved for use by Truevision for general applications. Image Data Type codes
    // 128 to 255 may be used for developer applications.
    // For a complete description of these image-data types, see the IMAGE TYPES section.
    TGAByte imageType;

    // This field and its sub-fields describe the color map (if any) used for the image. If the Color Map Type field is
    // set to zero, indicating that no color map exists, then these 5 bytes should be set to zero.
    // IMPORTANT: These bytes always must be written to the file.
    //
    //   * Field 0 (2 bytes) - First Entry Index:
    //      Index of the first color map entry. Index refers to the starting entry in loading the color map.
    //
    //      Example: If you would have 1024 entries in the entire color map but you only need to store 72 of those
    //      entries, this field allows you to start in the middle of the color-map (e.g., position 342).
    //
    //   * Field 1 (2 bytes) - Color Map Length:
    //      Total number of color map entries included.
    //
    //   * Field 2 (1 byte) - Color Map Entry Size:
    //      Establishes the number of bits per entry. Typically 15, 16, 24 or 32-bit values are used.
    TGAByte colorMapSpecification[5];

    // This field and its sub-fields describe the image screen location, size and pixel depth.
    // IMPORTANT: These bytes are always written to the file.
    //
    //  * Field 0 (2 bytes) - X-origin of Image:
    //      These bytes specify the absolute horizontal coordinate for the lower left corner of the image as it is
    //      positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series).
    //
    //  * Field 1 (2 bytes) - Y-origin of Image:
    //      These bytes specify the absolute vertical coordinate for the lower left corner of the image as it is
    //      positioned on a display device having an origin at the lower left of the screen (e.g., the TARGA series).
    //
    //  * Field 2 (2 bytes) - Image Width:
    //      This field specifies the width of the image in pixels.
    //
    //  * Field 3 (2 bytes) - Image Height:
    //      This field specifies the height of the image in pixels.
    //
    //  * Field 4 (1 byte) - Pixel Depth:
    //      This field indicates the number of bits per pixel. This number includes the Attribute or Alpha channel bits.
    //      Common values are 8, 16, 24 and 32 but other pixel depths could be used.
    //
    //  * Field 5 (1 byte) - Image Descriptor:
    //      [7..6]  | [5..4]         | [3, 2, 1, 0]
    //      ^unused | ^ image origin | ^alpha channel bits
    //
    //      * Bits 3-0: These bits specify the number of attribute bits per pixel. In the case of the TrueVista, these
    //        bits indicate the number of bits per pixel which are designated as Alpha Channel bits. For the ICB and
    //        TARGA products, these bits indicate the number of overlay bits available per pixel. See field 24
    //        (Attributes Type in the Extensions Area) for more information.
    //
    //      * Bits 5-4: These bits are used to indicate the order in which pixel data is transferred from the file to
    //        the screen. Bit 4 is for left-to-right ordering and bit 5 is for top-to-bottom ordering as shown below:
    //
    //        | Screen destination Image Origin of first pixel | Image bit 5 | Image bit 4 |
    //        | bottom left                                    | 0           | 0           |
    //        | bottom right                                   | 0           | 1           |
    //        | top left                                       | 1           | 0           |
    //        | top right                                      | 1           | 1           |
    //
    //      * Bits 7 & 6: Must be zero to insure future compatibility.
    //
    TGAByte imageSpecification[10];

    constexpr inline i32 colorMapFirstEntryIdx() const {
        return i32(colorMapSpecification[0]) | (i32(colorMapSpecification[1]) << core::BYTE_SIZE);
    }
    constexpr inline i32 colorMapLength() const {
        return i32(colorMapSpecification[2]) | (i32(colorMapSpecification[3]) << core::BYTE_SIZE);
    }
    constexpr inline i32 colorMapEntrySize() const {
        return i32(colorMapSpecification[4]);
    }

    constexpr inline i32 offsetX() const {
        return i32(imageSpecification[0]) | (i32(imageSpecification[1]) << core::BYTE_SIZE);
    }
    constexpr inline void setOffsetX(u16 x) {
        imageSpecification[0] = TGAByte(x);
        imageSpecification[1] = TGAByte(x >> core::BYTE_SIZE);
    }

    constexpr inline i32 offsetY() const {
        return i32(imageSpecification[2]) | (i32(imageSpecification[3]) << core::BYTE_SIZE);
    }
    constexpr inline void setOffsetY(u16 x) {
        imageSpecification[2] = TGAByte(x);
        imageSpecification[3] = TGAByte(x >> core::BYTE_SIZE);
    }

    constexpr inline i32 width() const {
        return i32(imageSpecification[4]) | (i32(imageSpecification[5]) << core::BYTE_SIZE);
    }
    constexpr inline void setWidth(u16 x) {
        imageSpecification[4] = TGAByte(x);
        imageSpecification[5] = TGAByte(x >> core::BYTE_SIZE);
    }

    constexpr inline i32 height() const {
        return i32(imageSpecification[6]) | (i32(imageSpecification[7]) << core::BYTE_SIZE);
    }
    constexpr inline void setHeight(u16 x) {
        imageSpecification[6] = TGAByte(x);
        imageSpecification[7] = TGAByte(x >> core::BYTE_SIZE);
    }

    constexpr inline i32 pixelDepth() const {
        return i32(imageSpecification[8]);
    }
    constexpr inline void setPixelDepth(u8 x) {
        imageSpecification[8] = TGAByte(x);
    }

    constexpr inline i32 alphaBits() const {
        return i32(0b1111 & imageSpecification[9]);
    }
    constexpr inline void setAlphaBits(u8 x) {
        imageSpecification[9] = TGAByte(0b1111 & x);
    }

    constexpr inline i32 origin() const {
        return i32(0b110000 & imageSpecification[9]) >> 4;
    }
    constexpr inline void setOrigin(u8 x) {
        imageSpecification[9] = TGAByte((0b11 & x) << 4);
    }
};
PACK_POP

PACK_PUSH
struct PACKED Footer {
    TGALong extensionAreaOffset;      // Bytes 0-3: The Extension Area Offset
    TGALong developerDirectoryOffset; // Bytes 4-7: The Developer Directory Offset
    char signature[18];               // Bytes 8-24: The Signature, Byte 25: Binary zero string terminator (0x00)
};
PACK_POP

struct ImageColorMapData {};

struct DeveloperArea {};

struct ExtensionArea {};

enum struct FileType {
    Unknown,
    Original,
    New
};

struct TGAFile {
    core::AllocatorContext* actx;

    core::Memory<u8> memory;

    constexpr static addr_off fileHeaderOff = 0;

    constexpr static addr_off imageColorMapDataAreaOff = sizeof(Header);
    addr_off imageIdOff = -1;
    addr_off colorMapDataOff = -1;
    addr_off imageDataOff = -1;

    addr_off developerAreaOff = -1;
    addr_off extAreaOff = -1;
    addr_off footerOff = -1;

    core::expected<TGAError> header(const Header*& out) const;
    core::expected<TGAError> footer(const Footer*& out) const;

    FileType fileType() const;
    bool isValid() const;

    void free();
};

struct CreateFileFromSurfaceParams {
    const Surface& surface;
    const char* path = nullptr;
    i32 imageType = 1;
    FileType fileType = FileType::Unknown;
};

const char* errorToCstr(TGAError err);

core::expected<TGAFile, TGAError> loadFile(const char* path, core::AllocatorContext& actx = DEF_ALLOC);
core::expected<TGAError> createFileFromSurface(const CreateFileFromSurfaceParams& params);

} // namespace TGA
