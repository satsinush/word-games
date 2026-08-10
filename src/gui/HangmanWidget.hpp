#ifdef WITH_GUI

#pragma once

#include "gui/GameWidget.hpp"
#include "hangman/hangman.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace Ui {
class HangmanWidget;
}

class HangmanWidget : public GameWidget {
  Q_OBJECT

public:
  explicit HangmanWidget(QWidget *parent = nullptr);
  ~HangmanWidget() override;

public slots:
  void newGame() override;

protected slots:
  void onNewGame() override;
  void onSettings() override;
  void onSolverFinished() override;

protected:
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;

private slots:
  void onSolve();
  void onTableRowClicked(int row, int column);

private:
  Ui::HangmanWidget *ui;
  Hangman::Config config;
  Hangman::Result lastResult;

  // Input widgets
  QLineEdit *patternInput; // Input for word patterns like "_A__ _A_ _____"
  QLineEdit *excludedLettersInput; // Input for letters not in the word

  // Setup the input widgets
  void setupInputs();

  // Rebuild feedback history from inputs
  void rebuildFeedbackFromInputs();

  // Show config dialog
  bool showConfigDialog();

  // Populate results table
  void populateResults(int maxRows = 100);

  // Solve hangman in background thread
  void solveHangman();

  // Handle input changes
  void onPatternChanged();
  void onExcludedLettersChanged();
};

#endif // WITH_GUI
