#pragma once

#include <QString>
#include <QTreeWidgetItem>

namespace prs {

// Leaves (items with Qt::UserRole data) match by name; a group stays visible
// while any descendant matches, or always when `text` is empty (so empty groups
// show). Returns whether `item` is visible.
inline bool filterTreeItem(QTreeWidgetItem* item, const QString& text) {
    bool visible = false;
    if (item->data(0, Qt::UserRole).isValid()) {
        visible = item->text(0).contains(text, Qt::CaseInsensitive);
    } else {
        for (int i = 0; i < item->childCount(); ++i)
            visible |= filterTreeItem(item->child(i), text);
        if (text.isEmpty()) visible = true;
        else if (visible) item->setExpanded(true);
    }
    item->setHidden(!visible);
    return visible;
}

} // namespace prs
