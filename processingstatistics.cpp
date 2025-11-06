#include "processingstatistics.h"

void ProcessingStatistics::reset()
{
    m_successCount = 0;
    m_errorCount = 0;
    m_totalBytesProcessed = 0;
}

void ProcessingStatistics::addSuccess(qint64 fileSize)
{
    m_successCount++;
    m_totalBytesProcessed += fileSize;
}

void ProcessingStatistics::addError()
{
    m_errorCount++;
}

QString ProcessingStatistics::formatBytes(qint64 bytes)
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

QString ProcessingStatistics::getFormattedSize() const
{
    return formatBytes(m_totalBytesProcessed);
}

QString ProcessingStatistics::getSummary() const
{
    QString summary;
    summary += "Успешно обработано: " + QString::number(m_successCount) + " файлов\n";
    summary += "Ошибок обработки: " + QString::number(m_errorCount) + " файлов\n";
    summary += "Всего обработано данных: " + getFormattedSize();
    return summary;
}
