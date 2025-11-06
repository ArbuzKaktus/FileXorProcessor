#include "fileprocessorconfig.h"
#include <QDir>
#include <QFileInfo>

bool FileProcessorConfig::isValid(QString* errorMessage) const
{
    if (m_xorKey.length() != 8) {
        if (errorMessage) {
            *errorMessage = "XOR ключ должен содержать ровно 16 hex-символов (8 байт)";
        }
        return false;
    }

    if (m_inputPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Укажите входную директорию";
        }
        return false;
    }

    QDir inputDir(m_inputPath);
    if (!inputDir.exists()) {
        if (errorMessage) {
            *errorMessage = "Входная директория не существует";
        }
        return false;
    }

    if (m_outputPath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Укажите выходную директорию";
        }
        return false;
    }

    if (m_fileMasks.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Укажите маску файлов";
        }
        return false;
    }

    return true;
}
