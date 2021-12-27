#include "patch.h"

#include <QCryptographicHash>
#include <QTextStream>
#include <memory>

bool Patch::addBytesFrom1337(QString strInput, int64_t offsetBase)
{
    // parse .1337 file format (patch file format from x64dbg)
    QTextStream in(&strInput);
    QString lineRaw, line;
    size_t nBytesBefore = m_bytes.size();

    bool ok = true;
    while(in.readLineInto(&lineRaw))
    {
        line = lineRaw.trimmed();
        if(line.isEmpty() || line[0] == ">" || line[0] == '#')
            continue;


        // parse line
        int idxSeparator1 = line.indexOf(':');
        int idxSeparator2 = line.indexOf('-', idxSeparator1);
        int idxSeparator3 = idxSeparator2 + 1;

        ok = ok && idxSeparator1 >= 0;
        ok = ok && idxSeparator2 >= 0;
        ok = ok && idxSeparator3+2 < line.size();
        ok = ok && line[idxSeparator3] == '>';
        for(int i=0; i<line.size() && ok; ++i)
        {
            if(i != idxSeparator1 && i != idxSeparator2 && i != idxSeparator3)
            {
                QChar c = line[i];

                if(!c.isNumber() && (c < QChar('A') || c > QChar('F')))
                    ok = false;
            }
        }
        if(!ok)
            break;

        // convert substrings to numbers
        bool okOffset = true, okOriginal = true, okPatched = true;
        uint64_t offset = line.left(idxSeparator1).toULongLong(&okOffset, 16);
        uint original = line.mid(idxSeparator1+1, idxSeparator2-(idxSeparator1+1)).toUInt(&okOriginal, 16);
        uint patched = line.mid(idxSeparator3+1, line.length()-(idxSeparator3+1)).toUInt(&okPatched, 16);
        offset -= offsetBase;
        if(original > 0xFF) okOriginal = false;
        if(patched > 0xFF) okPatched = false;
        ok = ok && okOffset && okOriginal && okPatched;
        ok = ok && offset < 1ULL<<60; // sanity check

        addByte(offset, uint8_t(original), uint8_t(patched));
    }

    if(!ok)
    {
        // revert all changes
        m_bytes.resize(nBytesBefore);
    }
    return ok;
}

uint64_t Patch::getLargestPatchedOffset()
{
    uint64_t result = 0;
    for(const PatchByte& b : m_bytes)
    {
        result = std::max(result, b.offset);
    }
    return result;
}

Patch::Error Patch::applyPatch(QDir dirBase, Patch::PatchFlags flags)
{
    bool force = flags & PATCH_FORCE;
    bool dryRun = flags & PATCH_DRY_RUN;
    bool verifyChecksum = flags & PATCH_VERIFY_CHECKSUM;

    // get file to patch
    QString path = dirBase.absoluteFilePath(m_pathRelative);
    QFile file(path);
    if(!file.exists())
        return ERROR_FILE_NOT_FOUND;

    // check file size
    uint64_t largestOffset = getLargestPatchedOffset();
    int64_t filesize = file.size();
    if(filesize > m_sizeMax || uint64_t(filesize) < largestOffset+1)
        return ERROR_PATCH_INCOMPATIBLE_HARD;

    // open and read file
    bool ok = file.open(QIODevice::ReadWrite);
    if(!ok)
        return ERROR_FILE_READ_FAILED;
    QByteArray data = file.readAll();
    ok = file.seek(0);
    if(!ok || int64_t(data.size()) != filesize)
        return ERROR_UNKNOWN;

    // verify checksum
    bool checksumMatches = true;
    if(verifyChecksum)
    {
        QByteArray checksum = QCryptographicHash::hash(data, QCryptographicHash::Md5);
        if(size_t(checksum.size()) != m_checksum.size())
            checksumMatches = false;
        else if(memcmp(checksum.data(), m_checksum.data(), m_checksum.size()) != 0)
            checksumMatches = false;
        else
            checksumMatches = true;
    }

    // patch file data in memory
    bool isAllOriginal = true;
    bool isAllPatched = true;

    for(const PatchByte& b : m_bytes)
    {
        if(b.offset >= (1ULL<<31) || int(b.offset) >= data.size())
            return ERROR_UNKNOWN; // QByteArray uses int32_t for size

        uint8_t byteFromFile = data[int(b.offset)];
        isAllOriginal = isAllOriginal && byteFromFile == b.original;
        isAllPatched = isAllPatched && byteFromFile == b.patched;

        // If we don't force patch, one of these two conditions has to be true:
        // * All bytes match the original bytes => we can apply the patch
        // * All bytes match the patched bytes => patch is already applied, nothing to do
        if(!force && !(isAllOriginal || isAllPatched))
            return ERROR_PATCH_INCOMPATIBLE;

        data[int(b.offset)] = char(b.patched);
    }

    // nothing to be done => no need to write the file
    if(isAllPatched)
        return ERROR_PATCH_ALREADY_INSTALLED;

    // write back patched file data (skip if this is a dry run)
    if(!dryRun)
    {
        int64_t bytesWritten = file.write(data);
        if(bytesWritten != data.size())
            return ERROR_FILE_WRITE_FAILED;
    }

    return (verifyChecksum && !checksumMatches) ? ERROR_SUCCESS_MODIFIED : ERROR_SUCCESS;
}

