#ifndef FILEPROCESSORCONFIG_H
#define FILEPROCESSORCONFIG_H

#include <QString>
#include <QStringList>

class FileProcessorConfig
{
public:
    FileProcessorConfig() = default;

    QString inputPath() const { return m_inputPath; }
    QString outputPath() const { return m_outputPath; }
    QStringList fileMasks() const { return m_fileMasks; }
    QByteArray xorKey() const { return m_xorKey; }
    bool deleteInputFiles() const { return m_deleteInputFiles; }
    bool isTimerMode() const { return m_isTimerMode; }
    int timerInterval() const { return m_timerInterval; }
    bool addCounterOnConflict() const { return m_addCounterOnConflict; }

    void setInputPath(const QString& path) { m_inputPath = path; }
    void setOutputPath(const QString& path) { m_outputPath = path; }
    void setFileMasks(const QStringList& masks) { m_fileMasks = masks; }
    void setXorKey(const QByteArray& key) { m_xorKey = key; }
    void setDeleteInputFiles(bool value) { m_deleteInputFiles = value; }
    void setTimerMode(bool value) { m_isTimerMode = value; }
    void setTimerInterval(int interval) { m_timerInterval = interval; }
    void setAddCounterOnConflict(bool value) { m_addCounterOnConflict = value; }

    bool isValid(QString* errorMessage = nullptr) const;

private:
    QString m_inputPath;
    QString m_outputPath;
    QStringList m_fileMasks;
    QByteArray m_xorKey;
    bool m_deleteInputFiles = false;
    bool m_isTimerMode = false;
    int m_timerInterval = 5000;
    bool m_addCounterOnConflict = false;
};

#endif // FILEPROCESSORCONFIG_H
