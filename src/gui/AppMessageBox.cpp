#include "AppMessageBox.h"

#include <QApplication>

namespace prs {

namespace {

void applyWindowIcon(QMessageBox& box)
{
    const QIcon icon = QApplication::windowIcon();
    if (!icon.isNull())
        box.setWindowIcon(icon);
}

} // namespace

void showWarning(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox box(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
    applyWindowIcon(box);
    box.exec();
}

QMessageBox::StandardButton showQuestionYesNo(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);
    applyWindowIcon(box);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton showQuestionSaveDiscardCancel(QWidget* parent, const QString& title,
    const QString& text)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    applyWindowIcon(box);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

} // namespace prs
