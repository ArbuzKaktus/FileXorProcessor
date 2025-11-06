#ifndef PROCESSINGSTATISTICS_H
#define PROCESSINGSTATISTICS_H

#include <QString>

class ProcessingStatistics
{
public:
    ProcessingStatistics() = default;

    void reset();
    void addSuccess(qint64 fileSize);
    void addError();

    int successCount() const { return m_successCount; }
    int errorCount() const { return m_errorCount; }
    qint64 totalBytesProcessed() const { return m_totalBytesProcessed; }
    int totalFiles() const { return m_successCount + m_errorCount; }

    QString getFormattedSize() const;
    QString getSummary() const;

private:
    int m_successCount = 0;
    int m_errorCount = 0;
    qint64 m_totalBytesProcessed = 0;

    static QString formatBytes(qint64 bytes);
};

#endif // PROCESSINGSTATISTICS_H
