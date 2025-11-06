#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileutils.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QDateTime>
#include <QDebug>
#include <QResizeEvent>
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_isProcessing(false)
    , m_isWorkerBusy(false)
{
    ui->setupUi(this);
    
    setupUI();
    setupConnections();
    setupValidator();
    
    adjustUIForResolution();
    
    logMessage("Programm initialized");
    logMessage("Programm is ready to start");
}

MainWindow::~MainWindow()
{
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }

    delete ui;
}

void MainWindow::setupUI()
{
    ui->EditInputPath->setText(QDir::currentPath());
    ui->EditOutputPath->setText(QDir::currentPath() + "/output");
    ui->EditFileMask->setText("*.txt");
    ui->Interval->setValue(5000);
    ui->Interval->setEnabled(false);

    QDir outputDir(ui->EditOutputPath->text());
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }
}

void MainWindow::setupConnections()
{
    m_processingTimer = new QTimer(this);
    connect(m_processingTimer, &QTimer::timeout, this, &MainWindow::onProcessingTimerTimeout);

    m_workerThread = new QThread(this);
    m_worker = new Worker();
    m_worker->moveToThread(m_workerThread);

    connect(m_worker, &Worker::errorOccurred, this, &MainWindow::onWorkerErrorOccurred);
    connect(m_worker, &Worker::progressChanged, this, &MainWindow::onWorkerProgressChanged);
    connect(m_worker, &Worker::finished, this, &MainWindow::onWorkerFinished);
    connect(m_worker, &Worker::statusChanged, this, &MainWindow::onWorkerStatusChanged);

    m_workerThread->start();
}

void MainWindow::setupValidator()
{
    QRegularExpressionValidator *hexValidator = 
        new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{16}"), this);
    ui->EditXOR->setValidator(hexValidator);
    
    connect(ui->EditXOR, &QLineEdit::textChanged, [this](const QString& text) {
        if (text.length() == 16) {
            ui->EditXOR->setStyleSheet("QLineEdit { background-color: #09820F; }");
        } else {
            ui->EditXOR->setStyleSheet("QLineEdit { background-color: #820909; }");
        }
    });
}

void MainWindow::on_buttonStartStop_clicked() {
    if (!m_isProcessing) {
        startProcessing();
    } else {
        stopProcessing();
    }
}

void MainWindow::on_buttonBrowseInput_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Выберите входную директорию",
        ui->EditInputPath->text()
    );

    if (!dir.isEmpty()) {
        ui->EditInputPath->setText(dir);
        logMessage("input directory is set: " + dir);
    }
}

void MainWindow::on_buttonBrowseOutput_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Выберите выходную директорию",
        ui->EditOutputPath->text()
    );

    if (!dir.isEmpty()) {
        ui->EditOutputPath->setText(dir);
        logMessage("output directory is set: " + dir);
    }
}

void MainWindow::on_WorkMode_currentTextChanged(const QString &text) {
    bool isTimerMode = (text == "Работа по таймеру");
    ui->Interval->setEnabled(isTimerMode);

    if (isTimerMode) {
        logMessage("turn to timer work mode");
    } else {
        logMessage("turn to one time work mode");
    }
}

FileProcessorConfig MainWindow::getConfigFromUI() const
{
    FileProcessorConfig config;
    config.setInputPath(ui->EditInputPath->text());
    config.setOutputPath(ui->EditOutputPath->text());
    config.setFileMasks(ui->EditFileMask->text().split(',', Qt::SkipEmptyParts));
    config.setXorKey(QByteArray::fromHex(ui->EditXOR->text().toUtf8()));
    config.setDeleteInputFiles(ui->checkBoxDeleteInput->isChecked());
    config.setTimerMode(ui->WorkMode->currentText() == "Работа по таймеру");
    config.setTimerInterval(ui->Interval->value());
    config.setAddCounterOnConflict(ui->ActionOnConflict->currentText() == "Добавить Счётчик");
    return config;
}

bool MainWindow::validateConfiguration()
{
    FileProcessorConfig config = getConfigFromUI();
    QString errorMessage;
    
    if (!config.isValid(&errorMessage)) {
        QMessageBox::warning(this, "Ошибка", errorMessage);
        return false;
    }
    
    QDir outputDir(config.outputPath());
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            QMessageBox::warning(this, "Ошибка",
                               "Не удалось создать выходную директорию: " + config.outputPath());
            return false;
        }
    }
    
    return true;
}

