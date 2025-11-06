#ifndef WORKER_H
#define WORKER_H

#include <QObject>

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

public slots:
    void processFile(const QString& inputFilePath,
                     const QString& outputFilePath,
                     const QByteArray& xorKey);
signals:
    void progressChanged(int percent);
    void statusChanged(const QString& status);
    void finished();
    void errorOccurred(const QString& errorMessage);

private:
    bool m_abortRequested = false;
};

#endif // WORKER_H