QString Patch::errorString(Patch::Error error)
{
    switch(error)
    {
    case ERROR_SUCCESS:
        return "Success.";
    case ERROR_SUCCESS_MODIFIED:
        return "Success. (Patch applied to modified game file)";
    case ERROR_PATCH_ALREADY_INSTALLED:
        return "Patch is already installed.";
    case ERROR_PATCH_INCOMPATIBLE:
        return "Game file is not compatible with this patch. Forcing the patch installation will override this error but is not recommended.";
    case ERROR_PATCH_INCOMPATIBLE_HARD:
        return "Game file is not compatible with this patch.";
    case ERROR_FILE_NOT_FOUND:
        return "Game file not found. Please check whether you have selected the correct installation directory.";
    case ERROR_FILE_READ_FAILED:
        return "Failed to read game file.";
    case ERROR_FILE_WRITE_FAILED:
        return "Failed to write game file.";
    case ERROR_UNKNOWN:
    default:
        return "Unknown error.";
    }
}

bool Patch::backupExists(QDir dirBase)
{
    QString pathDst = getBackupFilePath(dirBase);
    QFile fileDst(pathDst);
    return fileDst.exists();
}

bool Patch::backupCreate(QDir dirBase)
{
    // get file to back up
    QString path = dirBase.absoluteFilePath(m_pathRelative);
    QFile file(path);
    if(!file.exists())
        return false;

    // get destination file
    QString pathDst = getBackupFilePath(dirBase);
    QFile fileDst(pathDst);
    if(fileDst.exists())
        return false; // we're not going to overwrite an existing backup

    // create the backup file
    bool success = file.copy(pathDst);
    return success;
}

bool Patch::backupRestore(QDir dirBase)
{
    // get backup file
    QString path = getBackupFilePath(dirBase);
    QFile file(path);
    if(!file.exists())
        return false;

    // get destination file
    QString pathDst = dirBase.absoluteFilePath(m_pathRelative);
    QFile fileDst(pathDst);
    if(fileDst.exists())
        fileDst.remove(); // delete game file if it exists

    // restore the backup file
    bool success = file.copy(pathDst);
    return success;
}

QString Patch::getBackupFilePath(QDir dirBase)
{
    QString path = dirBase.absoluteFilePath(m_pathRelative);
    int idxFileEnding = path.lastIndexOf('.');
    if(idxFileEnding < 0) // no file ending
        idxFileEnding = path.size();
    path.insert(idxFileEnding, "_backup-StokDokulus");
    return path;
}
