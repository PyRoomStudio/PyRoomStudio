#include "gui/dialogs/RenderOptionsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QtTest/QtTest>

using namespace prs;

class TestRenderOptionsDialog : public QObject {
    Q_OBJECT
  private slots:
    void testDialogExposesRenderModel() {
        std::vector<RenderOptionsDialog::ListenerEntry> listeners = {{"Listener A", true},
                                                                     {"Listener B", false}};

        RenderOptionsDialog dlg(listeners);

        auto checkboxes = dlg.findChildren<QCheckBox*>();
        QCOMPARE(static_cast<int>(checkboxes.size()), 2);
        QCOMPARE(checkboxes[0]->isChecked(), true);
        QCOMPARE(checkboxes[1]->isChecked(), false);

        auto combos = dlg.findChildren<QComboBox*>();
        QCOMPARE(static_cast<int>(combos.size()), 1);
        combos[0]->setCurrentIndex(2);

        auto spins = dlg.findChildren<QSpinBox*>();
        QCOMPARE(static_cast<int>(spins.size()), 2);
        spins[0]->setValue(5);
        spins[1]->setValue(2400);

        auto options = dlg.renderOptions();
        QCOMPARE(options.method, RenderMethod::DG_3D);
        QCOMPARE(options.dgPolyOrder, 5);
        QCOMPARE(options.dgMaxFrequency, 2400.0f);

        auto selected = dlg.selectedListenerIndices();
        QCOMPARE(static_cast<int>(selected.size()), 1);
        QCOMPARE(selected[0], 0);
    }
};

QTEST_MAIN(TestRenderOptionsDialog)
#include "test_render_options_dialog.moc"
