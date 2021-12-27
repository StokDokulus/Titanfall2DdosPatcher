#ifndef PATCH_H
#define PATCH_H

#include <QDir>
#include <QString>
#include <cstdint>
#include <vector>

struct PatchByte
{
    uint64_t offset;
    uint8_t original;
    uint8_t patched;
    PatchByte() : offset(0), original(0), patched(0) {}
    PatchByte(uint64_t offset, uint8_t original, uint8_t patched)
        : offset(offset), original(original), patched(patched) {}
};


class Patch
{
public:
    enum PatchStatus {
        STATUS_FILE_ORIGINAL = 0,
        STATUS_FILE_MODIFIED,
        STATUS_FILE_INCOMPATIBLE
    };

    enum Error {
        ERROR_SUCCESS = 0,
        ERROR_SUCCESS_MODIFIED,
        ERROR_PATCH_ALREADY_INSTALLED,
        ERROR_PATCH_INCOMPATIBLE,
        ERROR_PATCH_INCOMPATIBLE_HARD,
        ERROR_FILE_NOT_FOUND,
        ERROR_FILE_READ_FAILED,
        ERROR_FILE_WRITE_FAILED,
        ERROR_UNKNOWN
    };

    typedef int PatchFlags;
    static const PatchFlags PATCH_FORCE = 1<<1;
    static const PatchFlags PATCH_DRY_RUN = 1<<2;
    static const PatchFlags PATCH_VERIFY_CHECKSUM = 1<<3;

private:
    QString m_pathRelative;
    std::vector<PatchByte> m_bytes;
    std::vector<uint8_t> m_checksum;
    int64_t m_sizeMax = 1LL<<28; /* sanity check: limit filesize to 256MiB by default */
public:
    Patch() {}
    Patch(QString pathRelative) : m_pathRelative(pathRelative) {}

    QString getPathRelative() { return m_pathRelative; }

    void setPathRelative(QString pathRelative) { m_pathRelative = pathRelative; }
    void clearBytes() { m_bytes.clear(); }
    void addByte(uint64_t offset, uint8_t original, uint8_t patched) { m_bytes.emplace_back(offset, original, patched);}
    bool addBytesFrom1337(QString str1337, int64_t offsetBase);
    void setChecksum(std::vector<uint8_t> checksum) { m_checksum = checksum; }

    uint64_t getLargestPatchedOffset();

    Error applyPatch(QDir dirBase, PatchFlags flags = PATCH_VERIFY_CHECKSUM);
    QString errorString(Error error);

    bool backupExists(QDir dirBase);
    bool backupCreate(QDir dirBase);
    bool backupRestore(QDir dirBase);
    QString getBackupFilePath(QDir dirBase);

};

#endif // PATCH_H
