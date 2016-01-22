
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

QItemSelection
NMGlobalHelper::selectRows(const QAbstractItemModel* model,
                          QList<int>& ids)
{
    QItemSelection newsel;

    if (model == 0 || ids.size() == 0)
    {
        return newsel;
    }

    int maxcolidx = model->columnCount();

    int start = ids[0];
    int end = start;
    for (int i=1; i < ids.size(); ++i)
    {
        if (ids[i] > end+1)
        {
            const QModelIndex& tl = model->index(start, maxcolidx);
            const QModelIndex& br = model->index(end, maxcolidx);
            newsel.append(QItemSelectionRange(tl, br));

            start = ids[i];
            end = start;
        }
        else
        {
            end = ids[i];
        }
    }

    //if (end != ids.last())
    {
        const QModelIndex& tl = model->index(start, maxcolidx);
        const QModelIndex& br = model->index(ids.last(), maxcolidx);
        newsel.append(QItemSelectionRange(tl, br));

    }

    return newsel;
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
