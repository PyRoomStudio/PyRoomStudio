#pragma once

#include <QString>

class QWidget;

namespace prs {
namespace FileDialogs {

/** Single entry point for open-file dialogs (native where supported). */
QString openFile(QWidget* parent, const QString& caption, const QString& dir, const QString& filter);

/** Single entry point for save-file dialogs. */
QString saveFile(QWidget* parent, const QString& caption, const QString& dir, const QString& filter);

/** Single entry point for folder pickers. */
QString existingDirectory(QWidget* parent, const QString& caption, const QString& dir = QString());

} // namespace FileDialogs
} // namespace prs
