/******************************************************************************
* Created by Alexander Herzig
* Copyright 2010-2016 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This programs distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QObject>
#include <QApplication>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "nmlog.h"
#include "ui_lumassmainwin.h"
#include "NMGlobalHelper.h"
#include "NMModelController.h"
#include "NMIterableComponent.h"

const std::string NMGlobalHelper::ctx = "NMGlobalHelper";

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
    //NMDebugAI(<< "dialog closed: " << ret << std::endl);

    QString retText = textEdit->toPlainText();
    if (ret == 0)
    {
        retText = "0";
    }
    //NMDebugAI(<< "user text: " << retText.toStdString() << std::endl);

    dlg->deleteLater();
    return retText;

}

LUMASSMainWin *NMGlobalHelper::getMainWindow()
{
    LUMASSMainWin* mainWin = 0;
    QWidgetList tlw = qApp->topLevelWidgets();
    QWidgetList::ConstIterator it = tlw.constBegin();
    for (; it != tlw.constEnd(); ++it)
    {
        QWidget* w = const_cast<QWidget*>(*it);
        if (w->objectName().compare("LUMASSMainWin") == 0)
        {
            mainWin = qobject_cast<LUMASSMainWin*>(w);
        }
    }

    return mainWin;
}

QItemSelection
NMGlobalHelper::selectRows(const QAbstractItemModel* model,
                          QList<int>& ids)
{
    NMDebugCtx(ctx, << "...");

    QItemSelection newsel;

    if (model == 0 || ids.size() == 0)
    {
        NMDebugCtx(ctx, << "done!");
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

    NMDebugCtx(ctx, << "done!");
    return newsel;
}

qreal
NMGlobalHelper::getLUMASSVersion()
{
    // DO NOT ALTER THE FORMATTING HERE
    // IT'LL BREAK THE *.lmx FILE IMPORT
    QString vnumStr = QString("%1.%2%3")
                        .arg(LUMASS_VERSION_MAJOR)
                        .arg(LUMASS_VERSION_MINOR)
                        .arg(LUMASS_VERSION_REVISION);
    return (qreal)vnumStr.toDouble();
}

vtkRenderWindow*
NMGlobalHelper::getRenderWindow()
{
   return NMGlobalHelper::getMainWindow()->getRenderWindow();
}

QVTKWidget*
NMGlobalHelper::getVTKWidget()
{
    return NMGlobalHelper::getMainWindow()->ui->qvtkWidget;
}

void
NMGlobalHelper::startBusy()
{
    NMGlobalHelper::getMainWindow()->showBusyStart();
}

void
NMGlobalHelper::endBusy()
{
    NMGlobalHelper::getMainWindow()->showBusyEnd();
}
