#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace prs {

/** QMessageBox helpers that set the window icon (required on macOS for correct app icon in alerts). */
void showWarning(QWidget* parent, const QString& title, const QString& text);

QMessageBox::StandardButton showQuestionYesNo(QWidget* parent, const QString& title, const QString& text);

QMessageBox::StandardButton showQuestionSaveDiscardCancel(QWidget* parent, const QString& title,
    const QString& text);

} // namespace prs
