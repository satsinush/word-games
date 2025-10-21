#ifdef WITH_GUI

#pragma once

#include "spellingBee/spellingBee.hpp"
#include "utils/wordUtils.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QTableWidget>
#include <QThread>
#include <QWidget>
#include <array>
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

class SpellingBeeWidget : public QWidget {
  Q_OBJECT

public:
  explicit SpellingBeeWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~SpellingBeeWidget();

public slots:
  void newGame();

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame();
  void onSettings();
  void onSolverFinished();

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const SpellingBee::Config &cfg,
                 const std::vector<Utils::Word> &words)
        : config(cfg), wordVec(words) {}

    std::vector<Utils::Word> getResult() const { return solutions; }

  protected:
    void run() override {
      solutions = SpellingBee::runSpellingBeeSolver(wordVec, config);
    }

  private:
    SpellingBee::Config config;
    const std::vector<Utils::Word> &wordVec;
    std::vector<Utils::Word> solutions;
  };

private:
  Ui::SpellingBeeWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  SpellingBee::Config config;
  std::vector<Utils::Word> solutions;
  bool gameInitialized;

  QLabel *configInfoLabel;
  QTableWidget *resultsTable;
  QWidget *hexWidget;
  std::array<HexagonButton *, 7> hexButtons;
  QProgressDialog *progressDialog;
  SolverThread *solverThread;

  bool showConfigDialog();
  void initGame();
  void setUIEnabled(bool enabled);
  void updateConfigInfo();
  void populateResultTable();
  void createHexagons();
  void updateHexagonsFromInput(const QString &text);
};

#endif // WITH_GUI
