#ifdef WITH_GUI

#include "gui/GameWidget.hpp"
#include <QApplication>
#include <QDebug>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), configInfoLabel(nullptr), progressDialog(nullptr),
      solverThread(nullptr), gameInitialized(false),
      cancellationRequested(false) {}

GameWidget::~GameWidget() {
  // Simple, fast cleanup - let Qt handle the rest
  cancellationRequested.store(true, std::memory_order_release);

  if (solverThread && solverThread->isRunning()) {
    solverThread->requestInterruption();
    // Don't wait - just let it finish on its own
    // The thread will be cleaned up by Qt's parent-child relationship
  }

  // Progress dialog is our child, Qt will delete it
  // No manual cleanup needed
}

void GameWidget::createProgressDialog(const QString &labelText, int minimum,
                                      int maximum) {
  // Clean up old dialog if it exists
  if (progressDialog) {
    progressDialog->close();
    progressDialog->deleteLater();
    progressDialog = nullptr;
  }
  progressDialog =
      new QProgressDialog(labelText, "Cancel", minimum, maximum, this);
  // Non-modal so the UI remains fully responsive while the solver runs.
  // The dialog can still be canceled by the user.
  progressDialog->setWindowModality(Qt::NonModal);
  progressDialog->setMinimumDuration(500);
  progressDialog->setAutoClose(true);
  progressDialog->setAutoReset(true);

  // Simple cancel handler - just request interruption
  // Ask the solver thread to stop if the user cancels. Use a queued
  // connection to avoid running thread-interrupt logic inline in the
  // progress dialog event handler.
  connect(
      progressDialog, &QProgressDialog::canceled, this,
      [this]() {
        cancellationRequested.store(true, std::memory_order_release);
        if (solverThread) {
          // Prefer requesting interruption; do not block here.
          solverThread->requestInterruption();
        }
        // Close dialog immediately on cancel (UI action only)
        if (progressDialog) {
          progressDialog->close();
        }
      },
      Qt::QueuedConnection);

  progressDialog->setValue(0);

  // If a solver thread exists, connect its finished signal to cleanup the
  // dialog and the thread itself. Use queued connections so cleanup runs on
  // the GUI thread safely and non-blocking.
  if (solverThread) {
    connect(
        solverThread, &QThread::started, this,
        [this]() {
          // Reset cancellation flag at the start of a run and show dialog
          cancellationRequested.store(false, std::memory_order_release);
          if (progressDialog) {
            progressDialog->show();
          }
        },
        Qt::QueuedConnection);

    connect(
        solverThread, &QThread::finished, this,
        [this]() {
          // Ensure cancellation flag is cleared for future runs
          cancellationRequested.store(false, std::memory_order_release);
          // Close and delete the progress dialog on the GUI thread
          if (progressDialog) {
            progressDialog->reset();
            cleanupProgressDialog();
          }
          // Clean up the thread object (deleteLater to avoid blocking)
          cleanupSolverThread();
        },
        Qt::QueuedConnection);
  }
}

void GameWidget::cleanupProgressDialog() {
  if (progressDialog) {
    // Use reset() to make sure any blocking UI is dismissed, then schedule
    // deletion. Do not call delete synchronously.
    progressDialog->reset();
    progressDialog->deleteLater();
    progressDialog = nullptr;
  }
}

void GameWidget::cleanupSolverThread() {
  if (!solverThread)
    return;

  // If the thread is still running, request interruption and schedule
  // deletion when it finishes. Don't block the UI by waiting here.
  if (solverThread->isRunning()) {
    solverThread->requestInterruption();
    // Attempt a cooperative shutdown by quitting the event loop if present.
    // This is safe when the thread runs an event loop; otherwise it is a no-op.
    solverThread->quit();

    // Ensure the object is deleted once finished.
    connect(solverThread, &QThread::finished, solverThread,
            &QObject::deleteLater, Qt::QueuedConnection);
    // Clear our reference; the object will delete itself later.
    solverThread = nullptr;
    return;
  }

  // Not running: safe to schedule deletion immediately.
  solverThread->deleteLater();
  solverThread = nullptr;
}

void GameWidget::terminateThread(QThread *thread) {
  if (!thread)
    return;

  if (thread->isRunning()) {
    // Ask the thread to stop cooperatively. Do not wait here; waiting
    // would block the GUI. If the thread runs an event loop, also ask it
    // to quit.
    thread->requestInterruption();
    thread->quit();

    // Ensure the thread object is deleted when it is finished. Use a queued
    // connection to avoid immediate deletion from another thread.
    connect(thread, &QThread::finished, thread, &QObject::deleteLater,
            Qt::QueuedConnection);
  } else {
    // Not running: safe to schedule deletion now
    thread->deleteLater();
  }
}

bool GameWidget::isSolverRunning() const {
  return solverThread && solverThread->isRunning();
}

#endif // WITH_GUI
