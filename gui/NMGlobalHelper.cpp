
#include "nmlog.h"
#include "NMGlobalHelper.h"
#include <QObject>
#include <QApplication>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>


QString
NMGlobalHelper::getMultiLineInput(const QString& title,
                          const QString& suggestion,
                          QWidget *parent)
{
    QDialog* dlg = new QDialog(parent);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setWindowTitle(title);

    QPlainTextEdit* textEdit = new QPlainTextEdit(suggestion, dlg);
    QPushButton* btnCancel = new QPushButton("Cancel", dlg);
    dlg->connect(btnCancel, SIGNAL(pressed()), dlg, SLOT(reject()));
    QPushButton* btnOk = new QPushButton("Ok", dlg);
    dlg->connect(btnOk, SIGNAL(pressed()), dlg, SLOT(accept()));

    QVBoxLayout* vlayout = new QVBoxLayout(dlg);
    vlayout->addWidget(textEdit);

    QHBoxLayout* hlayout = new QHBoxLayout(dlg);
    hlayout->addWidget(btnCancel);
    hlayout->addWidget(btnOk);
    vlayout->addItem(hlayout);

    dlg->setLayout(vlayout);

    int ret = dlg->exec();
    NMDebugAI(<< "dialog closed: " << ret << std::endl);

    QString retText = textEdit->toPlainText();
    if (ret == 0)
    {
        retText = "0";
    }
    NMDebugAI(<< "user text: " << retText.toStdString() << std::endl);

    dlg->deleteLater();
    return retText;

}

OtbModellerWin*
NMGlobalHelper::getMainWindow()
{
    OtbModellerWin* mainWin = 0;
    QWidgetList tlw = qApp->topLevelWidgets();
    QWidgetList::ConstIterator it = tlw.constBegin();
    for (; it != tlw.constEnd(); ++it)
    {
        QWidget* w = const_cast<QWidget*>(*it);
        if (w->objectName().compare("OtbModellerWin") == 0)
        {
            mainWin = qobject_cast<OtbModellerWin*>(w);
        }
    }

    return mainWin;
}

vtkRenderWindow*
NMGlobalHelper::getRenderWindow()
{
   return getMainWindow()->getRenderWindow();
}

QVTKWidget*
NMGlobalHelper::getVTKWidget()
{
    return this->getMainWindow()->ui->qvtkWidget;
}

void
NMGlobalHelper::startBusy()
{
    this->getMainWindow()->showBusyStart();
}

void
NMGlobalHelper::endBusy()
{
    this->getMainWindow()->showBusyEnd();
}
