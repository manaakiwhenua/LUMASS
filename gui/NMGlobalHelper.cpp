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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListView>
#include <QStringListModel>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>

//#include "QVTKOpenGLWidget.h"

#include "ui_lumassmainwin.h"
#include "lumassmainwin.h"
#include "NMParamEdit.h"
#include "NMParamHighlighter.h"
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

    //QTextEdit* textEdit = new QTextEdit(suggestion, dlg);
    NMParamEdit* textEdit = new NMParamEdit(dlg);
    textEdit->setPlainText(suggestion);

    if (    parent != nullptr
        &&  QString::fromLatin1("NMSqlTableView").compare(parent->metaObject()->className(), Qt::CaseSensitive) == 0
       )
    {
        NMParamHighlighter* highlighter = new NMParamHighlighter(textEdit);
        highlighter->setExpressionType(NMParamHighlighter::NM_SQLITE_EXP);

        QColor bkg = textEdit->palette().background().color();
        bool bdark = false;
        if (bkg.red() < 80 && bkg.green() < 80 && bkg.blue() < 80)
        {
            highlighter->setDarkColorMode(true);
        }
    }

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
    if (ret == 0)
    {
        retText.clear();
    }
    //NMDebugAI(<< "user text: " << retText.toStdString() << std::endl);

    dlg->deleteLater();
    return retText;

}

QString
NMGlobalHelper::getNetCDFVarPathInput(const QString& ncFilename)
{
    QString ret;
    using namespace netCDF;

    QStringList varPaths;
    try
    {
        NcFile nc(ncFilename.toStdString(), NcFile::read);
        if (nc.isNull())
        {
            ret = QString("ERROR - Failed reading '%1'!").arg(ncFilename);
            return ret;
        }

        NcGroup grp(nc.getId());
        NMGlobalHelper::parseNcGroup(grp, varPaths);

    }
    catch(exceptions::NcException& e)
    {
        ret = QString("ERROR - %1").arg(e.what());
        return ret;
    }

    bool bOK;
    ret = QInputDialog::getItem(nullptr, "NcVars...", "Pick a variable to display", varPaths, 0, false, &bOK);
    if (!bOK)
    {
        ret = QString("ERROR - Well, you canceled it - not much I can do about that!");
        return ret;
    }

    return ret;
}

void
NMGlobalHelper::parseNcGroup(const netCDF::NcGroup& grp, QStringList& varPaths)
{
    using namespace netCDF;

    try
    {
        // collecting the variables of this group
        std::multimap<std::string, NcVar> vars = grp.getVars();
        std::multimap<std::string, NcVar>::const_iterator varIt = vars.cbegin();
        std::string gpath = grp.getName(true).c_str();
        if (gpath.compare("/") == 0)
        {
            gpath.clear();
        }
        std::stringstream dimstr;
        while (varIt != vars.cend())
        {
            std::vector<NcDim> dims = varIt->second.getDims();
            for (int d=dims.size()-1; d >=0; --d)
            {
                dimstr << dims[d].getName() << ": " << dims[d].getSize();
                if (d > 0)
                {
                    dimstr << ", ";
                }
            }

            QString ventry = QString("%1/%2 (%3)").arg(gpath.c_str()).arg(varIt->first.c_str()).arg(dimstr.str().c_str());
            varPaths << ventry;
            ++varIt;
            dimstr.str("");
        }

        // parsing any subgroups
        std::multimap<std::string, NcGroup> groups = grp.getGroups();
        std::multimap<std::string, NcGroup>::const_iterator groupsIt = groups.cbegin();
        while (groupsIt != groups.cend())
        {
            NMGlobalHelper::parseNcGroup(groupsIt->second, varPaths);
            ++groupsIt;
        }
    }
    catch(exceptions::NcException& e)
    {
        throw e;
    }
}


