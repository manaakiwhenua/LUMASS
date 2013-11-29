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
 * NMTableView.cxx
 *
 *  Created on: 22/09/2011
 *      Author: alex
 */

#include <string>
#include <iostream>
#include <vector>
#include <limits>

#include "NMMacros.h"
#include "NMTableView.h"
#include "NMAddColumnDialog.h"
#include "NMTableCalculator.h"

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

//#include "valgrind/callgrind.h"


NMTableView::NMTableView(QAbstractItemModel* model, QWidget* parent)
	: QWidget(parent), mViewMode(NMTABVIEW_ATTRTABLE),
	  mModel(model), mbSwitchSelection(false), mbClearSelection(false)
{
	this->mTableView = new QTableView(this);
	this->initView();

	this->mSortFilter = new NMSelectableSortFilterProxyModel(this);
	this->mSortFilter->setDynamicSortFilter(true);
	this->mSortFilter->setSourceModel(mModel);

	this->mTableView->setModel(this->mSortFilter);
	this->mProxySelModel = new NMFastTrackSelectionModel(mSortFilter, this);
	this->mProxySelModel->setObjectName("FastProxySelection");
	this->mTableView->setSelectionModel(mProxySelModel);

	//this->mDeletedColumns.clear();
	//this->mAlteredColumns.clear();
	//this->mMapColSortAsc.clear();
}

NMTableView::NMTableView(QAbstractItemModel* model, ViewMode mode, QWidget* parent)
	: QWidget(parent), mViewMode(mode),
	  mModel(model), mbSwitchSelection(false), mbClearSelection(false)
{
	this->mTableView = new QTableView(this);
	this->initView();

	this->mSortFilter = new NMSelectableSortFilterProxyModel(this);
	this->mSortFilter->setDynamicSortFilter(true);
	this->mSortFilter->setSourceModel(mModel);

	this->mTableView->setModel(this->mSortFilter);
	this->mProxySelModel = new NMFastTrackSelectionModel(mSortFilter, this);
	this->mProxySelModel->setObjectName("FastProxySelection");
	this->mTableView->setSelectionModel(mProxySelModel);

	//this->mDeletedColumns.clear();
	//this->mAlteredColumns.clear();
	//this->mMapColSortAsc.clear();
}


NMTableView::~NMTableView()
{
}

void NMTableView::initView()
{
	this->mTableView->setCornerButtonEnabled(false);
	this->mTableView->setAlternatingRowColors(true);
	this->mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

	this->setWindowFlags(Qt::Window);
	this->setWindowTitle(tr("Attributes"));

	//this->mDeletedColumns.clear();
	//this->mAlteredColumns.clear();
	this->mHiddenColumns.clear();

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

	// ---------------- NMTableView - Layout ------------------------------

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

	//QAction* actFilter = new QAction(this->mColHeadMenu);
	//actFilter->setText(tr("Arbitrary SQL Query ..."));

	QAction* actSel = new QAction(this->mColHeadMenu);
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
	//this->mColHeadMenu->addAction(actFilter);
	this->mColHeadMenu->addSeparator();
	this->mColHeadMenu->addAction(actStat);

	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		this->mColHeadMenu->addAction(actCalc);
		this->mColHeadMenu->addAction(actNorm);
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
	//this->connect(actFilter, SIGNAL(triggered()), this, SLOT(userQuery()));
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
NMTableView::hideSource(const QList<int>& rows)
{
	this->mSortFilter->hideSource(rows);
	this->updateSelectionAdmin(QItemSelection(), QItemSelection());
}

void
NMTableView::connectSelModels(bool bconnect)
{
	if (bconnect)
	{
		connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
				                                         const QItemSelection &)),
				this, SLOT(updateSelectionAdmin(const QItemSelection &,
						                        const QItemSelection &)));

		connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
				                                         const QItemSelection &)),
				this, SLOT(updateProxySelection(const QItemSelection &,
						                        const QItemSelection &)));
	}
	else
	{
		disconnect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
				                                         const QItemSelection &)),
				this, SLOT(updateSelectionAdmin(const QItemSelection &,
						                        const QItemSelection &)));

		disconnect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
				                                         const QItemSelection &)),
				this, SLOT(updateProxySelection(const QItemSelection &,
						                        const QItemSelection &)));
	}
}


