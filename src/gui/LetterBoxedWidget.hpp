#ifdef WITH_GUI

#pragma once

#include "letterBoxed/letterBoxed.hpp"
#include "utils/wordUtils.hpp"
#include <QLabel>
#include <QTableWidget>
#include <QWidget>
#include <array>
#include <bitset>
#include <vector>

namespace Ui {
class LetterBoxedWidget;
}

class LetterBoxDisplay : public QWidget {
  Q_OBJECT
public:
  explicit LetterBoxDisplay(QWidget *parent = nullptr);
  void setLetters(const std::array<char, 12> &letters);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::array<char, 12> currentLetters;
};

class LetterBoxedWidget : public QWidget {
  Q_OBJECT

public:
  explicit LetterBoxedWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~LetterBoxedWidget();

public slots:
  void newGame();

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame();
  void onSolve();
  void onSettings();

private:
  Ui::LetterBoxedWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  LetterBoxed::Config config;
  std::vector<LetterBoxed::Solution> solutions;
  bool gameInitialized;

  QLabel *configInfoLabel;
  QTableWidget *resultsTable;
  LetterBoxDisplay *boxDisplay;

  bool showConfigDialog();
  void initGame();
  void setUIEnabled(bool enabled);
  void updateConfigInfo();
  void populateResultTable();
  void createLetterBox();
  void updateLetterBoxFromInput(const QString &text);
};

#endif // WITH_GUI