void MainWindow::startProcessing() {
    if (!validateConfiguration()) {
        return;
    }

    m_isProcessing = true;
    toggleUI(true);

    ui->progressBar->setValue(0);
    m_processedFiles.clear();
    m_fileQueue.clear();
    m_statistics.reset();

    ui->editLogs->clear();
    ui->filesWidget->clear();

    FileProcessorConfig config = getConfigFromUI();
    
    logMessage("=== START ===");
    logMessage("input path: " + config.inputPath());
    logMessage("output path: " + config.outputPath());
    logMessage("file mask: " + ui->EditFileMask->text());
    logMessage("XOR key: " + ui->EditXOR->text());

    if (config.isTimerMode()) {
        int interval = config.timerInterval();
        m_processingTimer->start(interval);
        logMessage("Interval Mode: timer = " + QString::number(interval) + " ms");
    } else {
        logMessage("One time Mode");
        scanForFiles();
    }
}

void MainWindow::stopProcessing() {
    m_isProcessing = false;
    toggleUI(false);
    m_processingTimer->stop();

    logStatistics();
}

void MainWindow::onProcessingTimerTimeout() {
    scanForFiles();
}

void MainWindow::scanForFiles() {
    if (!m_isProcessing) return;

    FileProcessorConfig config = getConfigFromUI();
    QDir directory(config.inputPath());

    if (!directory.exists()) {
        logMessage("error: input directory doesnt exist");
        stopProcessing();
        return;
    }

    if (config.isTimerMode()) {
        m_processedFiles.clear();
    }

    QStringList filesToProcess;
    
    for (const QString& mask : config.fileMasks()) {
        QString cleanMask = mask.trimmed();
        QStringList files = directory.entryList(QStringList() << cleanMask, QDir::Files | QDir::NoDotAndDotDot);
        for (const QString& file : files) {
            QString filePath = directory.absoluteFilePath(file);
            if (shouldProcessFile(filePath)) {
                filesToProcess.append(filePath);
            }
        }
    }

    if (!filesToProcess.isEmpty()) {
        m_fileQueue.append(filesToProcess);
        logMessage("Found " + QString::number(filesToProcess.size()) + " file(s) to process");

        if (!m_isWorkerBusy) {
            processNextFile();
        }
    } else if (!config.isTimerMode()) {
        logMessage("Files with current masks not found");
        if (m_processedFiles.isEmpty()) {
            QMessageBox::information(this, "Информация", "Файлы по заданной маске не найдены");
        }
        stopProcessing();
    }
}

void MainWindow::toggleUI(bool processing) {
    ui->buttonStartStop->setText(processing ? "Стоп" : "Старт");

    ui->EditInputPath->setEnabled(!processing);
    ui->buttonBrowseInput->setEnabled(!processing);
    ui->EditOutputPath->setEnabled(!processing);
    ui->buttonBrowseOutput->setEnabled(!processing);
    ui->EditFileMask->setEnabled(!processing);
    ui->EditXOR->setEnabled(!processing);
    ui->WorkMode->setEnabled(!processing);
    ui->ActionOnConflict->setEnabled(!processing);
    ui->checkBoxDeleteInput->setEnabled(!processing);
    ui->Interval->setEnabled(!processing &&
                                        ui->WorkMode->currentText() == "Работа по таймеру");
}

bool MainWindow::shouldProcessFile(const QString& filePath) {
    if (!m_currentInputFile.isEmpty() && m_currentInputFile == filePath) {
        return false;
    }

    if (m_processedFiles.contains(filePath)) {
        return false;
    }
    
    if (m_fileQueue.contains(filePath)) {
        return false;
    }

    QFileInfo fileInfo(filePath);
    return fileInfo.isReadable();
}

void MainWindow::processNextFile() {
    if (m_fileQueue.isEmpty() || m_isWorkerBusy) {
        return;
    }
    
    QString filePath = m_fileQueue.takeFirst();
    m_currentInputFile = filePath;
    m_isWorkerBusy = true;

    QFileInfo fileInfo(filePath);
    FileProcessorConfig config = getConfigFromUI();
    
    QString outputFileName = fileInfo.fileName();
    QString fullOutputPath = QDir(config.outputPath()).absoluteFilePath(outputFileName);

    if (config.addCounterOnConflict() && QFile::exists(fullOutputPath)) {
        outputFileName = FileUtils::generateUniqueFileName(config.outputPath(), outputFileName);
        fullOutputPath = QDir(config.outputPath()).absoluteFilePath(outputFileName);
    }

    logFileProcessingStart(fileInfo, outputFileName);
    updateFileList(fileInfo.fileName(), outputFileName);

    QMetaObject::invokeMethod(m_worker, "processFile",
                              Qt::QueuedConnection,
                              Q_ARG(QString, filePath),
                              Q_ARG(QString, fullOutputPath),
                              Q_ARG(QByteArray, config.xorKey())
                              );
}

void MainWindow::processSingleFile(const QString &filePath)
{
    m_fileQueue.append(filePath);
    if (!m_isWorkerBusy) {
        processNextFile();
    }
}

