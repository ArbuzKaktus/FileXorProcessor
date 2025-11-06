#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QFileInfo>

class FileUtils
{
public:
    static QString generateUniqueFileName(const QString& basePath, const QString& fileName);

    static QString formatFileSize(qint64 bytes);

    static QString formatFileSize(const QFileInfo& fileInfo);
};

#endif // FILEUTILS_H
