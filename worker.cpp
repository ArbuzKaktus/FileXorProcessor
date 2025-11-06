#include "worker.h"

#include <QFile>
#include <QDataStream>
#include <QDebug>

Worker::Worker(QObject *parent)
    : QObject{parent}
{}

void Worker::processFile(const QString& inputFilePath,
                 const QString& outputFilePath,
                 const QByteArray& xorKey) {
    m_abortRequested = false;

    if (xorKey.isEmpty()) {
        emit errorOccurred("XOR ключ не может быть пустым!");
        emit finished();
        return;
    }

    QFile inputFile(inputFilePath);
    QFile outputFile(outputFilePath);

    if (!inputFile.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Не удалось открыть входной файл: " + inputFilePath);
        emit finished();
        return;
    }

    if (!outputFile.open(QIODevice::WriteOnly)) {
        emit errorOccurred("Не удалось создать выходной файл: " + outputFilePath);
        inputFile.close();
        emit finished();
        return;
    }

    const qint64 fileSize = inputFile.size();
    const int keyLength = xorKey.length();
    qint64 totalBytesRead = 0;
    const qint64 bufferSize = 64 * 1024; // 64Kb

    emit statusChanged("Начата обработка файла: " + inputFilePath);
    emit progressChanged(0);

    QDataStream in(&inputFile);
    QDataStream out(&outputFile);

    char *buffer = new char[bufferSize];
    bool isErrorOccurred = false;

    while(!in.atEnd() && !m_abortRequested && !isErrorOccurred) {
        qint64 bytesRead = in.readRawData(buffer, bufferSize);

        if (bytesRead == -1) {
            emit errorOccurred("Ошибка чтения из файла: " + inputFilePath);
            isErrorOccurred = true;
            break;
        }

        for (qint64 i = 0; i != bytesRead; ++i) {
            buffer[i] = buffer[i] ^ xorKey[i % keyLength];
        }

        qint64 bytesWritten = out.writeRawData(buffer, bytesRead);

        if (bytesWritten != bytesRead) {
            emit errorOccurred("Ошибка записи в файл: " + outputFilePath);
            isErrorOccurred = true;
            break;
        }

        totalBytesRead += bytesRead;
        int progress = 0;

        if (fileSize > 0) {
            progress = static_cast<int>((totalBytesRead * 100) / fileSize);
        }

        emit progressChanged(progress);
    }

    delete [] buffer;
    inputFile.close();
    outputFile.close();

    if (m_abortRequested) {
        outputFile.remove();
        emit statusChanged("Обработка прервана: " + inputFilePath);
    } else if (isErrorOccurred) {
        outputFile.remove();
    } else {
        emit statusChanged("Файл успешно обработан: " + outputFile.fileName());
        emit progressChanged(100);
    }

    emit finished();

}
