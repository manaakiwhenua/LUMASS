 /******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*
 * NMSqlTableView.cxx
 *
 *  Created on: 17/08/2015
 *      Author: alex
 */

#include <string>
#include <iostream>
#include <vector>
#include <limits>

#include "NMMacros.h"
#include "NMSqlTableView.h"
#include "NMAddColumnDialog.h"
#include "NMTableCalculator.h"
#include "NMSelSortSqlTableProxyModel.h"
#include "NMGlobalHelper.h"

#include "vtkQtTableModelAdapter.h"
#include "vtkDelimitedTextReader.h"
#include "vtkSmartPointer.h"

#include <QtGui>
#include <QWidget>
#include <QObject>
#include <QMetaObject>
#include <QModelIndex>
#include <QHeaderView>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QPointer>
#include <QStringList>
#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QScopedPointer>
#include <QRegExp>
#include <QCheckBox>
#include <QStyledItemDelegate>

#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlIndex>
#include <QSqlError>

//#include "valgrind/callgrind.h"


NMSqlTableView::NMSqlTableView(QSqlTableModel* model, QWidget* parent)
	: QWidget(parent), mViewMode(NMTABVIEW_ATTRTABLE),
	  mModel(model), mbSwitchSelection(false), mbClearSelection(false),
      mSelectionModel(0), mbIsSelectable(true), mSortFilter(0), mQueryCounter(0)
{
    this->mTableView = new QTableView(this);

    mSortFilter = new NMSelSortSqlTableProxyModel(this);
    mSortFilter->setSourceModel(mModel);

    //this->mTableView->setModel(mSortFilter);
    mModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    this->mTableView->setModel(mModel);

    //this->mProxySelModel = new NMFastTrackSelectionModel(mSortFilter, this);
    this->mProxySelModel = new NMFastTrackSelectionModel(mModel, this);
    this->mProxySelModel->setObjectName("FastProxySelection");
    this->mTableView->setSelectionModel(mProxySelModel);

    this->initView();
}

NMSqlTableView::NMSqlTableView(QSqlTableModel *model, ViewMode mode, QWidget* parent)
	: QWidget(parent), mViewMode(mode),
	  mModel(model), mbSwitchSelection(false), mbClearSelection(false),
      mSelectionModel(0), mbIsSelectable(true), mSortFilter(0), mQueryCounter(0)
{
    this->mTableView = new QTableView(this);

    mSortFilter = new NMSelSortSqlTableProxyModel(this);
    mSortFilter->setSourceModel(mModel);


    this->mTableView->setModel(mModel);

    this->mProxySelModel = new NMFastTrackSelectionModel(mModel, this);
    this->mProxySelModel->setObjectName("FastProxySelection");
    this->mTableView->setSelectionModel(mProxySelModel);

    this->initView();
}


NMSqlTableView::~NMSqlTableView()
{
}

void NMSqlTableView::initView()
{

	this->mTableView->setCornerButtonEnabled(false);
	this->mTableView->setAlternatingRowColors(true);
	this->mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

	this->setWindowFlags(Qt::Window);
	this->mHiddenColumns.clear();

    mPrimaryKey = mSortFilter->getSourcePK();

	// ---------------------------- THE PROGRESS DIALOG -------------------
	mProgressDialog = new QProgressDialog(this);


	// ------------------ SET UP STATUS BAR ------------------------------
	this->mStatusBar = new QStatusBar(this);

	this->mRecStatusLabel = new QLabel(tr(""), this->mStatusBar);
	this->mStatusBar->addWidget(this->mRecStatusLabel);

	this->mBtnClearSelection = new QPushButton(this->mStatusBar);
	this->mBtnClearSelection->setText(tr("Clear Selection"));
	this->mStatusBar->addWidget(this->mBtnClearSelection);

	this->mBtnSwitchSelection = new QPushButton(this->mStatusBar);
	this->mBtnSwitchSelection->setText(tr("Swap Selection"));
	this->mStatusBar->addWidget(this->mBtnSwitchSelection);

	this->mChkSelectedRecsOnly = new QCheckBox(this->mStatusBar);
	this->mChkSelectedRecsOnly->setText(tr("Selected Records Only"));
	this->mChkSelectedRecsOnly->setCheckState(Qt::Unchecked);
	this->mStatusBar->addWidget(this->mChkSelectedRecsOnly);

    // ----------------- SOME STATUS BAR INFORMATION ------------------------------
    this->mlNumRecs = mSortFilter->getNumTableRecords();
    this->updateSelectionAdmin(mlNumRecs);
    this->connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                  this, SLOT(procRowsInserted(QModelIndex, int, int)));


    // ---------------- NMSqlTableView - Layout ------------------------------
	this->mLayout = new QVBoxLayout(this);
	this->mLayout->addWidget(this->mTableView);
	this->mLayout->addWidget(this->mStatusBar);


	// install an event filter on the widget underlying the
	// horizontal header area
    this->mTableView->horizontalHeader()->viewport()->installEventFilter(this);

	// install an event filter on the widget underlying the
	// vertical header area
    this->mTableView->verticalHeader()->viewport()->installEventFilter(this);

	// install an event filter on the itemWidget
    this->mTableView->viewport()->installEventFilter(this);

	// init the context menu of column headers
	this->mColHeadMenu = new QMenu(this);

	QAction* actStat = new QAction(this->mColHeadMenu);
	actStat->setText(tr("Statistics ..."));

	QAction* actExp = new QAction(this->mColHeadMenu);
	actExp->setText(tr("Export Table ..."));

    QAction* actFilter = new QAction(this->mColHeadMenu);
    actFilter->setText(tr("Arbitrary SQL Query ..."));

	QAction* actSel = new QAction(this->mColHeadMenu);
	this->mActSel = actSel;
	actSel->setText(tr("Select Attributes ..."));

	QAction* actHide = new QAction(this->mColHeadMenu);
	actHide->setText(tr("Hide Column"));

	QAction* actUnHide = new QAction(this->mColHeadMenu);
	actUnHide->setText(tr("Unhide Column ..."));


	QAction* actDelete;
	QAction* actAdd;
	QAction* actCalc;
	QAction* actNorm;
	QAction* actJoin;

	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		actDelete = new QAction(this->mColHeadMenu);
		actDelete->setText(tr("Delete Column"));

		actAdd = new QAction(this->mColHeadMenu);
		actAdd->setText(tr("Add Column ..."));

		actCalc = new QAction(this->mColHeadMenu);
		actCalc->setText(tr("Calculate Column ..."));

		actNorm = new QAction(this->mColHeadMenu);
		actNorm->setText(tr("Normalise Attributes ..."));

		actJoin = new QAction(this->mColHeadMenu);
		actJoin->setText(tr("Join Attributes ..."));
	}


	this->mColHeadMenu->addAction(actSel);
    this->mColHeadMenu->addAction(actFilter);
	this->mColHeadMenu->addSeparator();
	this->mColHeadMenu->addAction(actStat);

	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		this->mColHeadMenu->addAction(actCalc);
        //this->mColHeadMenu->addAction(actNorm);
		this->mColHeadMenu->addSeparator();
		this->mColHeadMenu->addAction(actAdd);
		this->mColHeadMenu->addAction(actDelete);
	}
	this->mColHeadMenu->addSeparator();
	this->mColHeadMenu->addAction(actHide);
	this->mColHeadMenu->addAction(actUnHide);

	this->mColHeadMenu->addSeparator();
	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		this->mColHeadMenu->addAction(actJoin);
	}
	this->mColHeadMenu->addAction(actExp);


    // CONNECT MENU ACTIONS WITH SLOTS
	this->connect(this->mBtnClearSelection, SIGNAL(pressed()), this, SLOT(clearSelection()));
	this->connect(this->mBtnSwitchSelection, SIGNAL(pressed()), this, SLOT(switchSelection()));
	this->connect(this->mChkSelectedRecsOnly, SIGNAL(stateChanged(int)), this,
			SLOT(updateSelRecsOnly(int)));

	this->connect(actSel, SIGNAL(triggered()), this, SLOT(selectionQuery()));
	this->connect(actStat, SIGNAL(triggered()), this, SLOT(colStats()));
	this->connect(actExp, SIGNAL(triggered()), this, SLOT(exportTable()));
    this->connect(actFilter, SIGNAL(triggered()), this, SLOT(userQuery()));
	this->connect(actHide, SIGNAL(triggered()), this, SLOT(callHideColumn()));
	this->connect(actUnHide, SIGNAL(triggered()), this, SLOT(callUnHideColumn()));

	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		this->connect(actAdd, SIGNAL(triggered()), this, SLOT(addColumn()));
		this->connect(actDelete, SIGNAL(triggered()), this, SLOT(deleteColumn()));
		this->connect(actCalc, SIGNAL(triggered()), this, SLOT(calcColumn()));
		this->connect(actNorm, SIGNAL(triggered()), this, SLOT(normalise()));
		this->connect(actJoin, SIGNAL(triggered()), this, SLOT(joinAttributes()));
	}

	///////////////////////////////////////////
	// init the context menu for managing rasdaman layers
	this->mManageLayerMenu = new QMenu(this);

	QAction* loadLayer = new QAction(this->mManageLayerMenu);
	loadLayer->setText(tr("Load Layer"));

	QAction* delLayer = new QAction(this->mManageLayerMenu);
	delLayer->setText(tr("Delete Selected Layer(s)"));

	this->mManageLayerMenu->addAction(loadLayer);
	this->mManageLayerMenu->addAction(delLayer);

	this->connect(loadLayer, SIGNAL(triggered()), this, SLOT(loadRasLayer()));
	this->connect(delLayer, SIGNAL(triggered()), this, SLOT(deleteRasLayer()));
}

