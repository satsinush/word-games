#ifdef WITH_GUI

#include "gui/MainWindow.hpp" // Use the relative path from your src directory
#include "ui_MainWindow.h"    // Include the header generated from the .ui file

#include <QPushButton> // You need to include headers for widgets you use in slots

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // This function reads the .ui file and creates all the widgets
    ui->setupUi(this);

    setWindowTitle("Word Games Suite");

    // Connect the button's clicked signal to our slot
    connect(ui->helloButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onButtonClicked()
{
    ui->statusLabel->setText("Hello, Designer World!");
    ui->helloButton->setText("Clicked!");
}

#endif // WITH_GUI