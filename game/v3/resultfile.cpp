/**
  *  \file game/v3/resultfile.cpp
  *  \brief Class game::v3::ResultFile
  */

#include <cstring>
#include "game/v3/resultfile.hpp"
#include "afl/except/fileformatexception.hpp"
#include "afl/string/format.hpp"
#include "game/v3/structures.hpp"

// Constructor.
game::v3::ResultFile::ResultFile(afl::io::Stream& file, afl::string::Translator& tx)
    : m_file(file)
{
    loadHeader(tx);
}

// Destructor.
game::v3::ResultFile::~ResultFile()
{ }

// Get result file version.
int
game::v3::ResultFile::getVersion() const
{
    return m_version;
}

// Get offset of a RST section.
bool
game::v3::ResultFile::getSectionOffset(Section section, afl::io::Stream::FileSize_t& offset) const
{
    // ex GResultFile::getSectionOffset
    if (hasSection(section)) {
        offset = m_offset[section];
        return true;
    } else {
        return false;
    }
}

// Check whether section is present.
bool
game::v3::ResultFile::hasSection(Section section) const
{
    // ex GResultFile::haveSection
    return m_offset[section] > 0;
}


// Get underlying file.
afl::io::Stream&
game::v3::ResultFile::getFile() const
{
    return m_file;
}

/** Load and validate header.
    This also figures out the version number. */
void
game::v3::ResultFile::loadHeader(afl::string::Translator& tx)
{
    // ex GResultFile::checkHeader
    // Initialize everything to default
    m_version = -1;
    for (size_t i = 0; i < NUM_SECTIONS; ++i) {
        m_offset[i] = 0;
    }

    // Load header
    m_file.setPos(0);
    structures::ResultHeader header;
    m_file.fullRead(afl::base::fromObject(header));

    // RST must be seekable
    afl::io::Stream::FileSize_t size = m_file.getSize();
    if (size == 0) {
        throw afl::except::FileFormatException(m_file, tx.translateString("Result file is not a regular file"));
    }

    // Copy first 8 sections
    for (size_t i = 0; i < 8; ++i) {
        setSectionAddress(Section(i), header.address[i], size, tx);
    }

    // Size of Winplan part
    static const size_t WINSIZE = 500*8 + 50*12 + 50*4 + 682 + 7800;
    
    if (std::memcmp(header.signature, "VER3.5", 6) == 0
        && header.signature[6] >= '0' && header.signature[6] <= '9'
        && header.signature[7] >= '0' && header.signature[7] <= '9')
    {
        // Might be Winplan RST.
        // Host occasionally sends out RSTs bearing the 3.5 header, which are not actually Winplan-style.
        int32_t koreOffset = header.addressWindows - 1;
        if (koreOffset >= 0 && afl::io::Stream::FileSize_t(koreOffset) <= size - WINSIZE) {
            uint8_t buf[4];
            m_file.setPos(koreOffset + WINSIZE);
            if (m_file.read(buf) == 4 && (std::memcmp(buf, "1211", 4) == 0 || std::memcmp(buf, "1120", 4) == 0)) {
                // It is a Winplan file.
                m_version = 10*(header.signature[6]-'0') + header.signature[7]-'0';
            }
        }

        // Copy pointers
        if (m_version >= 0) {
            setSectionAddress(KoreSection, header.addressWindows, size, tx);
            if (header.addressLeech > 0) {
                setSectionAddress(LeechSection, header.addressLeech, size, tx);
            }
        }
        if (m_version >= 1) {
            setSectionAddress(SkoreSection, header.addressSkore, size, tx);
        }
    }
}

/** Set section address.
    Validates the address, checking for obvious mistakes, and then stores them in our data structure.
    \param section section
    \param addressFromFile address as read from file (1-based)
    \param fileSize file size
    \param tx translator for error messages */
void
game::v3::ResultFile::setSectionAddress(Section section, int32_t addressFromFile, afl::io::Stream::FileSize_t fileSize, afl::string::Translator& tx)
{
    if (addressFromFile < 32 || afl::io::Stream::FileSize_t(addressFromFile) >= fileSize) {
        throw afl::except::FileFormatException(m_file, afl::string::Format(tx.translateString("Section %d has an invalid address").c_str(), int(section)));
    }
    m_offset[section] = uint32_t(addressFromFile - 1);
}
