#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void decompile();
    void saveOutput();
    void updateProgress(int value);
    void showAbout();

private:
    void createMenu();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();

    QTextEdit *outputText;
    QProgressBar *progressBar;
    QPushButton *openButton;
    QPushButton *decompileButton;
    QPushButton *saveButton;
    QLabel *statusLabel;
    QString currentFile;
}; 