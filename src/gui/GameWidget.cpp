#ifdef WITH_GUI

#include "gui/GameWidget.hpp"
#include <QApplication>
#include <QDialog>
#include <QPointer>
#include <QTimer>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), configInfoLabel(nullptr), progressDialog(nullptr),
      solverThread(nullptr), gameInitialized(false),
      cancellationRequested(false) {}

GameWidget::~GameWidget() {
  // Simple, fast cleanup - let Qt handle the rest
  cancellationRequested.store(true, std::memory_order_release);

  if (solverThread && solverThread->isRunning()) {
    // Ask for cooperative shutdown and schedule deletion when finished.
    solverThread->requestInterruption();
    // Attempt a cooperative shutdown by quitting the event loop if present.
    solverThread->quit();
    connect(solverThread, &QThread::finished, solverThread,
            &QObject::deleteLater, Qt::QueuedConnection);
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
        // User requested cancellation: mark the flag and attempt cooperative
        // interruption. Use QPointer to avoid dangling references if
        // solverThread is changed or deleted.
        cancellationRequested.store(true, std::memory_order_release);
        QPointer<QThread> threadRef(solverThread);
        if (threadRef) {
          threadRef->requestInterruption();
        }

        // Keep the progress dialog visible until the worker actually
        // finishes. Update the label to indicate cancellation is in
        // progress and disable the cancel button to prevent repeated
        // requests.
        if (progressDialog) {
          progressDialog->setLabelText("Cancelling...");
          // Hide the cancel button if the API is available; otherwise
          // clear its text to discourage further interaction.
          progressDialog->setCancelButton(nullptr);
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

bool GameWidget::handleSolverFinished() {
  if (!solverThread) {
    return false;
  }

  // Close progress dialog immediately
  if (progressDialog) {
    progressDialog->close();
  }

  // Check if cancellation was requested using our atomic flag
  // (isInterruptionRequested() may not be reliable when called from main
  // thread)
  if (cancellationRequested.load(std::memory_order_acquire)) {
    // Don't clear results, don't show messages - just clean up and return
    cleanupProgressDialog();
    cleanupSolverThread();
    return false;
  }

  // Processing should continue
  return true;
}

#endif // WITH_GUI