void
//NMTableView::setSelectionModel(QItemSelectionModel* selectionModel)
NMTableView::setSelectionModel(NMFastTrackSelectionModel* selectionModel)
{
	this->mSelectionModel = selectionModel;
	if (this->mSelectionModel)
	{
		mSelectionModel->setObjectName("FastSourceSelection");
		const QItemSelection proxySelection = this->mSortFilter->mapRowSelectionFromSource(
				this->mSelectionModel->selection(), false);
		mProxySelModel->setSelection(proxySelection);

		this->updateSelectionAdmin(QItemSelection(), QItemSelection());

		this->connectSelModels(true);
	}
}

void
NMTableView::setViewMode(ViewMode mode)
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

void NMTableView::updateSelRecsOnly(int state)
{
	NMDebugCtx(__ctxtabview, << "...");
	if (state == Qt::Checked)
	{
		this->mSortFilter->setFilterOn(false);
		this->mProxySelModel->clearSelection();
		this->mSortFilter->clearFilter();
		const QItemSelection srcSel = this->mSelectionModel->selection();
		const QItemSelection proxySel = this->mSortFilter->mapRowSelectionFromSource(srcSel);
		this->mSortFilter->addToFilter(proxySel);
		this->mSortFilter->setFilterOn(true);
	}
	else
	{
		this->mSortFilter->setFilterOn(false);
		this->mSortFilter->clearFilter();
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}
	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::switchSelection()
{
	NMDebugCtx(__ctxtabview, << "...");

	mbSwitchSelection = true;

	if (mSelectionModel != 0)
	{
		const QItemSelection toggledSelection = this->mSortFilter->swapRowSelection(
				mSelectionModel->selection(), true);

		mSelectionModel->setSelection(toggledSelection);
	}

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::normalise()
{
	// NOTE: for progress maxrange needs to be ncols * rows!

	NMDebugCtx(__ctxtabview, << "...");

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
		NMDebugCtx(__ctxtabview, << "done!");
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
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}
	bool bCostCriterion = sMode == "Cost Criterion" ? true : false;

	// -----------------------------------------------------------------------

	// split the strings into a stringlist
	QStringList columnNames = fieldNames.split(tr(" "),
			QString::SkipEmptyParts, Qt::CaseInsensitive);

	const int ncols = columnNames.size();
	const int maxrange = mlNumSelRecs ? mlNumSelRecs * ncols : mSortFilter->sourceRowCount() * ncols;

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(mModel));
	calc->setRowFilter(mSelectionModel->selection());
	calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));

	prepareProgressDlg(calc.data(), "Normalise Columns ...", maxrange);
	QStringList normCols = calc->normaliseColumns(columnNames, bCostCriterion);
	cleanupProgressDlg(calc.data(), maxrange);

	// since we've added columns, we want to make sure, that any selection is expanded to
	// those columns as well
	this->updateProxySelection(QItemSelection(), QItemSelection());

	// inform listeners
	//emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);

	NMDebugCtx(__ctxtabview, << "done!");
}


//void NMTableView::userQuery()
//{
//	//NMDebugCtx(__ctxtabview, << "...");
//	//// ask for the name and the type of the new data field
//	//bool bOk = false;
//	//QString sqlStmt = QInputDialog::getText(this, tr("Filter Attributes"),
//	//                                          tr("SQL Query (use 'memtab1' as table required!)"), QLineEdit::Normal,
//	//                                          tr("Select * from memtab1 where "), &bOk);
//	//if (!bOk || sqlStmt.isEmpty())
//	//{
//	//	NMDebugCtx(__ctxtabview, << "done!");
//	//	return;
//	//}
//    //
//    ////QString sqlStmt = QString(tr("%1")).arg(whereclause);
//	//vtkSmartPointer<vtkTable> restab = this->queryTable(sqlStmt);
//	//if (restab == 0)
//	//{
//	//	NMDebugAI( << "got an empty result table (i.e. NULL)" << endl);
//	//	NMDebugCtx(__ctxtabview, << "done!");
//	//	return;
//	//}
//    //
//	////this->mVtkTableAdapter->setTable(restab);
//    //
//	//NMTableView *resview = new NMTableView(restab, this->parentWidget());
//	//resview->setWindowFlags(Qt::Window);
//	//resview->setTitle(tr("Query Result"));
//	//resview->show();
//
//	NMDebugCtx(__ctxtabview, << "done!");
//}

