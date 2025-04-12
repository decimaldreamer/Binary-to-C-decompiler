#include "mainwindow.h"
#include "opsoup.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Binary-to-C Decompiler");
    resize(800, 600);

    createMenu();
    createToolBar();
    createStatusBar();
    createCentralWidget();
}

MainWindow::~MainWindow() {
}

void MainWindow::createMenu() {
    QMenu *fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Open", this, &MainWindow::openFile);
    fileMenu->addAction("Save", this, &MainWindow::saveOutput);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", this, &MainWindow::showAbout);
}

void MainWindow::createToolBar() {
    QToolBar *toolBar = addToolBar("Main");
    toolBar->addAction("Open", this, &MainWindow::openFile);
    toolBar->addAction("Decompile", this, &MainWindow::decompile);
    toolBar->addAction("Save", this, &MainWindow::saveOutput);
}

void MainWindow::createStatusBar() {
    statusBar()->addWidget(new QLabel("Ready"));
}

void MainWindow::createCentralWidget() {
    QWidget *central = new QWidget;
    setCentralWidget(central);

    QVBoxLayout *layout = new QVBoxLayout(central);

    outputText = new QTextEdit;
    outputText->setReadOnly(true);
    layout->addWidget(outputText);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    openButton = new QPushButton("Open");
    decompileButton = new QPushButton("Decompile");
    saveButton = new QPushButton("Save");
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(decompileButton);
    buttonLayout->addWidget(saveButton);
    layout->addLayout(buttonLayout);

    progressBar = new QProgressBar;
    layout->addWidget(progressBar);

    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(decompileButton, &QPushButton::clicked, this, &MainWindow::decompile);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveOutput);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Binary File", QString(),
        "Binary Files (*.exe *.dll *.so *.dylib);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        currentFile = fileName;
        statusBar()->showMessage("File opened: " + fileName);
    }
}

void MainWindow::decompile() {
    if (currentFile.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file selected");
        return;
    }

    // TODO: Implement decompilation
    outputText->setText("Decompilation in progress...");
    progressBar->setValue(0);
}

void MainWindow::saveOutput() {
    if (outputText->toPlainText().isEmpty()) {
        QMessageBox::warning(this, "Error", "No output to save");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Output", QString(),
        "C Files (*.c);;Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << outputText->toPlainText();
            statusBar()->showMessage("File saved: " + fileName);
        }
    }
}

void MainWindow::updateProgress(int value) {
    progressBar->setValue(value);
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About",
        "Binary-to-C Decompiler\n\n"
        "A tool for converting binary files to C code.\n"
        "Version 1.0.0\n\n"
        "Copyright (C) uinptr of course 2025");
} 