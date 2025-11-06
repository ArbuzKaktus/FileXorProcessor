#include "fileutils.h"
#include <QDir>

QString FileUtils::generateUniqueFileName(const QString& basePath, const QString& fileName)
{
    QFileInfo fileInfo(fileName);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();

    QDir dir(basePath);
    int counter = 1;
    QString newFileName;

    do {
        if (suffix.isEmpty()) {
            newFileName = QString("%1 (%2)").arg(baseName).arg(counter);
        } else {
            newFileName = QString("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
        }
        counter++;
    } while (dir.exists(newFileName));

    return newFileName;
}

QString FileUtils::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString::number(bytes) + " байт";
    } else if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 2) + " КБ";
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " МБ";
    } else {
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " ГБ";
    }
}

QString FileUtils::formatFileSize(const QFileInfo& fileInfo)
{
    return formatFileSize(fileInfo.size());
}
