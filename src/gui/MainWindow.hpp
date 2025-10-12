#ifdef WITH_GUI

#pragma once

#include <QMainWindow>

// Forward-declare the auto-generated UI class
namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onButtonClicked();

private:
    // A pointer to the class that holds our UI elements
    Ui::MainWindow *ui;
};

#endif // WITH_GUI