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
#include <QListView>
#include <QStringListModel>

#include "QVTKOpenGLWidget.h"

#include "ui_lumassmainwin.h"
#include "lumassmainwin.h"
#include "NMGlobalHelper.h"
#include "NMModelController.h"
#include "NMIterableComponent.h"


#include "nmlog.h"

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
    QPushButton* btnOk = new QPushButton("OK", dlg);
    dlg->connect(btnOk, SIGNAL(pressed()), dlg, SLOT(accept()));

    QVBoxLayout* vlayout = new QVBoxLayout(dlg);
    vlayout->addWidget(textEdit);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addWidget(btnOk);
    hlayout->addWidget(btnCancel);
    vlayout->addItem(hlayout);

    //dlg->setLayout(vlayout);

    int ret = dlg->exec();
    //NMDebugAI(<< "dialog closed: " << ret << std::endl);

    QString retText = textEdit->toPlainText();
    //    if (ret == 0)
    //    {
    //        retText = "0";
    //    }
    //NMDebugAI(<< "user text: " << retText.toStdString() << std::endl);

    dlg->deleteLater();
    return retText;

}

QString
NMGlobalHelper::getUserSetting(const QString& key)
{
    QString ret;

    LUMASSMainWin* win = NMGlobalHelper::getMainWindow();
    if (win->mSettings.find(key) != win->mSettings.end())
    {
        ret = win->mSettings[key].toString();
    }
    return ret;
}

QStringList
NMGlobalHelper::getUserSettingsList(void)
{
    QStringList ret = NMGlobalHelper::getMainWindow()->mSettings.keys();
    return ret;
}

QStringList
NMGlobalHelper::getModelSettingsList(void)
{
    NMModelController* ctrl = NMGlobalHelper::getModelController();
    return ctrl->getModelSettingsList();
}


QStringList
NMGlobalHelper::getMultiItemSelection(const QString& title,
                                     const QString& label,
                                     const QStringList& items,
                                     QWidget* parent)
{
    if (items.size() == 0)
    {
        return QStringList();
    }


    QDialog* dlg = new QDialog(parent);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setWindowTitle(title);

    QLabel* userPrompt = new QLabel(label, dlg);
    QListView* lstView = new QListView(dlg);
    lstView->setSelectionMode(QAbstractItemView::MultiSelection);
    lstView->setModel(new QStringListModel(items, lstView));

    QPushButton* btnCancel = new QPushButton("Cancel", dlg);
    dlg->connect(btnCancel, SIGNAL(pressed()), dlg, SLOT(reject()));
    QPushButton* btnOk = new QPushButton("OK", dlg);
    dlg->connect(btnOk, SIGNAL(pressed()), dlg, SLOT(accept()));

    QHBoxLayout* hbox = new QHBoxLayout();
    hbox->addWidget(btnOk);
    hbox->addWidget(btnCancel);

    QVBoxLayout* vbox = new QVBoxLayout(dlg);
    vbox->addWidget(userPrompt);
    vbox->addWidget(lstView);
    vbox->addItem(hbox);

    int ret = dlg->exec();

    QStringList retList;
    if (ret)
    {
        QModelIndexList rows = lstView->selectionModel()->selectedRows();
        foreach(const QModelIndex& idx, rows)
        {
            retList << items.at(idx.row());
        }
    }

    dlg->deleteLater();
    return retList;
}


void
NMGlobalHelper::appendLogMsg(const QString& msg)
{
    NMGlobalHelper::getMainWindow()->appendLogMsg(msg);
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

NMModelController*
NMGlobalHelper::getModelController(void)
{
    return NMGlobalHelper::getMainWindow()->ui->modelViewWidget->getModelController();
}

QStringList
NMGlobalHelper::searchPropertyValues(const QObject* obj, const QString& searchTerm)
{
    QStringList props;
    if (obj == 0 || searchTerm.isEmpty())
    {
        return props;
    }

    int nprops = obj->metaObject()->propertyCount();
    for (int i=0; i < nprops; ++i)
    {
        bool bfound = false;

        QString propName = obj->metaObject()->property(i).name();
        QVariant pVal = obj->property(propName.toStdString().c_str());
        if (QString::fromLatin1("QStringList").compare(pVal.typeName()) == 0)
        {
            QString aStr = pVal.toStringList().join(' ');
            bfound = aStr.contains(searchTerm, Qt::CaseInsensitive);
        }
        else if (QString::fromLatin1("QList<QStringList>").compare(pVal.typeName()) == 0)
        {
            QString allString;
            QList<QStringList> lsl = pVal.value<QList<QStringList> >();
            foreach(const QStringList& sl, lsl)
            {
                allString += sl.join(' ') + ' ';
            }
            bfound = allString.contains(searchTerm, Qt::CaseInsensitive);
        }
        else if (QString::fromLatin1("QList<QList<QStringList> >").compare(pVal.typeName()) == 0)
        {
            QString allallString;
            QList<QList<QStringList> > llsl = pVal.value<QList<QList<QStringList> > >();

            foreach(const QList<QStringList>& lsl2, llsl)
            {
                foreach(const QStringList& sl2, lsl2)
                {
                    allallString += sl2.join(' ') + ' ';
                }
            }
            bfound = allallString.contains(searchTerm, Qt::CaseInsensitive);
        }
        else
        {
            bfound = pVal.toString().contains(searchTerm, Qt::CaseInsensitive);
        }

        if (bfound)
        {
            props << propName;
        }
    }

    return props;
}


NMLogWidget*
NMGlobalHelper::getLogWidget()
{
    return NMGlobalHelper::getMainWindow()->getLogWidget();
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

    int maxcolidx = model->columnCount()-1;

    int start = ids[0];
    int end = start;
    for (int i=1; i < ids.size(); ++i)
    {
        if (ids[i] > end+1)
        {
            QModelIndex tl = model->index(start, 0);
            QModelIndex br = model->index(end, maxcolidx);
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
        QModelIndex tl = model->index(start, 0);
        QModelIndex br = model->index(ids.last(), maxcolidx);
        newsel.append(QItemSelectionRange(tl, br));

    }

    NMDebugCtx(ctx, << "done!");
    return newsel;
}

QString
NMGlobalHelper::getRandomString(int len)
{
    if (len < 1)
    {
        return QString();
    }

    //std::srand(std::time(0));
    char* nam = new char[len+1];
    for (int i=0; i < len; ++i)
    {
        if (i == 0)
        {
            if (::rand() % 2 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else
            {
                nam[i] = ::rand() % 26 + 97;
            }
        }
        else
        {
            if (::rand() % 7 == 0)
            {
                nam[i] = '_';
            }
            else if (::rand() % 5 == 0)
            {
                nam[i] = ::rand() % 26 + 65;
            }
            else if (::rand() % 3 == 0)
            {
                nam[i] = ::rand() % 26 + 97;
            }
            else
            {
                nam[i] = ::rand() % 10 + 48;
            }
        }
    }
    nam[len] = '\0';
    QString ret = nam;
    delete[] nam;

    return ret;
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

QVTKOpenGLWidget*
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
