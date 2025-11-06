#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QListWidgetItem>
#include "worker.h"
#include "fileprocessorconfig.h"
#include "processingstatistics.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_buttonStartStop_clicked();
    void on_buttonBrowseInput_clicked();
    void on_buttonBrowseOutput_clicked();
    void on_WorkMode_currentTextChanged(const QString &text);

    void onWorkerProgressChanged(int percent);
    void onWorkerStatusChanged(const QString &status);
    void onWorkerFinished();
    void onWorkerErrorOccurred(const QString &errorMessage);
    void onProcessingTimerTimeout();

private:
    Ui::MainWindow *ui;
    
    QTimer *m_processingTimer;
    Worker *m_worker;
    QThread *m_workerThread;
    
    bool m_isProcessing;
    bool m_isWorkerBusy;
    QString m_currentInputFile;
    QStringList m_processedFiles;
    QStringList m_fileQueue;
    
    ProcessingStatistics m_statistics;

    void setupUI();
    void setupConnections();
    void setupValidator();
    
    FileProcessorConfig getConfigFromUI() const;
    bool validateConfiguration();
    
    void startProcessing();
    void stopProcessing();
    void toggleUI(bool processing);
    
    void scanForFiles();
    void processNextFile();
    void processSingleFile(const QString& filePath);
    bool shouldProcessFile(const QString& filePath);
    
    void logMessage(const QString& message);
    void logFileProcessingStart(const QFileInfo& fileInfo, const QString& outputFileName);
    void logFileProcessingSuccess(const QFileInfo& fileInfo);
    void logFileProcessingError(const QString& errorMessage);
    void logStatistics();
    void updateFileList(const QString& inputFile, const QString& outputFile);
    
    void adjustUIForResolution();
    void applyDPIScaling();
    qreal getCurrentDPIScale() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H