void
NMSqlTableView::setSelectable(bool bselectable)
{
	if (mbIsSelectable == bselectable)
		return;

	this->mbIsSelectable = bselectable;
	if (bselectable)
	{
		mActSel->setEnabled(true);
		mBtnSwitchSelection->setEnabled(true);
	}
	else
	{
		mActSel->setEnabled(false);
		mBtnSwitchSelection->setEnabled(false);
		this->mSelectionModel->clearSelection();
	}
}

void
NMSqlTableView::connectSelModels(bool bconnect)
{
	if (bconnect)
	{
//		connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
//				                                         const QItemSelection &)),
//				this, SLOT(updateSelectionAdmin(const QItemSelection &,
//						                        const QItemSelection &)));

        connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
                                                         const QItemSelection &)),
                this, SLOT(updateProxySelection(const QItemSelection &,
                                                const QItemSelection &)));
	}
	else
	{
//		disconnect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
//				                                         const QItemSelection &)),
//				this, SLOT(updateSelectionAdmin(const QItemSelection &,
//						                        const QItemSelection &)));

        disconnect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
                                                         const QItemSelection &)),
                this, SLOT(updateProxySelection(const QItemSelection &,
                                                const QItemSelection &)));
	}
}

void
NMSqlTableView::update(void)
{
    this->updateProxySelection(QItemSelection(), QItemSelection());
}

void
NMSqlTableView::setSelectionModel(NMFastTrackSelectionModel* selectionModel)
{
    if (selectionModel && mSelectionModel != 0)
    {
		this->connectSelModels(false);
        delete mSelectionModel;
    }

	this->mSelectionModel = selectionModel;
    this->connectSelModels(true);
	if (this->mSelectionModel)
	{
        //this->mTableView->setSelectionModel(mSelectionModel);
        this->updateProxySelection(QItemSelection(), QItemSelection());
        //this->updateSelectionAdmin(QItemSelection(), QItemSelection());
	}
}

void
NMSqlTableView::setViewMode(ViewMode mode)
{
	switch(mode)
	{
	case NMTABVIEW_ATTRTABLE:
		this->mViewMode = mode;
		this->mTableView->verticalHeader()->show();
		break;
	case NMTABVIEW_RASMETADATA:
		this->mViewMode = mode;
		this->mTableView->verticalHeader()->hide();
		break;
	default:
		break;
	}
}

void NMSqlTableView::updateSelRecsOnly(int state)
{
    NMDebugCtx(__ctxsqltabview, << "...");

    if (!this->mChkSelectedRecsOnly->isEnabled())
    {
        return;
    }

    updateSelection(mbSwitchSelection);

//	if (state == Qt::Checked)
//	{
//		this->mSortFilter->setFilterOn(false);
//		this->mProxySelModel->clearSelection();
//		this->mSortFilter->clearFilter();
//		const QItemSelection srcSel = this->mSelectionModel->selection();
//		const QItemSelection proxySel = this->mSortFilter->mapRowSelectionFromSource(srcSel);
//		this->mSortFilter->addToFilter(proxySel);
//		this->mSortFilter->setFilterOn(true);
//	}
//	else
//	{
//		this->mSortFilter->setFilterOn(false);
//		this->mSortFilter->clearFilter();
//		this->updateProxySelection(QItemSelection(), QItemSelection());
//	}
    NMDebugCtx(__ctxsqltabview, << "done!");
}

void NMSqlTableView::switchSelection()
{
    //NMDebugCtx(__ctxsqltabview, << "...");

    mbSwitchSelection = !mbSwitchSelection;
    updateSelection(mbSwitchSelection);

//	mbSwitchSelection = true;

//	if (mSelectionModel != 0)
//	{
//		const QItemSelection toggledSelection = this->mSortFilter->swapRowSelection(
//				mSelectionModel->selection(), true);

//		mSelectionModel->setSelection(toggledSelection);
//	}

    //NMDebugCtx(__ctxsqltabview, << "done!");
}

void NMSqlTableView::normalise()
{
	// NOTE: for progress maxrange needs to be ncols * rows!

    NMDebugCtx(__ctxsqltabview, << "...");

	// -----------------------------------------------------------------------
	// get user input

	// get the names of the fields to normalise
	bool bOk = false;
	QString fieldNames = QInputDialog::getText(this, tr("Normalise Fields"),
	                                          tr("List of Field Names (separated by whitespace):"), QLineEdit::Normal,
	                                          tr(""), &bOk);
	if (!bOk || fieldNames.isEmpty())
	{
		NMDebugAI(<< "No input fields for normalisation specified!" << endl);
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

	// ask for the criterion type
	QStringList slModes;
	slModes.append(tr("Cost Criterion"));
	slModes.append(tr("Benefit Criterion"));

	QString sMode = QInputDialog::getItem(this, tr("Normalisation Mode"),
													 tr("Select mode"),
													 slModes, 0, false, &bOk, 0);
	if (!bOk)
	{
		NMDebugAI(<< "No normalisation mode specified!" << endl);
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}
	bool bCostCriterion = sMode == "Cost Criterion" ? true : false;

	// -----------------------------------------------------------------------

	// split the strings into a stringlist
	QStringList columnNames = fieldNames.split(tr(" "),
			QString::SkipEmptyParts, Qt::CaseInsensitive);

	const int ncols = columnNames.size();
    //const int maxrange = mlNumSelRecs ? mlNumSelRecs * ncols : mSortFilter->sourceRowCount() * ncols;
    //const int maxrange = mlNumSelRecs ? mlNumSelRecs * ncols : mSortFilter->rowCount() * ncols;
    const int maxrange = mlNumSelRecs ? mlNumSelRecs * ncols : mlNumRecs * ncols;

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(mModel));
	calc->setRowFilter(mSelectionModel->selection());
    //calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));

	prepareProgressDlg(calc.data(), "Normalise Columns ...", maxrange);
	QStringList normCols = calc->normaliseColumns(columnNames, bCostCriterion);
	cleanupProgressDlg(calc.data(), maxrange);

	// since we've added columns, we want to make sure, that any selection is expanded to
	// those columns as well
	this->updateProxySelection(QItemSelection(), QItemSelection());

	// inform listeners
	//emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);

    NMDebugCtx(__ctxsqltabview, << "done!");
}


