#ifdef WITH_GUI

#pragma once

#include "gui/GameWidget.hpp"
#include "spellingBee/spellingBee.hpp"
#include "utils/wordUtils.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QTableWidget>
#include <QThread>
#include <QWidget>
#include <array>
#include <atomic>
#include <vector>

namespace Ui {
class SpellingBeeWidget;
}

class HexagonButton : public QPushButton {
  Q_OBJECT
public:
  explicit HexagonButton(bool isCenter = false, QWidget *parent = nullptr);
  void setLetter(char letter);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  bool isCenterHex;
  char currentLetter;
};

class SpellingBeeWidget : public GameWidget {
  Q_OBJECT

public:
  explicit SpellingBeeWidget(QWidget *parent = nullptr);
  ~SpellingBeeWidget() override;

public slots:
  void newGame() override;

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame() override;
  void onSettings() override;
  void onSolverFinished() override;

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const SpellingBee::Config &cfg, std::atomic<bool> *cancelFlag)
        : config(cfg), cancellationFlag(cancelFlag) {}

    std::vector<Utils::Word> getResult() const { return solutions; }

  protected:
    void run() override {
      solutions = SpellingBee::runSpellingBeeSolver(config, cancellationFlag);
    }

  private:
    SpellingBee::Config config;
    // Copy of the word vector so the worker thread doesn't hold references
    // to GUI-owned data.
    std::vector<Utils::Word> solutions;
    std::atomic<bool> *cancellationFlag;
  };

private:
  Ui::SpellingBeeWidget *ui;

  SpellingBee::Config config;
  std::vector<Utils::Word> solutions;

  QTableWidget *resultsTable;
  QWidget *hexWidget;
  std::array<HexagonButton *, 7> hexButtons;

  bool showConfigDialog();
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;
  void populateResults(int maxRows = 1000);
  void updateHexagonsFromInput(const QString &text);
};

#endif // WITH_GUI
