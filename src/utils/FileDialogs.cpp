#include "FileDialogs.h"

#include <QFileDialog>
#include <QWidget>

namespace prs {
namespace FileDialogs {

QString openFile(QWidget* parent, const QString& caption, const QString& dir, const QString& filter)
{
    return QFileDialog::getOpenFileName(parent, caption, dir, filter);
}

QString saveFile(QWidget* parent, const QString& caption, const QString& dir, const QString& filter)
{
    return QFileDialog::getSaveFileName(parent, caption, dir, filter);
}

QString existingDirectory(QWidget* parent, const QString& caption, const QString& dir)
{
    return QFileDialog::getExistingDirectory(parent, caption, dir);
}

} // namespace FileDialogs
} // namespace prs