void
NMTableView::prepareProgressDlg(NMTableCalculator* obj, const QString& msg, int maxrange)
{
	if (maxrange == 0)
		maxrange = mlNumSelRecs == 0 ? this->mSortFilter->sourceRowCount() : mlNumSelRecs;

	mProgressDialog->setWindowModality(Qt::WindowModal);
	mProgressDialog->setLabelText(msg);
	mProgressDialog->setRange(0, maxrange);
	connect(obj, SIGNAL(signalProgress(int)), mProgressDialog, SLOT(setValue(int)));
	connect(mProgressDialog, SIGNAL(canceled()), obj, SLOT(cancelRequested()));
}

void NMTableView::cleanupProgressDlg(NMTableCalculator* obj, int maxrange)
{
	if (maxrange == 0)
		maxrange = mlNumSelRecs == 0 ? this->mSortFilter->sourceRowCount() : mlNumSelRecs;

	mProgressDialog->setValue(maxrange);
	disconnect(obj, SIGNAL(signalProgress(int)), mProgressDialog, SLOT(setValue(int)));
	disconnect(mProgressDialog, SIGNAL(canceled()), obj, SLOT(cancelRequested()));
}

void NMTableView::calcColumn()
{
	NMDebugCtx(__ctxtabview, << "...");

	// get user input

	QString label = QString(tr("%1 = ").arg(this->mLastClickedColumn));
	bool bOk = false;
	QString func = QInputDialog::getText(this, tr("Calculate Column"),
	                                          label, QLineEdit::Normal,
	                                          tr(""), &bOk);
	if (!bOk || func.isEmpty())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(this->mModel));
	prepareProgressDlg(calc.data(), QString("Calculate %1 ...").arg(mLastClickedColumn));
	calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));
	calc->setResultColumn(this->mLastClickedColumn);

	if (this->mSelectionModel->selection().count() > 0)
		calc->setRowFilter(this->mSelectionModel->selection());

	try
	{
		calc->setFunction(func);
		if (!calc->calculate())
		{
			NMErr(__ctxtabview, << "Something went wrong!");
		}
	}
	catch (itk::ExceptionObject& err)
	{
		cleanupProgressDlg(calc.data());
		QString errmsg = QString(tr("%1: %2")).arg(err.GetLocation())
				      .arg(err.GetDescription());

		NMErr(__ctxtabview, << "Calculation failed!"
				<< errmsg.toStdString());

		QMessageBox::critical(this, "Table Calculation Error", errmsg);
	}
	cleanupProgressDlg(calc.data());
	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::updateSelectionAdmin(long numSel)
{
	this->mBtnClearSelection->setEnabled(numSel);

	this->mRecStatusLabel->setText(
			QString(tr("%1 of %2 records selected")).arg(numSel).arg(
					mSortFilter->sourceRowCount()));
	mlNumSelRecs = numSel;
}