void NMSqlTableView::userQuery()
{
    NMDebugCtx(__ctxsqltabview, << "...");

    if (mModel == 0)
    {
        return;
    }

    // ask for the name and the type of the new data field
    bool bOk = false;
    QString queryTemplate = QString("select * from %1").arg(mModel->tableName());

    NMGlobalHelper helper;
    QString sqlStmt = helper.getMultiLineInput("Arbitrary SQL-SELECT Query",
                                               queryTemplate, this);
    if (sqlStmt.compare("0") == 0 || sqlStmt.isEmpty())
    {
        NMDebugCtx(__ctxsqltabview, << "done!");
        return;
    }

    this->processUserQuery("query", sqlStmt);
}

void
NMSqlTableView::processUserQuery(const QString &queryName, const QString &sql)
{
    // create a unique temp table visisble to the source model's
    // database connection
    QString tableName = QString("%1").arg(queryName);
    QStringList allTables = mModel->database().tables();
    while (allTables.contains(tableName))
    {
        tableName = QString("%1_%2").arg(queryName).arg(++mQueryCounter);
    }

    QString queryStr = QString("Create temp table %1 as %2").arg(tableName).arg(sql);
    QSqlQuery userQuery(mModel->database());
    if (!userQuery.exec(queryStr))
    {
        NMBoxErr("User Query", userQuery.lastError().text().toStdString() << std::endl);
        NMDebugCtx(__ctxsqltabview, << "done!");
        return;
    }
    ++mQueryCounter;

    NMSqlTableModel* restab = new NMSqlTableModel(this, mModel->database());
    restab->setTable(tableName);
    restab->select();

    NMSqlTableView *resview = new NMSqlTableView(restab, this->parentWidget());
    resview->setWindowFlags(Qt::Window);
    resview->setTitle(tableName);
    resview->show();

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::prepareProgressDlg(NMTableCalculator* obj, const QString& msg, int maxrange)
{
	if (maxrange == 0)
    {
        //maxrange = mlNumSelRecs == 0 ? this->mSortFilter->sourceRowCount() : mlNumSelRecs;
        //maxrange = mlNumSelRecs == 0 ? this->mSortFilter->rowCount() : mlNumSelRecs;
        maxrange = mlNumSelRecs == 0 ? mlNumRecs : mlNumSelRecs;
    }

	mProgressDialog->setWindowModality(Qt::WindowModal);
	mProgressDialog->setLabelText(msg);
	mProgressDialog->setRange(0, maxrange);
	connect(obj, SIGNAL(signalProgress(int)), mProgressDialog, SLOT(setValue(int)));
	connect(mProgressDialog, SIGNAL(canceled()), obj, SLOT(cancelRequested()));
}

void NMSqlTableView::cleanupProgressDlg(NMTableCalculator* obj, int maxrange)
{
	if (maxrange == 0)
    {
        //maxrange = mlNumSelRecs == 0 ? this->mSortFilter->sourceRowCount() : mlNumSelRecs;
        //maxrange = mlNumSelRecs == 0 ? this->mSortFilter->rowCount() : mlNumSelRecs;
        maxrange = mlNumSelRecs == 0 ? mlNumRecs : mlNumSelRecs;
    }

	mProgressDialog->setValue(maxrange);
	disconnect(obj, SIGNAL(signalProgress(int)), mProgressDialog, SLOT(setValue(int)));
	disconnect(mProgressDialog, SIGNAL(canceled()), obj, SLOT(cancelRequested()));
}

void NMSqlTableView::calcColumn()
{
    NMDebugCtx(__ctxsqltabview, << "...");

	// get user input

	QString label = QString(tr("%1 = ").arg(this->mLastClickedColumn));
	bool bOk = false;
	QString func = QInputDialog::getText(this, tr("Calculate Column"),
	                                          label, QLineEdit::Normal,
	                                          tr(""), &bOk);
	if (!bOk || func.isEmpty())
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

    QString whereClause = "";
    if (!mSortFilter->getFilter().isEmpty())
    {
        whereClause = QString("where %1").arg(mSortFilter->getFilter());
    }

    QString uStr = QString("update %1 set %2 = %3 %4")
                    .arg(mModel->tableName())
                    .arg(mLastClickedColumn)
                    .arg(func)
                    .arg(whereClause);

    QSqlQuery qUpdate(mModel->database());
    if (!qUpdate.exec(uStr))
    {
        NMBoxErr("Calculate Column", qUpdate.lastError().text().toStdString());
        NMDebugCtx(__ctxsqltabview, << "done!");
        return;
    }

    mModel->select();
    updateProxySelection(QItemSelection(), QItemSelection());



    //	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(this->mModel));
    //	prepareProgressDlg(calc.data(), QString("Calculate %1 ...").arg(mLastClickedColumn));
    //    //calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));
    //	calc->setResultColumn(this->mLastClickedColumn);

    //	if (this->mSelectionModel->selection().count() > 0)
    //		calc->setRowFilter(this->mSelectionModel->selection());

    //	try
    //	{
    //		calc->setFunction(func);
    //		if (!calc->calculate())
    //		{
    //            NMErr(__ctxsqltabview, << "Something went wrong!");
    //		}
    //	}
    //	catch (itk::ExceptionObject& err)
    //	{
    //		cleanupProgressDlg(calc.data());
    //		QString errmsg = QString(tr("%1: %2")).arg(err.GetLocation())
    //				      .arg(err.GetDescription());

    //        NMErr(__ctxsqltabview, << "Calculation failed!"
    //				<< errmsg.toStdString());

    //		QMessageBox::critical(this, "Table Calculation Error", errmsg);
    //	}
    //	cleanupProgressDlg(calc.data());
    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::updateSelectionAdmin(long numSel)
{
	this->mBtnClearSelection->setEnabled(numSel);
    this->mRecStatusLabel->setText(
            QString(tr("%1 of %2 records selected")).arg(numSel).arg(
                    mlNumRecs));
	mlNumSelRecs = numSel;

    if (mlNumSelRecs)
    {
        this->mBtnClearSelection->setEnabled(true);
        this->mBtnSwitchSelection->setEnabled(true);
        this->mChkSelectedRecsOnly->setEnabled(true);
    }
    else
    {
        this->mBtnClearSelection->setEnabled(false);
        this->mBtnSwitchSelection->setEnabled(false);
        this->mChkSelectedRecsOnly->setEnabled(false);
        this->mChkSelectedRecsOnly->setChecked(false);
    }
}

void
NMSqlTableView::updateSelectionAdmin(const QItemSelection& sel,
		const QItemSelection& desel)
{
    NMDebugCtx(__ctxsqltabview, << "...");
    if (this->mProxySelModel == 0)
    {
        NMDebugAI(<< "Haven't got any selectiom model set up!" << endl);
        this->updateSelectionAdmin(0);
        NMDebugCtx(__ctxsqltabview, << "done!");
        return;
    }

    const QItemSelection& proxSel = mProxySelModel->selection();

    long selcnt = 0;
    foreach(const QItemSelectionRange& range, proxSel)
    {
        selcnt += range.bottom() - range.top() + 1;
    }
    mlNumSelRecs = selcnt;
	NMDebugAI(<< "report " << selcnt << " selected records" << std::endl);
	this->updateSelectionAdmin(selcnt);
    NMDebugCtx(__ctxsqltabview, << "done!");
}

void NMSqlTableView::addColumn()
{
    NMDebugCtx(__ctxsqltabview, << "...");

	NMAddColumnDialog* dlg = new NMAddColumnDialog();
	int ret = 0;
	bool bok = true;
	QString name;
	QVariant::Type type = QVariant::Invalid;

	// regular expression which defines possible names for columns
	QRegExp nameRegExp("^[A-Za-z_]+[\\d\\w]*$", Qt::CaseInsensitive);
	int pos = -1;
	do {
		ret = dlg->exec();
		if (!ret)
			break;

		name = dlg->getColumnName();
		type = (QVariant::Type)dlg->getDataType();

		// check, whether name matches the regular expression
		// (i.e. is valid)
		pos = nameRegExp.indexIn(name);
		bok = pos >= 0 ? true : false;

		//		NMDebugAI(<< "pos: " << pos << endl);
		//		if (pos != -1)
		//		{
		//			NMDebugAI(<< "that's what I've got ..." << endl);
		//			foreach(const QString& s, nameRegExp.capturedTexts())
		//					NMDebugAI(<< s.toStdString() << endl);
		//		}

		if (	type == QVariant::Invalid
			||  name.isEmpty()
			|| !bok
		   )
		{
			QMessageBox msgBox;
			msgBox.setText(tr("Name invalid!"));
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.exec();
			NMDebugAI(<< "type '" << type << "' or name '" << name.toStdString() << "' is invalid!" << endl);
		}

		if (this->getColumnIndex(name) >= 0)
		{
			bok = false;
			QMessageBox msgBox;
			msgBox.setText(tr("Column already exists!"));
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.exec();
		}
	} while (!bok || name.isEmpty() || type < 0);

	if (!ret)
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

	delete dlg;

    //int ncols = this->mSortFilter->columnCount();
    int ncols = this->mModel->columnCount();

	NMDebugAI(<< "add column ? " << ret << endl);
	NMDebugAI(<< "name: " << name.toStdString() << endl);
	NMDebugAI(<< "type: " << type << endl);
	NMDebugAI(<< "ncols in tab: " << ncols << endl);

    //if (this->mSortFilter->insertColumns(0, type, QModelIndex()))

    if (mSortFilter->insertColumn(name, type))
	{
        //this->mSortFilter->setHeaderData(ncols, Qt::Horizontal, name, Qt::DisplayRole);
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void NMSqlTableView::deleteColumn()
{
	// ask the user whether he really wants to delete the current field
	QMessageBox msgBox;
	QString text = QString(tr("Delete Field '%1'?")).arg(this->mLastClickedColumn);
	msgBox.setText(text);
	//msgBox.setInformativeText("Do you want to save your changes?");
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret = msgBox.exec();

	if (ret == QMessageBox::Cancel)
		return;

	int col = this->getColumnIndex(this->mLastClickedColumn);
	if (col < 0)
		return;

    //if (this->mSortFilter->removeColumns(col, 1, QModelIndex()))
    if (this->mModel->removeColumns(col, 1, QModelIndex()))
	{
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}
}

void NMSqlTableView::joinAttributes()
{
    NMDebugCtx(__ctxsqltabview, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Select Source Attribute Table"), "~", tr("Delimited Text File (*.csv)"));
	if (fileName.isNull())
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

	vtkSmartPointer<vtkDelimitedTextReader> tabReader =
						vtkSmartPointer<vtkDelimitedTextReader>::New();

	tabReader->SetFileName(fileName.toStdString().c_str());
	tabReader->SetHaveHeaders(true);
	tabReader->DetectNumericColumnsOn();
    tabReader->SetTrimWhitespacePriorToNumericConversion(1);
	tabReader->SetFieldDelimiterCharacters(",\t");
	tabReader->Update();

	QScopedPointer<vtkQtTableModelAdapter> srcModel(new vtkQtTableModelAdapter);
	srcModel->setTable(tabReader->GetOutput());

	int numJoinCols = srcModel->columnCount(QModelIndex());
	NMDebugAI( << "Analyse CSV Table Structure ... " << endl);
	QStringList srcJoinFields;
	for (int i=0; i < numJoinCols; ++i)
	{
		QModelIndex idx = srcModel->index(0, i, QModelIndex());
		QVariant::Type type = srcModel->data(idx, Qt::DisplayRole).type();
		if (type != QVariant::Invalid)
		{
			srcJoinFields.append(srcModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
		}
	}

	int numTarCols = this->mModel->columnCount(QModelIndex());
	QStringList tarJoinFields;
	for (int i=0; i < numTarCols; ++i)
	{
		QModelIndex idx = this->mModel->index(0, i, QModelIndex());
		QVariant::Type type = this->mModel->data(idx, Qt::DisplayRole).type();
		if (type != QVariant::Invalid)
		{
			tarJoinFields.append(this->mModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
		}
	}

	// ask the user for semantically common fields
	bool bOk = false;
	QString tarFieldName = QInputDialog::getItem(this,
			tr("Select Target Join Field"), tr("Select Target Join Field"),
			tarJoinFields, 0, false, &bOk, 0);
	int tarJoinColIdx = tarJoinFields.indexOf(tarFieldName);
	if (!bOk || tarJoinColIdx < 0)
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}
	NMDebugAI(<< "target field name=" << tarFieldName.toStdString()
			  << " at index " << tarJoinColIdx << std::endl);


	QString srcFieldName = QInputDialog::getItem(this,
			tr("Select Source Join Field"), tr("Select Source Join Field"),
			srcJoinFields, 0, false, &bOk, 0);
	int srcJoinColIdx = srcJoinFields.indexOf(srcFieldName);
	if (!bOk || srcJoinColIdx < 0)
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}
	NMDebugAI(<< "source field name=" << srcFieldName.toStdString()
			  << " at index " << srcJoinColIdx << std::endl);

	this->appendAttributes(tarJoinColIdx, srcJoinColIdx, srcModel.data());

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::appendAttributes(const int tarJoinColIdx, const int srcJoinColIdx,
		QAbstractItemModel* srcTable)
{
    NMDebugCtx(__ctxsqltabview, << "...");

	QStringList allTarFields;
	for (int i=0; i < mModel->columnCount(QModelIndex()); ++i)
	{
		QString fN = mModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
		allTarFields.push_back(fN);
	}

	// we determine which columns to copy, and which name + index they have
	QRegExp nameRegExp("^[\"A-Za-z_]+[\\d\\w]*$", Qt::CaseInsensitive);

	NMDebugAI(<< "checking column names ... " << std::endl);
	QStringList copyNames;
	QList<int> copyIdx;
    QList<QVariant::Type> copyType;
	QStringList writeNames;
	for (int i=0; i < srcTable->columnCount(QModelIndex()); ++i)
	{
		QString name = srcTable->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
		QString writeName = name;
		if (i == srcJoinColIdx || allTarFields.contains(name))
		{
			continue;
		}
		//else if (allTarFields.contains(name, Qt::CaseInsensitive))
		//{
		//	// come up with a new name for the column to appended
		//	writeName = QString(tr("copy_%1")).arg(name);
		//}

		int pos = -1;
		pos = nameRegExp.indexIn(name);
		if (pos >= 0)
		{
			copyNames.push_back(name);
			copyIdx.push_back(i);
            const QModelIndex smi = srcTable->index(0, i);
            copyType.push_back(srcTable->data(smi).type());
			writeNames.push_back(writeName);
		}
	}

	NMDebugAI( << "adding new columns to target table ..." << std::endl);
	// create new output columns for join fields
	QList<int> writeIdx;
	long tarnumrows = mModel->rowCount(QModelIndex());
	for (int t=0; t < copyNames.size(); ++t)
	{
		QModelIndex srcidx = srcTable->index(0, copyIdx.at(t), QModelIndex());
		QVariant::Type type = srcTable->data(srcidx, Qt::DisplayRole).type();

		mModel->insertColumns(0, type, QModelIndex());
		int colnum = mModel->columnCount(QModelIndex());
		mModel->setHeaderData(colnum-1, Qt::Horizontal, QVariant(writeNames.at(t)), Qt::DisplayRole);
		writeIdx.push_back(colnum-1);
	}

	// DEBUG
	//NMDebugAI(<< "SOURCE TABLE 0 to 10 ..." << std::endl);
	//int nsrccols = srcTable->columnCount(QModelIndex());
	//for (int i=0; i < srcTable->rowCount(QModelIndex()) && i < 10; ++i)
	//{
	//	for (int c=nsrccols-4; c < nsrccols; ++c)
	//	{
	//		const QModelIndex ind = srcTable->index(i, c, QModelIndex());
	//		QString nam = srcTable->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
	//		QString val = srcTable->data(ind, Qt::DisplayRole).toString();
	//		NMDebug(<< "#" << i << " " << nam.toStdString().c_str() << "=" << val.toStdString().c_str() << " ");
	//	}
	//	NMDebug(<< std::endl);
	//}
    //
	NMDebugAI(<< "copying field contents ...." << std::endl);

	mProgressDialog->setWindowModality(Qt::WindowModal);
	mProgressDialog->setLabelText("Joining Attributes ...");
	mProgressDialog->setRange(0, tarnumrows);

	// copy field values
	//vtkAbstractArray* tarJoin = tar->GetColumnByName(tarJoinField.toStdString().c_str());
	//vtkAbstractArray* srcJoin = src->GetColumnByName(srcJoinField.toStdString().c_str());
	//long cnt = 0;

	// let's create a hash index into the src key column to
	// quickly find corresponding records
	QHash<QString, int> srchash;
	for (int sr=0; sr < srcTable->rowCount(QModelIndex()); ++sr)
	{
		const QModelIndex idx = srcTable->index(sr, srcJoinColIdx, QModelIndex());
		const QString key = srcTable->data(idx, Qt::DisplayRole).toString();
		srchash.insert(key, sr);
	}


	long srcnumrows = srcTable->rowCount(QModelIndex());
	long havefound = 0;
	for (long row=0; row < tarnumrows && !mProgressDialog->wasCanceled(); ++row)
	{
		const QModelIndex tarJoinIdx = mModel->index(row, tarJoinColIdx, QModelIndex());
		const QString sTarJoin = mModel->data(tarJoinIdx, Qt::DisplayRole).toString();//tarJoin->GetVariantValue(row);

		const int foundyou = srchash.value(sTarJoin, -1);
		if (foundyou >= 0)
		{
			// copy columns for current row
			for (int c=0; c < copyIdx.size(); ++c)
			{
				const QModelIndex srcIdx = srcTable->index(foundyou, copyIdx.at(c), QModelIndex());
				const QVariant srcVal = srcTable->data(srcIdx, Qt::DisplayRole);
                const QModelIndex tarIdx = mModel->index(row, writeIdx.at(c), QModelIndex());
                const QVariant::Type colType = copyType.at(c);

                switch(colType)
                {
                case QVariant::String:
                    // strip single quotes (may come from vtk CSV to vtkTable import)
                    {
                        const QString sVal = srcVal.toString();
                        if (    (    sVal.startsWith("'")
                                 &&  sVal.endsWith("'")
                                )
                            ||  (    sVal.startsWith("\"")
                                     &&  sVal.endsWith("\"")
                                )
                           )
                        {
                            QVariant vsVal = sVal.mid(1, sVal.size()-2);
                            mModel->setData(tarIdx, vsVal, Qt::DisplayRole);
                        }
                        else
                        {
                            mModel->setData(tarIdx, srcVal, Qt::DisplayRole);
                        }
                    }
                    break;

                default:
                    mModel->setData(tarIdx, srcVal, Qt::DisplayRole);
                }
			}
			++havefound;
		}

		mProgressDialog->setValue(row+1);
	}

	NMDebugAI(<< "did find " << havefound << " matching recs!" << std::endl);
    //this->mSortFilter->notifyLayoutUpdate();
    //mSortFilter->layoutChanged();
    mModel->layoutChanged();

    NMDebugCtx(__ctxsqltabview, << "done!");
}

//void NMSqlTableView::hideRow(int row)
//{
//    //if (row < 0 || row > this->mModel->rowCount()-1)
//    if (row < 0 || row > mlNumRecs - 1)
//		return;

//	this->mTableView->hideRow(row);

//}

void NMSqlTableView::exportTable()
{
    NMDebugCtx(__ctxsqltabview, << "...");

	QString tabName = this->windowTitle().split(" ", QString::SkipEmptyParts).last();
	QString proposedName = QString(tr("%1/%2.csv")).arg(getenv("HOME")).arg(tabName);

	// take the first layer and save as vtkpolydata
	QString selectedFilter = tr("Delimited Text (*.csv)");
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Table"), proposedName,
			tr("Delimited Text (*.csv)"), //;;SQLite Database (*.sqlite *.sdb *.db)"),
			&selectedFilter);
	if (fileName.isNull())
	{
		NMDebugAI( << "got an empty filename from the user!" << endl);
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

	QStringList fnList = fileName.split(tr("."), QString::SkipEmptyParts);

	QString suffix = fnList.last();
	if (suffix.compare(tr("txt"), Qt::CaseInsensitive) == 0 ||
		suffix.compare(tr("csv"), Qt::CaseInsensitive) == 0)
	{
		NMDebugAI(<< "write delimited text to " << fileName.toStdString() << endl);
		this->writeDelimTxt(fileName, false);
	}
	//else if (suffix.compare(tr("sqlite"), Qt::CaseInsensitive) == 0 ||
	//		 suffix.compare(tr("sdb"), Qt::CaseInsensitive) == 0 ||
	//		 suffix.compare(tr("db"), Qt::CaseInsensitive) == 0)
	//{
	//	QString dbName;
	//	QString tableName;
	//	if (fnList.size() == 3)
	//	{
	//		dbName = QString(tr("%1.%2")).arg(fnList.first()).arg(fnList.last());
	//		tableName = fnList.at(1);
	//		NMDebugAI(<< "insert table '" << tableName.toStdString() << "' "
	//				  << "into database '" << dbName.toStdString() << "'" << endl);
	//	}
	//	else
	//	{
    //		NMErr(__ctxsqltabview, << "invalid database and table name!");
	//		return;
	//	}
    //
	//	this->writeSqliteDb(dbName, tableName, false);
	//}

    NMDebugCtx(__ctxsqltabview, << "done!");
}
void NMSqlTableView::colStats()
{
    NMDebugCtx(__ctxsqltabview, << "...");

    if (mLastClickedColumn.isEmpty() || mModel == 0)
    {
        return;
    }

    QString name = QString("%1_stats").arg(mLastClickedColumn);

    std::string col = mLastClickedColumn.toStdString();
    std::stringstream sql;

    sql << "select min(" << col << ") as minimum, "
        << "max(" << col << ") as maximum, "
        << "avg(" << col << ") as mean, "
        << "sum(" << col << ") as sum from "
        << mModel->tableName().toStdString();

    if (mSortFilter->getSelCount())
    {
        sql << " where " << mSortFilter->getFilter().toStdString();
    }

    this->processUserQuery(name, QString(sql.str().c_str()));

    //	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(mModel));
    //    //const int maxrange = mlNumSelRecs ? mlNumSelRecs*2 : this->mSortFilter->sourceRowCount()*2;
    //    //const int maxrange = mlNumSelRecs ? mlNumSelRecs*2 : this->mSortFilter->rowCount()*2;
    //    const int maxrange = mlNumSelRecs ? mlNumSelRecs*2 : mlNumRecs*2;
    //	prepareProgressDlg(calc.data(), "Calculate Column Statistics ...", maxrange);

    //    //calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));
    //	calc->setRowFilter(mSelectionModel->selection());
    //	std::vector<double> stats = calc->calcColumnStats(this->mLastClickedColumn);

    //	if (stats.size() < 4)
    //	{
    //		NMDebugAI(<< "Couldn't calc stats for a reason!" << std::endl);
    //        NMDebugCtx(__ctxsqltabview, << "done!");
    //		return;
    //	}
    //	cleanupProgressDlg(calc.data(), maxrange);

    //	// min, max, mean, std. dev, sum
    //	QString smin  = QString("%1").arg(stats[0], 0, 'f', 4); //smin  = smin .rightJustified(15, ' ');
    //	QString smax  = QString("%1").arg(stats[1], 0, 'f', 4); //smax  = smax .rightJustified(15, ' ');
    //	QString smean = QString("%1").arg(stats[2], 0, 'f', 4); //smean = smean.rightJustified(15, ' ');
    //	QString smedi = QString("%1").arg(stats[3], 0, 'f', 4);
    //	QString ssdev = QString("%1").arg(stats[4], 0, 'f', 4); //ssdev = ssdev.rightJustified(15, ' ');
    //	QString ssum  = QString("%1").arg(stats[6], 0, 'f', 4); //ssum  = ssum .rightJustified(15, ' ');
    //	QString ssample = QString("Sample Size: %1").arg(stats[5]);

    //	QString strmin  ("Minimum: ");  //strmin  = strmin .rightJustified(10, ' ');
    //	QString strmax  ("Maximum: ");  //strmax  = strmax .rightJustified(10, ' ');
    //	QString strmean ("Mean: ");     //strmean = strmean.rightJustified(10, ' ');
    //	QString strmedi ("Median: ");
    //	QString strsdev ("Std.Dev.: "); //strsdev = strsdev.rightJustified(10, ' ');
    //	QString strsum  ("Sum: ");      //strsum  = strsum .rightJustified(10, ' ');


    //	QString res = QString("%1%2\n%3%4\n%5%6\n%7%8\n%9%10\n%11%12\n%13")
    //			.arg(strmin).arg(smin)
    //			.arg(strmax).arg(smax)
    //			.arg(strmean).arg(smean)
    //			.arg(strmedi).arg(smedi)
    //			.arg(strsdev).arg(ssdev)
    //			.arg(strsum).arg(ssum)
    //			.arg(ssample);

    //	QString title = QString(tr("%1")).arg(this->mLastClickedColumn);

    //	QMessageBox::information(this, title, res);

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::loadRasLayer(void)
{
	int srcRow = this->mlLastClickedRow;
//	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
//	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);
    QModelIndex sortIdx = this->mModel->index(this->mlLastClickedRow, 0, QModelIndex());
    QModelIndex srcIdx = sortIdx;
	srcRow = srcIdx.row();

	int oid_coll = this->getColumnIndex("oid");
	int coll_id = this->getColumnIndex("coll_name");
	int cov_id = this->getColumnIndex("covername");

	QModelIndex oididx = this->mModel->index(srcRow, oid_coll, QModelIndex());
	QModelIndex collidx = this->mModel->index(srcRow, coll_id, QModelIndex());
	QModelIndex covidx = this->mModel->index(srcRow, cov_id, QModelIndex());

	bool bok;
	long oid = (long)this->mModel->data(oididx, Qt::DisplayRole).toInt(&bok);
	QString coll = this->mModel->data(collidx, Qt::DisplayRole).toString();
	QString covname = this->mModel->data(covidx, Qt::DisplayRole).toString();

	QString imagespec = QString("%1:%2").arg(coll).arg(oid);

	emit notifyLoadRasLayer(imagespec, covname);
}

void
NMSqlTableView::deleteRasLayer(void)
{
	int srcRow = this->mlLastClickedRow;
//	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
//	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);
    QModelIndex sortIdx = this->mModel->index(this->mlLastClickedRow, 0, QModelIndex());
    QModelIndex srcIdx = sortIdx;
	srcRow = srcIdx.row();

	int oid_coll = this->getColumnIndex("oid");
	int coll_id = this->getColumnIndex("coll_name");

	QModelIndex oididx = this->mModel->index(srcRow, oid_coll, QModelIndex());
	QModelIndex collidx = this->mModel->index(srcRow, coll_id, QModelIndex());

	bool bok;
	long oid = (long)this->mModel->data(oididx, Qt::DisplayRole).toInt(&bok);
	QString coll = this->mModel->data(collidx, Qt::DisplayRole).toString();
	QString imagespec = QString("%1:%2").arg(coll).arg(oid);

	emit notifyDeleteRasLayer(imagespec);
}


bool
NMSqlTableView::writeDelimTxt(const QString& fileName,
		bool bselectedRecs)
{
    NMDebugCtx(__ctxsqltabview, << "...");

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		NMBoxErr("Export Table", "Couldn't create file '" << fileName.toStdString() << "'!");
        NMDebugCtx(__ctxsqltabview, << "done!");
		return false;
	}
	QTextStream out(&file);

    QString whereClause = "";
    // get either the selected or all records
    int maxrange = mlNumRecs;
    if (mSortFilter->getSelCount() > 0)
    {
        maxrange = mSortFilter->getSelCount();
        whereClause = QString("where %1").arg(mSortFilter->getFilter());
    }

    QString qStr = QString("select * from %1 %2")
                .arg(mModel->tableName())
                .arg(whereClause);
    QSqlQuery qTable(mModel->database());
    if (!qTable.exec(qStr))
    {
        NMBoxErr("Export Table", qTable.lastError().text().toStdString());
        file.close();
        return false;
    }

	const int ncols = mModel->columnCount(QModelIndex());

    mProgressDialog->setWindowModality(Qt::WindowModal);
    mProgressDialog->setLabelText("Export Table ...");
    mProgressDialog->setRange(0, maxrange);

	// write header first
    int pkIdx = -1;
    long progress = 0;
	for (int col=0; col < ncols; ++col)
	{
		QString cN = mModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
		out << "\"" << cN << "\"";
		if (col < ncols-1)
			out << ",";
	}
	out << endl;
    out.setRealNumberNotation(QTextStream::SmartNotation);

    //int progInterval = maxrange / 50;
    while (qTable.next())
    {
        for (int col=0; col < ncols; ++col)
        {
            QVariant val = qTable.value(col);
            if (val.type() == QVariant::String)
            {
                out << "'" << val.toString() << "'";
            }
            else
            {
                out << val.toString();
            }

            if (col < ncols-1)
                out << ",";
        }
        out << "\n";
        ++progress;
        mProgressDialog->setValue(progress);
        if (progress % 1000 == 0)
        {
            out.flush();
        }
    }

    mProgressDialog->setValue(maxrange);
    out.flush();
	file.close();
    NMDebugCtx(__ctxsqltabview, << "done!");
	return true;
}



int NMSqlTableView::getColumnIndex(const QString& attr)
{
	int attrCol = -1;

	int ncols = this->mModel->columnCount(QModelIndex());

	QString colname;
	for (int c=0; c < ncols; ++c)
	{
		colname = this->mModel->headerData(c, Qt::Horizontal).toString();
		if (colname.compare(attr, Qt::CaseInsensitive) == 0)
			return c;
	}

	return attrCol;
}

void
NMSqlTableView::callHideColumn(void)
{
	this->hideAttribute(this->mLastClickedColumn);
}

void
NMSqlTableView::callUnHideColumn(void)
{
	if (this->mHiddenColumns.size() == 0)
		return;

	// show list of columns which can be unhidden
	QInputDialog dlg(this);
	dlg.setWindowTitle("Unhide Table Column");
	dlg.setLabelText(QString(tr("Pick the column to unhide")));
	dlg.setOptions(QInputDialog::UseListViewForComboBoxItems);
	dlg.setComboBoxItems(this->mHiddenColumns);

	int ret = dlg.exec();
	if (!ret)
		return;

	QString colname = dlg.textValue();
	this->unhideAttribute(colname);

}

void NMSqlTableView::hideAttribute(const QString& attr)
{
	int colidx = this->getColumnIndex(attr);
	if (colidx == -1)
	{
		NMDebugAI(<< "Cannot hide '"
				<< attr.toStdString() << "'! "
				<< "It doesn't seem to be part of the table!"
				<< std::endl);
		return;
	}

	if (   attr.compare("nm_id")   != 0
		&& attr.compare("nm_hole") != 0
		&& attr.compare("nm_sel")  != 0
		&& !this->mHiddenColumns.contains(attr, Qt::CaseInsensitive))
	{
		this->mHiddenColumns.append(attr);
	}

	if (colidx >= 0)
		this->mTableView->hideColumn(colidx);
}

void
NMSqlTableView::unhideAttribute(const QString& attr)
{
	int colidx = this->getColumnIndex(attr);
	if (colidx == -1)
	{
		NMDebugAI(<< "Cannot unhide '"
				<< attr.toStdString() << "'! "
				<< "It doesn't seem to be part of the table!"
				<< std::endl);
		return;
	}

	if (this->mHiddenColumns.contains(attr, Qt::CaseInsensitive))
		this->mHiddenColumns.removeOne(attr);
	this->mTableView->showColumn(colidx);
}

bool NMSqlTableView::eventFilter(QObject* object, QEvent* event)
{
	// ======================== COLUMN HEADER + MOUSE BUTTON ==================================================
	if (	object == this->mTableView->horizontalHeader()->viewport()
		&&	event->type() == QEvent::MouseButtonPress
		&&  event->type() != QEvent::MouseMove)
	{
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
		int xpos = me->pos().x();
		// --------------------- CHECK FOR VALID COLUMN CLICKED --------------------------------------
		// if we haven't got a proper column on the hook, we bail out
		int col = this->mTableView->columnAt(xpos);
		if (col < 0)
		{
			this->mLastClickedColumn.clear();
			return false;
		}

		// -------------------- BAIL OUT IF WE're HOVERING ABOVE A COLUMN SEPARATOR ---------------------
		if (	(col != mTableView->columnAt(xpos -5))
			||  (col != mTableView->columnAt(xpos +5))
		   )
		{
			return false;
		}

		// ---------------------- MEMORISE CURRENTLY CLICKED COLUMN -------------------------------------
//		this->mLastClickedColumn = this->mSortFilter->
//				headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
        this->mLastClickedColumn = this->mModel->
                headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();

		// let's see what kind of mouse button we've got
		// --------------------- RIGHT BUTTON calls CONTEXT MENU ----------------------------------------
		if (me->button() == Qt::RightButton)
		{
			this->mColHeadMenu->move(me->globalPos());
			this->mColHeadMenu->exec();
		}
		// --------------------- LEFT BUTTON SORTS THE COLUMN ---------------------------------------------
		else if (me->button() == Qt::LeftButton)
		{
			NMDebugAI(<< "SROTING COLUMN #" << col << "..." << std::endl);
			this->sortColumn(col);
		}
		return false;
	}
	// ============================== ROW HEADER AND MOUSE BUTTON ================================================
	else if (	object == this->mTableView->verticalHeader()->viewport()
			 && event->type() == QEvent::MouseButtonPress
			 && event->type() != QEvent::MouseMove
			 )
	{
		QMouseEvent* me = static_cast<QMouseEvent*>(event);
		if (me->button() == Qt::LeftButton)
		{
			int row = this->mTableView->rowAt(me->pos().y());
			this->mlLastClickedRow = row;
            if (row != -1 && mbIsSelectable)
			{
//                QModelIndex indx = this->mSortFilter->index(row, 0, QModelIndex());
//                QModelIndex srcIndx = this->mSortFilter->mapToSource(indx);
//                long srcRow = srcIndx.row();
                //if (this->mbIsSelectable)
				{
                    this->toggleRow(row);
				}
//                emit notifyLastClickedRow(srcRow);
			}
		}
		return true;
	}
	// ================================ VIEWPORT  ==============================================
	else if (   (   object == this->mTableView->viewport()
			     && event->type() == QEvent::MouseButtonPress)
			 || (   object == this->mTableView->viewport()
				 && event->type() == QEvent::MouseButtonDblClick)
			 )
	{
		QMouseEvent* me = static_cast<QMouseEvent*>(event);

		// ----------------------- RIGHT BUTTON CALLS META MENU ------------------------------------------------
		if (me->button() == Qt::RightButton)
		{
                        if (this->mViewMode == NMSqlTableView::NMTABVIEW_RASMETADATA)
			{
				int row = this->mTableView->rowAt(me->pos().y());
				if (!row)
					return true;

				this->mlLastClickedRow = row;
//				const QModelIndex ridx = mSortFilter->index(row, 0, QModelIndex());
//				const int srcRow = mSortFilter->mapToSource(ridx).row();
                const QModelIndex ridx = mModel->index(row, 0, QModelIndex());
                const int srcRow = ridx.row();
				emit notifyLastClickedRow((long)srcRow);
				this->mManageLayerMenu->move(me->globalPos());
				this->mManageLayerMenu->exec();
			}
			return true;
		}
		// ------------------------LEFT BUTTON DBL CLICK EDITS THE CELL ----------------------------------------
		else if (	me->button() == Qt::LeftButton
				 && event->type() == QEvent::MouseButtonDblClick)
		{
            if (this->mViewMode == NMSqlTableView::NMTABVIEW_ATTRTABLE)
			{
				int row = this->mTableView->rowAt(me->pos().y());
				int col = this->mTableView->columnAt(me->pos().x());
				if (row && col)
				{
                    QModelIndex idx = this->mSortFilter->index(row, col, QModelIndex());
                    //QModelIndex idx = this->mModel->index(row, col, QModelIndex());
					this->mTableView->edit(idx);
				}
			}
			return true;
		}
		else
		{
			// we filter anything else out
			return true;
		}
	}

	return false;
}

void
NMSqlTableView::sortColumn(int col)
{
    NMDebugCtx(__ctxsqltabview, << "...");

    Qt::SortOrder order;
    QString orderStr;
	QMap<int, bool>::iterator it = mMapColSortAsc.find(col);
	if (it != mMapColSortAsc.end())
	{
		if (it.value())
		{
			order = Qt::DescendingOrder;
            orderStr = "Descending";
			mMapColSortAsc.insert(col, false);
		}
		else
		{
			order = Qt::AscendingOrder;
            orderStr = "Ascending";
			mMapColSortAsc.insert(col, true);
		}
	}
	else
	{
		order = Qt::AscendingOrder;
        orderStr = "Ascending";
		mMapColSortAsc.insert(col, true);
	}
	this->mTableView->horizontalHeader()->setSortIndicator(col, order);
	this->mTableView->horizontalHeader()->setSortIndicatorShown(true);

    //NMDebugAI(<< "... actually sorting the column" << std::endl);
    mSortFilter->sort(col, order);
    updateProxySelection(QItemSelection(), QItemSelection());


    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::selectionQuery(void)
{
    NMDebugCtx(__ctxsqltabview, << "...");

	bool bOk = false;
    QString queryStr = QInputDialog::getText(this, tr("Selection Query"),
                         tr("Where Clause"), QLineEdit::Normal,
                         QString("%1 = ").arg(this->mLastClickedColumn),
                         &bOk);
    if (!bOk || queryStr.isEmpty())
	{
        NMDebugCtx(__ctxsqltabview, << "done!");
		return;
	}

    //    if (!mCurrentQuery.isEmpty())
    //    {
    //        mCurrentQuery += QString(" AND %1").arg(mCurrentQuery);
    //    }
    //    else
    {
        mCurrentQuery = queryStr;
    }

    mCurrentSwapQuery = QString("NOT (%1)").arg(queryStr);

    // this clears any manually selected rows ...
    mPickedRows.clear();

    mbSwitchSelection = false;
    this->updateSelection(false);

    //NMDebugAI(<< cnt << " selected rows" << std::endl);

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::updateSelection(bool swap)
{
    // we build the final query from the
    // user's selection query (comprising the entire history of
    // seletion queries since the last selection clearance;

    QString queryStr = mCurrentQuery.simplified();
    if (swap)
    {
        queryStr = mCurrentSwapQuery.simplified();
    }


    if (!mBaseFilter.isEmpty())
    {
        if (!queryStr.isEmpty())
        {
            queryStr += QString(" AND %1").arg(mBaseFilter.simplified());
        }
        else
        {
            queryStr = mBaseFilter.simplified();
        }
    }


    if (!mPickedRows.isEmpty())
    {
        QString handPicked = QString("%1 in (").arg(mPrimaryKey);
        if (swap)
        {
            handPicked = QString("%1 not in (").arg(mPrimaryKey);
        }
        for (int r=0; r < mPickedRows.size(); ++r)
        {
            handPicked += QString("%1").arg(mPickedRows.at(r));
            if (r < mPickedRows.size() - 1)
            {
                handPicked += ",";
            }
        }
        handPicked += ")";

        if (!queryStr.isEmpty())
        {
            queryStr += QString(" OR %1").arg(handPicked);
        }
        else
        {
            queryStr = handPicked;
        }
    }

    NMGlobalHelper h;
    h.startBusy();
    if (mChkSelectedRecsOnly->isChecked())
    {
        mProxySelModel->clearSelection();
        mSortFilter->selectRows(queryStr, true);
        this->updateSelectionAdmin(mSortFilter->getSelCount());
    }
    else
    {
        mSortFilter->selectRows(queryStr, false);
        mProxySelModel->setSelection(mSortFilter->getProxySelection());
        this->updateSelectionAdmin(mSortFilter->getSelCount());
    }

    if (mSortFilter->getSelCount() == 0 && !mCurrentQuery.isEmpty())
    {
        this->clearSelection();
    }
    h.endBusy();
}

void NMSqlTableView::clearSelection()
{
    mSortFilter->clearSelection();
    mProxySelModel->clearSelection();
    updateSelectionAdmin(0);
    mTableView->reset();
    mCurrentQuery.clear();
    mCurrentSwapQuery.clear();
    mPickedRows.clear();
    mChkSelectedRecsOnly->setChecked(false);
}

void
NMSqlTableView::updateProxySelection(const QItemSelection& sel, const QItemSelection& desel)
{
    NMDebugCtx(__ctxsqltabview, << "...");

    if (mSelectionModel && sel.count() > 0)
    {
        mCurrentQuery.clear();
        mCurrentSwapQuery.clear();
        mbSwitchSelection = false;
        mPickedRows.clear();
        foreach(const QItemSelectionRange& range, sel)
        {
            for (int r=range.top(); r <= range.bottom(); ++r)
            {
                mPickedRows << r;
            }
        }
        this->updateSelection(false);
    }
    else
    {
        this->mProxySelModel->setSelection(mSortFilter->getProxySelection());
    }

    this->printSelRanges(this->mProxySelModel->selection(), "new selection ...");

    NMDebugCtx(__ctxsqltabview, << "done!");
}

void
NMSqlTableView::procRowsInserted(QModelIndex parent, int first, int last)
{
    if (!this->mChkSelectedRecsOnly->isChecked())
    {
        this->updateProxySelection(QItemSelection(), QItemSelection());
    }
}

void
NMSqlTableView::printSelRanges(const QItemSelection& selection, const QString& msg)
{
	int total = selection.count();
	NMDebugAI(<< msg.toStdString() << std::endl);
	int rcnt = 1;
	int numidx = 0;
	foreach(const QItemSelectionRange& range, selection)
	{
		NMDebugAI(<< "   range #" << rcnt << ":  " << range.width()
				                          << " x " << range.height() << std::endl);
		NMDebugAI(<< "     rows: " << range.top() << " - " << range.bottom() << std::endl);
		++rcnt;
		numidx += range.bottom() - range.top() + 1;
	}
	NMDebugAI(<< numidx << " selected rows in total" << std::endl);
}


void
NMSqlTableView::showEvent(QShowEvent* event)
{
	NMDebugAI(<< __FUNCTION__ << ": updating selection? ..." << std::endl);
	this->updateSelectionAdmin(QItemSelection(), QItemSelection());
}

void
NMSqlTableView::toggleRow(int row)
{
    // don't do any row toggling when 'selected records only'
    // mode is turned on
    // note that this mode can be turned on mandatorily when
    // then number of records gets to big to to handle the
    // queying overhead involved with that!
    if (this->mChkSelectedRecsOnly->isChecked())
    {
        return;
    }

    QModelIndex proxyIdx = mSortFilter->index(row, 0);
    QModelIndex srcIdx = mSortFilter->mapToSource(proxyIdx);
    if (!srcIdx.isValid())
    {
        return;
    }

    mCurrentQuery.clear();
    mCurrentSwapQuery.clear();
    mbSwitchSelection = false;

    int srcRow = srcIdx.row();
    int idx = mPickedRows.indexOf(srcRow);
    if (idx == -1)
    {
        mPickedRows << srcRow;
    }
    else
    {
        mPickedRows.removeAt(idx);
    }
    emit notifyLastClickedRow(srcRow);
    updateSelection(false);
}