void MainWindow::updateFileList(const QString& inputFile, const QString& outputFile) {
    QString itemText = inputFile + " -> " + outputFile;
    ui->filesWidget->addItem(itemText);
    ui->filesWidget->scrollToBottom();
}

void MainWindow::onWorkerProgressChanged(int percent) {
    ui->progressBar->setValue(percent);
}

void MainWindow::onWorkerStatusChanged(const QString& status) {
    ui->labelStatus->setText(status);
}

void MainWindow::onWorkerFinished() {
    m_isWorkerBusy = false;
    
    if (!m_currentInputFile.isEmpty()) {
        m_processedFiles.append(m_currentInputFile);
        
        QFileInfo fileInfo(m_currentInputFile);
        m_statistics.addSuccess(fileInfo.size());
        
        logFileProcessingSuccess(fileInfo);
        
        FileProcessorConfig config = getConfigFromUI();
        if (config.deleteInputFiles()) {
            if (QFile::remove(m_currentInputFile)) {
                logMessage("Входной файл удален: " + m_currentInputFile);
            } else {
                logMessage("Ошибка удаления входного файла: " + m_currentInputFile);
            }
        }
        
        m_currentInputFile.clear();
    }

    if (!m_fileQueue.isEmpty() && m_isProcessing) {
        processNextFile();
    } else if (m_fileQueue.isEmpty()) {
        FileProcessorConfig config = getConfigFromUI();
        if (!config.isTimerMode() && m_isProcessing) {
            stopProcessing();
        }
    }
}

void MainWindow::onWorkerErrorOccurred(const QString& errorMessage) {
    m_statistics.addError();
    logFileProcessingError(errorMessage);
    
    QMessageBox::warning(this, "Ошибка", errorMessage);
    
    m_isWorkerBusy = false;
    m_currentInputFile.clear();

    if (!m_fileQueue.isEmpty() && m_isProcessing) {
        processNextFile();
    }
}

void MainWindow::logMessage(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->editLogs->append("[" + timestamp + "] " + message);
    ui->editLogs->ensureCursorVisible();
}

void MainWindow::logFileProcessingStart(const QFileInfo& fileInfo, const QString& outputFileName) {
    QString sizeStr = FileUtils::formatFileSize(fileInfo);
    logMessage(">>> Начата обработка: " + fileInfo.fileName() + " (" + sizeStr + ") -> " + outputFileName);
}

void MainWindow::logFileProcessingSuccess(const QFileInfo& fileInfo) {
    QString sizeStr = FileUtils::formatFileSize(fileInfo);
    logMessage("<<< Успешно обработан: " + fileInfo.fileName() + " (" + sizeStr + ")");
}

void MainWindow::logFileProcessingError(const QString& errorMessage) {
    logMessage("!!! Ошибка обработки: " + errorMessage);
    
    if (!m_currentInputFile.isEmpty()) {
        logMessage("!!! Файл с ошибкой: " + QFileInfo(m_currentInputFile).fileName());
    }
}

void MainWindow::logStatistics() {
    logMessage("=== STOP ===");
    logMessage("processed files count: " + QString::number(m_processedFiles.size()));
    logMessage("Успешно обработано: " + QString::number(m_statistics.successCount()) + " файлов");
    logMessage("Ошибок обработки: " + QString::number(m_statistics.errorCount()) + " файлов");
    logMessage("Всего обработано данных: " + m_statistics.getFormattedSize());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    adjustUIForResolution();
}

void MainWindow::adjustUIForResolution()
{
    applyDPIScaling();

    QSize windowSize = this->size();
    
    int minButtonWidth = qMax(80, windowSize.width() / 15);
    
    if (ui->buttonStartStop) {
        ui->buttonStartStop->setMinimumWidth(minButtonWidth);
    }
    if (ui->buttonBrowseInput) {
        ui->buttonBrowseInput->setMinimumWidth(minButtonWidth);
    }
    if (ui->buttonBrowseOutput) {
        ui->buttonBrowseOutput->setMinimumWidth(minButtonWidth);
    }
}

void MainWindow::applyDPIScaling()
{
    qreal dpiScale = getCurrentDPIScale();

    if (qAbs(dpiScale - 1.0) < 0.1) {
        return;
    }
    
    QFont font = this->font();
    int baseFontSize = 9;
    font.setPointSize(qRound(baseFontSize * dpiScale));

    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        widget->setFont(font);
    }
}

qreal MainWindow::getCurrentDPIScale() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return 1.0;
    }

    qreal dpi = screen->logicalDotsPerInch();
    
    const qreal baseDPI = 96.0;
    
    return dpi / baseDPI;
}