void
NMTableView::updateSelectionAdmin(const QItemSelection& sel,
		const QItemSelection& desel)
{
	NMDebugCtx(__ctxtabview, << "...");
	const QItemSelection& srcSel = this->mSelectionModel->selection();

	long selcnt = 0;
	foreach(const QItemSelectionRange& range, srcSel)
	{
		selcnt += range.bottom() - range.top() + 1;
	}
	NMDebugAI(<< "report " << selcnt << " selected records" << std::endl);
	this->updateSelectionAdmin(selcnt);
	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::addColumn()
{
	NMDebugCtx(__ctxtabview, << "...");

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
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	delete dlg;

	int ncols = this->mSortFilter->columnCount();

	NMDebugAI(<< "add column ? " << ret << endl);
	NMDebugAI(<< "name: " << name.toStdString() << endl);
	NMDebugAI(<< "type: " << type << endl);
	NMDebugAI(<< "ncols in tab: " << ncols << endl);

	if (this->mSortFilter->insertColumns(0, type, QModelIndex()))
	{
		this->mSortFilter->setHeaderData(ncols, Qt::Horizontal, name, Qt::DisplayRole);
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::deleteColumn()
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

	if (this->mSortFilter->removeColumns(col, 1, QModelIndex()))
	{
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}
}

void NMTableView::joinAttributes()
{
	NMDebugCtx(__ctxtabview, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Select Source Attribute Table"), "~", tr("Delimited Text File (*.csv)"));
	if (fileName.isNull())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	vtkSmartPointer<vtkDelimitedTextReader> tabReader =
						vtkSmartPointer<vtkDelimitedTextReader>::New();

	tabReader->SetFileName(fileName.toStdString().c_str());
	tabReader->SetHaveHeaders(true);
	tabReader->DetectNumericColumnsOn();
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
		NMDebugCtx(__ctxtabview, << "done!");
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
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}
	NMDebugAI(<< "source field name=" << srcFieldName.toStdString()
			  << " at index " << srcJoinColIdx << std::endl);



	this->appendAttributes(tarJoinColIdx, srcJoinColIdx, srcModel.data());

	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::appendAttributes(const int tarJoinColIdx, const int srcJoinColIdx,
		QAbstractItemModel* srcTable)
{
	NMDebugCtx(__ctxtabview, << "...");

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

	NMDebugAI(<< "copying field contents ...." << std::endl);

	mProgressDialog->setWindowModality(Qt::WindowModal);
	mProgressDialog->setLabelText("Joining Attributes ...");
	mProgressDialog->setRange(0, tarnumrows);

	// copy field values
	//vtkAbstractArray* tarJoin = tar->GetColumnByName(tarJoinField.toStdString().c_str());
	//vtkAbstractArray* srcJoin = src->GetColumnByName(srcJoinField.toStdString().c_str());
	//long cnt = 0;
	long srcnumrows = srcTable->rowCount(QModelIndex());
	for (long row=0; row < tarnumrows && !mProgressDialog->wasCanceled(); ++row)
	{
		QModelIndex tarJoinIdx = mModel->index(row, tarJoinColIdx, QModelIndex());
		QVariant vTarJoin = mModel->data(tarJoinIdx, Qt::DisplayRole);//tarJoin->GetVariantValue(row);
		long search = row < srcnumrows ? row : 0;
		long cnt = 0;
		bool foundyou = false;
		while (cnt < srcnumrows)
		{
			QModelIndex srcJoinIdx = srcTable->index(search, srcJoinColIdx, QModelIndex());
			QVariant vSrcJoin = srcTable->data(srcJoinIdx, Qt::DisplayRole);
			if (vSrcJoin == vTarJoin)
			{
				foundyou = true;
				break;
			}

			++search;
			if (search >= srcnumrows)
				search = 0;

			++cnt;
		}

		if (foundyou)
		{
			// copy columns for current row
			for (int c=0; c < copyIdx.size(); ++c)
			{
				QModelIndex srcIdx = srcTable->index(row, copyIdx.at(c), QModelIndex());
				QVariant srcVal = srcTable->data(srcIdx, Qt::DisplayRole);

				QModelIndex tarIdx = mModel->index(row, writeIdx.at(c), QModelIndex());
				mModel->setData(tarIdx, srcVal, Qt::DisplayRole);
			}
		}

		mProgressDialog->setValue(row+1);
	}

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::hideRow(int row)
{
	if (row < 0 || row > this->mModel->rowCount()-1)
		return;

	this->mTableView->hideRow(row);

}

void NMTableView::exportTable()
{
	NMDebugCtx(__ctxtabview, << "...");

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
		NMDebugCtx(__ctxtabview, << "done!");
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
	//		NMErr(__ctxtabview, << "invalid database and table name!");
	//		return;
	//	}
    //
	//	this->writeSqliteDb(dbName, tableName, false);
	//}

	NMDebugCtx(__ctxtabview, << "done!");
}
void NMTableView::colStats()
{
	NMDebugCtx(__ctxtabview, << "...");

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(mModel));
	const int maxrange = mlNumSelRecs ? mlNumSelRecs*2 : this->mSortFilter->sourceRowCount()*2;
	prepareProgressDlg(calc.data(), "Calculate Column Statistics ...", maxrange);

	calc->setRaw2Source(const_cast<QList<int>* >(mSortFilter->getRaw2Source()));
	calc->setRowFilter(mSelectionModel->selection());
	std::vector<double> stats = calc->calcColumnStats(this->mLastClickedColumn);

	if (stats.size() < 4)
	{
		NMDebugAI(<< "Couldn't calc stats for a reason!" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}
	cleanupProgressDlg(calc.data(), maxrange);

	// min, max, mean, std. dev, sum
	QString smin  = QString("%1").arg(stats[0], 0, 'f', 4); //smin  = smin .rightJustified(15, ' ');
	QString smax  = QString("%1").arg(stats[1], 0, 'f', 4); //smax  = smax .rightJustified(15, ' ');
	QString smean = QString("%1").arg(stats[2], 0, 'f', 4); //smean = smean.rightJustified(15, ' ');
	QString ssdev = QString("%1").arg(stats[3], 0, 'f', 4); //ssdev = ssdev.rightJustified(15, ' ');
	QString ssum  = QString("%1").arg(stats[4], 0, 'f', 4); //ssum  = ssum .rightJustified(15, ' ');
	QString ssample = QString("Sample Size: %1").arg(stats[5]);

	QString strmin  ("Minimum: ");  //strmin  = strmin .rightJustified(10, ' ');
	QString strmax  ("Maximum: ");  //strmax  = strmax .rightJustified(10, ' ');
	QString strmean ("Mean: ");     //strmean = strmean.rightJustified(10, ' ');
	QString strsdev ("Std.Dev.: "); //strsdev = strsdev.rightJustified(10, ' ');
	QString strsum  ("Sum: ");      //strsum  = strsum .rightJustified(10, ' ');


	QString res = QString("%1%2\n%3%4\n%5%6\n%7%8\n%9%10\n%11")
			.arg(strmin).arg(smin)
			.arg(strmax).arg(smax)
			.arg(strmean).arg(smean)
			.arg(strsdev).arg(ssdev)
			.arg(strsum).arg(ssum)
			.arg(ssample);

	QString title = QString(tr("%1")).arg(this->mLastClickedColumn);

	QMessageBox::information(this, title, res);

	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::loadRasLayer(void)
{
	int srcRow = this->mlLastClickedRow;
	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);
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
NMTableView::deleteRasLayer(void)
{
	int srcRow = this->mlLastClickedRow;
	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);
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


bool NMTableView::writeDelimTxt(const QString& fileName,
		bool bselectedRecs)
{
	NMDebugCtx(__ctxtabview, << "...");

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		NMBoxErr("Export Table", "Couldn't create file '" << fileName.toStdString() << "'!");
		NMDebugCtx(__ctxtabview, << "done!");
		return false;
	}
	QTextStream out(&file);

	const QItemSelection& inputSelection = mlNumSelRecs > 0 ? this->mSelectionModel->selection()
			: this->mSortFilter->getSourceSelection(true);
	const int maxrange = mlNumSelRecs > 0 ? mlNumSelRecs : this->mSortFilter->sourceRowCount();
	const int ncols = mModel->columnCount(QModelIndex());

	mProgressDialog->setWindowModality(Qt::WindowModal);
	mProgressDialog->setLabelText("Export Table ...");
	mProgressDialog->setRange(0, maxrange);

	// write header first
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
	foreach(const QItemSelectionRange& range, inputSelection)
	{
		const int top = range.top();
		const int bottom = range.bottom();
		for (int row=top; row <= bottom; ++row)
		{
			for (int col=0; col < ncols; ++col)
			{
				QModelIndex idx = mModel->index(row, col, QModelIndex());
				QVariant val = mModel->data(idx, Qt::DisplayRole);
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
			if (progress % 100 == 0)
			{
				out.flush();
				mProgressDialog->setValue(progress);
			}
		}
	}
	mProgressDialog->setValue(maxrange);
	out.flush();
	file.close();
	NMDebugCtx(__ctxtabview, << "done!");
	return true;


//
//	// ToDo: account for selected rows i.e. filter before export
//
//	vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
//	writer->SetInput(this->mVtkTableAdapter->GetVTKDataObject());
//	writer->SetFieldDelimiter(",");
//
//	NMDebugAI( << "field delimiter: '" << writer->GetFieldDelimiter() << "'" << endl);
//	writer->SetFileName(fileName.toStdString().c_str());
//	writer->Update();
//
//	writer->Delete();
//

	return true;
}



int NMTableView::getColumnIndex(const QString& attr)
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
NMTableView::callHideColumn(void)
{
	this->hideAttribute(this->mLastClickedColumn);
}

void
NMTableView::callUnHideColumn(void)
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

void NMTableView::hideAttribute(const QString& attr)
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
NMTableView::unhideAttribute(const QString& attr)
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

void NMTableView::filterAttribute(const QString& attr,
		const QString& regexp)
{
	NMDebugCtx(__ctxtabview, << "...");

	// apply the provided filter
	int colidx;

	// if the column index is -1, all colmuns are used as key columns
	// in the filter expression
	if (attr.compare(tr("-1")) == 0)
		colidx = -1;
	else
	{
		// get getColumnIndex returns -1 if column is not found and in
		// this case, we have to quit
		colidx = this->getColumnIndex(attr);

		if (colidx < 0)
		{
			NMDebugAI(<< "couldn't find column '" << attr.toStdString()
					<< "'!" << std::endl);
			NMDebugCtx(__ctxtabview, << "done!");
			return;
		}
	}

	this->mSortFilter->setFilterRegExp(QRegExp(regexp, Qt::CaseInsensitive,
			QRegExp::FixedString));
	this->mSortFilter->setFilterKeyColumn(colidx);

	// set the number of rows and the row header index
	int nrows = this->mSortFilter->rowCount();

	QString nrecords = QString(tr("%1 records")).arg(nrows);
	this->mStatusBar->showMessage(nrecords);

	NMDebugCtx(__ctxtabview, << "done!");
}

bool NMTableView::eventFilter(QObject* object, QEvent* event)
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
		this->mLastClickedColumn = this->mSortFilter->
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
			if (row != -1)
			{
				int srcRow = row;
				QModelIndex indx = this->mSortFilter->index(row, 0, QModelIndex());
				QModelIndex srcIndx = this->mSortFilter->mapToSource(indx);
				NMDebugAI(<< "proxy --> source : " << indx.row() << " --> " << srcIndx.row() << std::endl);
				srcRow = srcIndx.row();
				this->toggleRow(srcRow);
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
			if (this->mViewMode == NMTableView::NMTABVIEW_RASMETADATA)
			{
				int row = this->mTableView->rowAt(me->pos().y());
				if (!row)
					return true;

				this->mlLastClickedRow = row;
				this->mManageLayerMenu->move(me->globalPos());
				this->mManageLayerMenu->exec();
			}
			return true;
		}
		// ------------------------LEFT BUTTON DBL CLICK EDITS THE CELL ----------------------------------------
		else if (	me->button() == Qt::LeftButton
				 && event->type() == QEvent::MouseButtonDblClick)
		{
			if (this->mViewMode == NMTableView::NMTABVIEW_ATTRTABLE)
			{
				int row = this->mTableView->rowAt(me->pos().y());
				int col = this->mTableView->columnAt(me->pos().x());
				if (row && col)
				{
					QModelIndex idx = this->mSortFilter->index(row, col, QModelIndex());
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
NMTableView::sortColumn(int col)
{
	NMDebugCtx(__ctxtabview, << "...");

	NMDebugAI(<< "... clear current table view selection" << std::endl);
	this->mTableView->selectionModel()->clear();

	Qt::SortOrder order;
	QMap<int, bool>::iterator it = mMapColSortAsc.find(col);
	if (it != mMapColSortAsc.end())
	{
		if (it.value())
		{
			order = Qt::DescendingOrder;
			mMapColSortAsc.insert(col, false);
		}
		else
		{
			order = Qt::AscendingOrder;
			mMapColSortAsc.insert(col, true);
		}
	}
	else
	{
		order = Qt::AscendingOrder;
		mMapColSortAsc.insert(col, true);
	}
	this->mTableView->horizontalHeader()->setSortIndicator(col, order);
	this->mTableView->horizontalHeader()->setSortIndicatorShown(true);

	NMDebugAI(<< "... actually sorting the column" << std::endl);
	this->mSortFilter->sort(col, order);

	if (this->mChkSelectedRecsOnly->isChecked())
	{
		this->updateSelRecsOnly(Qt::Checked);
	}
	else
	{
		// re-apply any existing selection
		NMDebugAI(<< "... mapping source selection to sorted model" << std::endl);
		this->updateProxySelection(QItemSelection(), QItemSelection());
	}
	NMDebugAI(<< "SORTING DONE!" << std::endl);

	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::selectionQuery(void)
{
	NMDebugCtx(__ctxtabview, << "...");

	bool bOk = false;
	QString query = QInputDialog::getText(this, tr("Selection Query"),
	                                          tr("Where Clause"), QLineEdit::Normal,
	                                          tr(""), &bOk);
	if (!bOk || query.isEmpty())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	QScopedPointer<NMTableCalculator> selector(new NMTableCalculator(this->mModel));
	prepareProgressDlg(selector.data(), QString("Looking for matching records ..."));

	selector->setFunction(query);
	selector->setRowFilter(this->mSelectionModel->selection());
	selector->setSelectionMode(true);
	selector->setRaw2Source(const_cast<QList<int>* >(this->mSortFilter->getRaw2Source()));

	try
	{
		if (!selector->calculate())
		{
			//QMessageBox::critical(this,
			//		"Selection Query Failed",
			//		"Error parsing the query!");

			NMErr(__ctxtabview, << "Selection Query failed!");
			NMDebugCtx(__ctxtabview, << "done!");
			//return;
		}
	}
	catch (itk::ExceptionObject& err)
	{
		cleanupProgressDlg(selector.data());
		QString errmsg = QString(tr("%1: %2")).arg(err.GetLocation())
				      .arg(err.GetDescription());
		NMBoxErr("Selection Query Failed!", "Invalid Query String!");
		NMErr(__ctxtabview, << "Calculation failed!"
				<< errmsg.toStdString());
		return;
	}
	cleanupProgressDlg(selector.data());

	const QItemSelection* srcSel = selector->getSelection();
	long selectorcount = selector->getSelectionCount();
	NMDebugAI(<< __FUNCTION__ << ": calculator reports "
			<< selectorcount << " selected rows" << std::endl);

	mbColumnCalc = true;
	mSelectionModel->setSelection(*srcSel);

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::clearSelection()
{
	mbClearSelection = true;
	if (this->mSelectionModel)
		this->mSelectionModel->clearSelection();
}

void
NMTableView::updateProxySelection(const QItemSelection& sel, const QItemSelection& desel)
{
	NMDebugCtx(__ctxtabview, << "...");
	if (mbClearSelection)
	{
		this->mbClearSelection = false;

		if (this->mChkSelectedRecsOnly->isChecked())
		{
			this->updateSelRecsOnly(Qt::Checked);
		}
		else
		{
			mProxySelModel->clearSelection();
		}
	}
	else
	{
		// probably don't need this anymore
		if (mbSwitchSelection || mbColumnCalc)
		{
			mbSwitchSelection = false;
			mbColumnCalc = false;
		}

		if (this->mChkSelectedRecsOnly->isChecked())
		{
			this->updateSelRecsOnly(Qt::Checked);
		}
		else
		{
			const QItemSelection& proxySel = this->mSortFilter->mapRowSelectionFromSource(
					mSelectionModel->selection(), false);
			mProxySelModel->setSelection(proxySel);
		}

	}
	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::printSelRanges(const QItemSelection& selection, const QString& msg)
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


//void
//NMTableView::selectRow(int row)
//{
////	QModelIndex srcIndex = this->mModel->index(row,0,QModelIndex());
////	QModelIndex proxyIndex = this->mSortFilter->mapFromSource(srcIndex);
////
////	this->mTableView->selectionModel()->select(proxyIndex, QItemSelectionModel::Select |
////			QItemSelectionModel::Rows);
////	this->mTableView->update(proxyIndex);
////
////	if (this->mSelectionModel)
////	{
////		this->mSelectionModel->select(srcIndex, QItemSelectionModel::Select |
////				QItemSelectionModel::Rows);
////	}
////	else
////		this->updateSelectionAdmin(QItemSelection(), QItemSelection());
//}
//
//
//void
//NMTableView::deselectRow(int row)
//{
////	QModelIndex srcIndex = this->mModel->index(row,0,QModelIndex());
////	QModelIndex proxyIndex = this->mSortFilter->mapFromSource(srcIndex);
////
////	this->mTableView->selectionModel()->select(proxyIndex, QItemSelectionModel::Deselect |
////			QItemSelectionModel::Rows);
////	this->mTableView->update(proxyIndex);
////
////	if (this->mSelectionModel)
////	{
////		this->mSelectionModel->select(srcIndex, QItemSelectionModel::Deselect |
////				QItemSelectionModel::Rows);
////	}
////	else
////		this->updateSelectionAdmin(QItemSelection(), QItemSelection());
//}

void
NMTableView::toggleRow(int row)
{
	QModelIndex srcIndex = this->mModel->index(row,0,QModelIndex());
	if (this->mSelectionModel)
	{
		this->mSelectionModel->toggleRow(row, 0, QModelIndex());
	}
}


