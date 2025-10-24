#ifdef WITH_GUI

#pragma once

#include <QLabel>
#include <QProgressDialog>
#include <QThread>
#include <QWidget>
#include <atomic>
#include <memory>

/**
 * @brief Base class for all game widgets to ensure consistent threading and
 * lifecycle management
 *
 * This class provides:
 * - Safe thread cleanup in destructor
 * - Consistent progress dialog management
 * - Common UI state management
 * - Thread-safe cancellation support
 */
class GameWidget : public QWidget {
  Q_OBJECT

public:
  explicit GameWidget(QWidget *parent = nullptr);
  virtual ~GameWidget();

  // Public interface that all game widgets should implement
public slots:
  virtual void newGame() = 0;

protected slots:
  // Common slots for all game widgets
  virtual void onNewGame() = 0;
  virtual void onSettings() = 0;

  // Called when solver thread finishes
  virtual void onSolverFinished() = 0;

protected:
  // Helper methods for consistent UI management
  virtual void initGame() = 0;
  virtual void setUIEnabled(bool enabled) = 0;
  virtual void updateConfigInfo() = 0;

  // Thread and progress dialog management
  void createProgressDialog(const QString &labelText, int minimum = 0,
                            int maximum = 0);
  void cleanupProgressDialog();
  void cleanupSolverThread();

  // Safe thread termination
  void terminateThread(QThread *thread);

  // Check if solver is currently running
  bool isSolverRunning() const;

  // Members that all game widgets use
  QLabel *configInfoLabel;
  QProgressDialog *progressDialog;
  QThread *solverThread;
  bool gameInitialized;

  // Thread-safe cancellation flag
  std::atomic<bool> cancellationRequested;

private:
  // Prevent copying
  GameWidget(const GameWidget &) = delete;
  GameWidget &operator=(const GameWidget &) = delete;
};

#endif // WITH_GUI