void
NMGlobalHelper::logQsqlConnections(void)
{
    QStringList conList = QSqlDatabase::connectionNames();
    foreach(const QString con, conList)
    {
        QSqlDatabase db = QSqlDatabase::database(con, false);
        if (db.isValid() && db.isOpen())
        {
            QString dbname = db.databaseName();
            QString conopt = db.connectOptions();
            NMGlobalHelper::appendLogMsg(QString("QSqlConnection: %1 -> %2 (Options: %3)\n").arg(con).arg(dbname).arg(conopt));
        }
    }
}

QStringList
NMGlobalHelper::identifyExternalDbs(QSqlDatabase targetDb, const QString &origexpr)
{
    QStringList ret;
    if (!targetDb.isOpen() || origexpr.isEmpty())
    {
        return ret;
    }
    QString expr = origexpr;

    // extracting table names from the expression
    QMultiMap<QString, QString> tabDbNames = NMGlobalHelper::getMainWindow()->getTableDbNamesList();
    QStringList tokens = expr.trimmed().split(' ');
    foreach (const QString& tok, tokens)
    {
        QStringList septokens = tok.split('.');
        foreach(const QString& toktok, septokens)
        {
            QString _str = toktok;
            _str = _str.replace("\"", "").replace("'", "");
            QString fn = tabDbNames.key(_str);
            if (    !fn.isEmpty()
                &&  !ret.contains(fn)
                &&  fn.compare(targetDb.databaseName(), Qt::CaseSensitive) != 0
               )
            {
                ret << fn;
            }
        }
    }
    ret.removeDuplicates();

    // fetch a list of databases that have been already attached to this database
    QString qstr = QString(QStringLiteral("select distinct file from pragma_database_list where file is not null"));

    QSqlQuery prQuery(targetDb);
    if (!prQuery.exec(qstr))
    {
        QString msg = QString("FETCH DATBASE LIST ERROR: %1").arg(prQuery.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        return ret;
    }

    while (prQuery.next())
    {
        QString dbfn = prQuery.value(0).toString();
        ret.removeOne(dbfn);
    }
    prQuery.finish();

    return ret;
}

bool
NMGlobalHelper::attachMultipleDbs(QSqlDatabase dbTarget, const QStringList& dbFileNames)
{
    bool ret = false;
    if (!dbTarget.isOpen())
    {
        return ret;
    }

    int dbcounter = 0;
    foreach (const QString& fn, dbFileNames)
    {
        QString sname;// = QString("db_%1").arg(++dbcounter);
        sname = QFileInfo(fn).baseName();
        if (NMGlobalHelper::attachDatabase(dbTarget, fn, sname))
        {
            ret = true;
        }
    }

    return ret;
}


bool
NMGlobalHelper::attachDatabase(QSqlDatabase dbTarget, const QString dbFileName, const QString schemaName)
{
    // the session db is in-memory, so can't check it's readbility and existence ...
    if (dbFileName.compare(NMGlobalHelper::getMainWindow()->getSessionDbFileName()) != 0)
    {
        QFileInfo fifo(dbFileName);
        if (!fifo.exists() || !fifo.isReadable())
        {
            return false;
        }
    }

    QSqlDriver* drv = dbTarget.driver();
    if (drv == nullptr)
    {
        return false;
    }

    const QString qStr = QString("ATTACH DATABASE '%1' as %2")
            .arg(dbFileName)
            .arg(drv->escapeIdentifier(schemaName, QSqlDriver::TableName));

    QSqlQuery q(dbTarget);
    if (!q.exec(qStr))
    {
        QString msg = QString("ATTACH DATABASE ERROR: %1").arg(q.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        q.finish();
        return false;
    }
    q.finish();

    return true;
}

bool NMGlobalHelper::detachDatabase(QSqlDatabase db, QString schemaName)
{
    if (!db.isOpen())
    {
        QString msg = QString("Failed detaching database '%1' from '%2': Target database is not open!")
                .arg(schemaName).arg(db.databaseName());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        return false;
    }

    QSqlDriver* drv = db.driver();
    if (drv == nullptr)
    {
        return false;
    }

    const QString qStr = QString("DETACH DATABASE %1")
            .arg(drv->escapeIdentifier(schemaName, QSqlDriver::TableName));

    QSqlQuery q(db);
    if (!q.exec(qStr))
    {
        QString msg = QString("DETACH DATABASE ERROR: %1").arg(q.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        q.finish();
        return false;
    }
    q.finish();

    return true;
}

bool NMGlobalHelper::detachMultipleDbs(QSqlDatabase dbTarget, const QStringList &dbFileNames)
{
    bool ret = true;
    foreach(const QString& fn, dbFileNames)
    {
        const QString schema = QFileInfo(fn).baseName();
        if (!detachDatabase(dbTarget, schema))
        {
            ret = false;
        }
    }

    return ret;
}

bool NMGlobalHelper::detachAllDbs(QSqlDatabase dbTarget)
{
    if (!dbTarget.isOpen())
    {
        QString msg = QString("Failed detaching external databases from '%1'. "
                              "The database is not open!").arg(dbTarget.databaseName());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        return false;
    }

#ifdef LUMASS_DEBUG

    QString strEdbs = QString("select name, file from pragma_database_list");

    NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                        QDateTime::currentDateTime().time().toString(),
                        NMLogger::NM_LOG_DEBUG,
                        QStringLiteral("detachAllDbs() - current external dbs ... "));


    QSqlQuery externalDbs(dbTarget);
    externalDbs.exec(strEdbs);
    while (externalDbs.next())
    {
       QString name = externalDbs.value(0).toString();
       QString file = externalDbs.value(1).toString();
       QString msg = QString("%1 | %2").arg(name).arg(file);
       NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                           QDateTime::currentDateTime().time().toString(),
                           NMLogger::NM_LOG_DEBUG,
                           msg);
    }
    externalDbs.finish();

#endif

    // ======================================================================
    //              GET A LIST OF DETACHABLE DATABASE SCHEMA NAMES
    // ======================================================================

    QString strDetach = QString("Select name from pragma_database_list "
                                "where name != 'main' and name != 'temp';");

    QSqlQuery qDetach(dbTarget);
    if (!qDetach.exec(strDetach))
    {
        QString msg = QString("Failed detaching external databases from '%1': %2")
                .arg(dbTarget.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        qDetach.finish();
        return false;
    }

    QStringList detachNames;
    while (qDetach.next())
    {
        detachNames << qDetach.value(0).toString();
    }
    qDetach.finish();


    // ======================================================================
    //              DETACH INDIVIDUAL SCHEMA NAMES
    // ======================================================================


    if (!dbTarget.transaction())
    {
        QString msg = QString("Failed detaching external databases from '%1'. "
                              "%2").arg(dbTarget.databaseName())
                                   .arg(dbTarget.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        dbTarget.rollback();
        return false;
    }


    QString strD2 = QString("detach database ?");
    QSqlQuery qD2(dbTarget);
    if (!qD2.prepare(strD2))
    {
        QString msg = QString("Failed detaching external databases from '%1': %2")
                .arg(dbTarget.databaseName()).arg(qD2.lastError().text());
        NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                            QDateTime::currentDateTime().time().toString(),
                            NMLogger::NM_LOG_ERROR,
                            msg);
        qD2.finish();
        dbTarget.rollback();
        return false;
    }

    foreach(const QString& sName, detachNames)
    {
        const QString dname = dbTarget.driver()->escapeIdentifier(sName, QSqlDriver::TableName);
        qD2.bindValue(0, dname);
        if (!qD2.exec())
        {
            QString msg = QString("Failed detaching database '%1' from '%2': %3")
                    .arg(sName).arg(dbTarget.databaseName()).arg(qD2.lastError().text());
            NMGlobalHelper::getMainWindow()->getLogger()->processLogMsg(
                                QDateTime::currentDateTime().time().toString(),
                                NMLogger::NM_LOG_ERROR,
                                msg);
        }
        qD2.finish();
    }
    qD2.finish();
    dbTarget.commit();

    return true;
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

QVTKOpenGLNativeWidget *NMGlobalHelper::getVTKWidget()
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
