 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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

#include "NMTableView.h"
#include "NMAddColumnDialog.h"
#include "NMTableCalculator.h"

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

#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataArray.h"
#include "vtkArrayCalculator.h"
#include "vtkAbstractArray.h"
#include "vtkDoubleArray.h"
#include "vtkTable.h"
#include "vtkTableToSQLiteWriter.h"
#include "vtkSQLiteQuery.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLiteDatabase.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDelimitedTextReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyDataWriter.h"
#include "vtkPolyData.h"
#include "vtkVariant.h"
#include "vtkAlgorithmOutput.h"


NMTableView::NMTableView(QWidget* parent)
	: QWidget(parent)
{
	this->mViewMode = NMTABVIEW_ATTRTABLE;
	this->initView();
}

NMTableView::NMTableView(vtkTable* tab, QWidget* parent)
	: QWidget(parent)
{
	this->mViewMode = NMTABVIEW_ATTRTABLE;
	this->initView();
	this->setTable(tab);
}


NMTableView::NMTableView(vtkTable* tab, ViewMode mode, QWidget* parent)
	: QWidget(parent), mViewMode(mode)
{
	this->initView();
	this->setTable(tab);
}

NMTableView::~NMTableView()
{
	NMDebugCtx(__ctxtabview, << "...");

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::initView()
{
	this->mTableView = new QTableView(this);
	this->mTableView->setSortingEnabled(true);
	this->mTableView->verticalHeader()->hide();

	this->mTableView->setCornerButtonEnabled(false);
	this->mTableView->setAlternatingRowColors(true);

	this->mVtkTableAdapter = new vtkQtTableModelAdapter(this);
	this->mSortFilter = new NMSelectableSortFilterProxyModel(this);
	this->mSortFilter->setDynamicSortFilter(true);

	this->setWindowFlags(Qt::Window);
	this->setWindowTitle(tr("Attributes"));

	this->mDeletedColumns.clear();
	this->mAlteredColumns.clear();

	// ------------------ SET UP STATUS BAR ------------------------------
	this->mStatusBar = new QStatusBar(this);

	this->mRecStatusLabel = new QLabel(tr(""), this->mStatusBar);
	this->mStatusBar->addWidget(this->mRecStatusLabel);

	this->mBtnClearSelection = new QPushButton(this->mStatusBar);
	this->mBtnClearSelection->setText(tr("Clear Selection"));
	this->mStatusBar->addWidget(this->mBtnClearSelection);

	this->mBtnSwitchSelection = new QPushButton(this->mStatusBar);
	this->mBtnSwitchSelection->setText(tr("Switch Selection"));
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

	QAction* actFilter = new QAction(this->mColHeadMenu);
	actFilter->setText(tr("Arbitrary SQL Query ..."));

	QAction* actSel = new QAction(this->mColHeadMenu);
	actSel->setText(tr("Select Attributes ..."));

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
		this->mColHeadMenu->addAction(actNorm);
		this->mColHeadMenu->addSeparator();
		this->mColHeadMenu->addAction(actAdd);
		this->mColHeadMenu->addAction(actDelete);
	}
	this->mColHeadMenu->addSeparator();

	if (mViewMode == NMTABVIEW_ATTRTABLE)
	{
		this->mColHeadMenu->addAction(actJoin);
	}
	this->mColHeadMenu->addAction(actExp);

	this->connect(this->mBtnClearSelection, SIGNAL(pressed()), this, SLOT(clearSelection()));
	this->connect(this->mBtnSwitchSelection, SIGNAL(pressed()), this, SLOT(switchSelection()));
	this->connect(this->mChkSelectedRecsOnly, SIGNAL(stateChanged(int)), this,
			SLOT(updateSelRecsOnly(int)));

	this->connect(actSel, SIGNAL(triggered()), this, SLOT(selectionQuery()));
	this->connect(actStat, SIGNAL(triggered()), this, SLOT(colStats()));
	this->connect(actExp, SIGNAL(triggered()), this, SLOT(exportTable()));
	this->connect(actFilter, SIGNAL(triggered()), this, SLOT(userQuery()));

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

void NMTableView::updateSelRecsOnly(int state)
{
	int selid = this->getColumnIndex("nm_sel");
	this->mSortFilter->setFilterKeyColumn(selid);

	switch (state)
	{
	case Qt::Checked:
		this->mSortFilter->setFilterFixedString("1");
		break;
	case Qt::Unchecked:
		this->mSortFilter->setFilterRegExp("");
		break;
	}

}

void NMTableView::switchSelection()
{
	vtkTable* tab = vtkTable::SafeDownCast(
			this->mVtkTableAdapter->GetVTKDataObject());
	vtkAbstractArray* aa = tab->GetColumnByName("nm_sel");
	if (aa == 0)
	{
		NMErr(__ctxtabview, << "Selection column 'nm_sel' not found!");
		return;
	}

	vtkDataArray* selar = vtkDataArray::SafeDownCast(aa);
	long selcnt = 0;
	for (int r = 0; r < tab->GetNumberOfRows(); ++r)
	{
		if (selar->GetTuple1(r) == 0)
		{
			selar->SetTuple1(r, 1);
			this->selectRow(r);
			++selcnt;
		}
		else
		{
			selar->SetTuple1(r, 0);
			this->deselectRow(r);
		}
	}

	if (selcnt)
	{
		this->mBtnClearSelection->setEnabled(true);
		this->mBtnSwitchSelection->setEnabled(true);
	}
	else
	{
		this->mBtnClearSelection->setEnabled(false);
		this->mBtnSwitchSelection->setEnabled(false);
	}

	this->mlNumSelRecs = selcnt;
	this->mRecStatusLabel->setText(
			QString(tr("%1 of %2 records selected")).arg(selcnt).arg(
					tab->GetNumberOfRows()));

	emit selectionChanged();
}

void NMTableView::normalise()
{
	NMDebugCtx(__ctxtabview, << "...");

	// -----------------------------------------------------------------------
	// get user input

	// get the names of the fields to normalise
	bool bOk = false;
	QString fieldNames = QInputDialog::getText(this, tr("Normalise Fields"),
	                                          tr("Field Names:"), QLineEdit::Normal,
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

	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(tab));
	if (this->mlNumSelRecs)
		calc->setRowFilterModeOn("nm_sel");
	QStringList normCols = calc->normaliseColumns(columnNames, bCostCriterion);
	this->mAlteredColumns.append(normCols);


	// inform listeners
	emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);

	NMDebugCtx(__ctxtabview, << "done!");
}


void NMTableView::userQuery()
{
	NMDebugCtx(__ctxtabview, << "...");
	// ask for the name and the type of the new data field
	bool bOk = false;
	QString sqlStmt = QInputDialog::getText(this, tr("Filter Attributes"),
	                                          tr("SQL Query (use 'memtab1' as table required!)"), QLineEdit::Normal,
	                                          tr("Select * from memtab1 where "), &bOk);
	if (!bOk || sqlStmt.isEmpty())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

    //QString sqlStmt = QString(tr("%1")).arg(whereclause);
	vtkSmartPointer<vtkTable> restab = this->queryTable(sqlStmt);
	if (restab == 0)
	{
		NMDebugAI( << "got an empty result table (i.e. NULL)" << endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	//this->mVtkTableAdapter->setTable(restab);

	NMTableView *resview = new NMTableView(restab, this->parentWidget());
	resview->setWindowFlags(Qt::Window);
	resview->setTitle(tr("Query Result"));
	resview->show();

	NMDebugCtx(__ctxtabview, << "done!");
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

	// get the table
	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(tab));
	calc->setResultColumn(this->mLastClickedColumn);

	if (this->mTableView->selectionModel()->selection().count() > 0)
		calc->setRowFilterModeOn("nm_sel");

	calc->setFunction(func);
	if (calc->calculate())
	{
		this->mAlteredColumns.append(this->mLastClickedColumn);

		emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);
	}
	else
	{
		NMErr(__ctxtabview, << "Calculation failed!");
	}

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::updateSelection(void)
{
	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	vtkAbstractArray* aa = tab->GetColumnByName("nm_sel");
	if (aa == 0)
	{
		NMErr(__ctxtabview, << "Selection column 'nm_sel' not found!");
		return;
	}

	vtkDataArray* selar = vtkDataArray::SafeDownCast(aa);
	long selcnt = 0;
	for (int r=0; r < tab->GetNumberOfRows(); ++r)
	{
		if (selar->GetTuple1(r) == 0)
			this->deselectRow(r);
		else
		{
			++selcnt;
			this->selectRow(r);
		}
	}

	this->updateSelectionAdmin(selcnt);
}

void NMTableView::updateSelectionAdmin(long numsel)
{
	vtkTable* tab = vtkTable::SafeDownCast(
			this->mVtkTableAdapter->GetVTKDataObject());
	int nrecs = tab->GetNumberOfRows();

	if (numsel >=0)
		this->mlNumSelRecs = numsel;

	if (this->mlNumSelRecs)
	{
		this->mBtnClearSelection->setEnabled(true);
		this->mBtnSwitchSelection->setEnabled(true);
	}
	else
	{
		this->mBtnClearSelection->setEnabled(false);
		this->mBtnSwitchSelection->setEnabled(false);
	}

	this->mRecStatusLabel->setText(QString(tr("%1 of %2 records selected")).
			arg(this->mlNumSelRecs).
			arg(nrecs));

}

void NMTableView::addColumn()
{
	NMDebugCtx(__ctxtabview, << "...");

	NMAddColumnDialog* dlg = new NMAddColumnDialog();
	int ret = 0;
	bool bok = true;
	QString name;
	int type = -1;

	// regular expression which defines possible names for columns
	QRegExp nameRegExp("^[A-Za-z_]+[\\d\\w]*$", Qt::CaseInsensitive);
	int pos = -1;
	do {
		ret = dlg->exec();
		if (!ret)
			break;

		name = dlg->getColumnName();
		type = dlg->getDataType();

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

		if (type < 0 || name.isEmpty() || !bok)
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


	} while (!bok || name.isEmpty() || type < 1);

	if (!ret)
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	delete dlg;

	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	int nrows = tab->GetNumberOfRows();

	NMDebugAI(<< "add column ? " << ret << endl);
	NMDebugAI(<< "name: " << name.toStdString() << endl);
	NMDebugAI(<< "type: " << type << endl);
	NMDebugAI(<< "nrows in tab: " << tab->GetNumberOfRows() << endl);

	vtkSmartPointer<vtkAbstractArray> a = this->createVTKArray(type);
	a->SetName(name.toStdString().c_str());
	a->SetNumberOfComponents(1);
	a->Allocate(nrows);

	vtkVariant v;
	if (type == VTK_STRING)
		v = vtkVariant("");
	else
		v = vtkVariant(0);

	for (int t=0; t < nrows; ++t)
		a->InsertVariantValue(t, v);

	tab->AddColumn(a);
	this->mAlteredColumns.append(name);

//	this->mVtkTableAdapter->setTable(tab);

	emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);


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

	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	tab->RemoveColumnByName(this->mLastClickedColumn.toStdString().c_str());
//	this->mVtkTableAdapter->setTable(tab);

	// notify potential listeners
	this->mDeletedColumns.append(this->mLastClickedColumn);
	emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);

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

	vtkTable* vtkTab = tabReader->GetOutput();

	int numJoinCols = vtkTab->GetNumberOfColumns();
	NMDebugAI( << "Analyse CSV Table Structure ... " << endl);
	QStringList srcJoinFields;
	for (int i=0; i < numJoinCols; ++i)
	{
		QString srcName = vtkTab->GetColumn(i)->GetDataTypeAsString();
		if (srcName.compare("int") == 0 || srcName.compare("string") == 0)
		{
			srcJoinFields.append(QString(vtkTab->GetColumnName(i)));
		}
	}

	vtkTable* tarTab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	int numTarCols = tarTab->GetNumberOfColumns();
	QStringList tarJoinFields;
	for (int i=0; i < numTarCols; ++i)
	{
		QString tarName = tarTab->GetColumn(i)->GetDataTypeAsString();
		if (tarName.compare("int") == 0 || tarName.compare("string") == 0)
		{
			tarJoinFields.append(QString(tarTab->GetColumnName(i)));
		}
	}

	// ask the user for semantically common fields
	bool bOk = false;
	QString tarFieldName = QInputDialog::getItem(this,
			tr("Select Target Join Field"), tr("Select Target Join Field"),
			tarJoinFields, 0, false, &bOk, 0);
	if (!bOk || tarFieldName.isEmpty())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	QString srcFieldName = QInputDialog::getItem(this,
			tr("Select Source Join Field"), tr("Select Source Join Field"),
			srcJoinFields, 0, false, &bOk, 0);
	if (!bOk || srcFieldName.isEmpty())
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	this->appendAttributes(tarFieldName, srcFieldName, vtkTab);

	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::appendAttributes(const QString& tarJoinField,
		const QString& srcJoinField,
		vtkTable* src)
{
	vtkTable* tar = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());

	QStringList allTarFields;
	for (int i=0; i < tar->GetNumberOfColumns(); ++i)
	{
		allTarFields.push_back(tar->GetColumnName(i));
	}

	// we determine which columns to copy, and which name + index they have
	QRegExp nameRegExp("^[A-Za-z_]+[\\d\\w]*$", Qt::CaseInsensitive);

	NMDebugAI(<< "checking column names ... " << std::endl);
	QStringList copyNames;
	QStringList writeNames;
	for (int i=0; i < src->GetNumberOfColumns(); ++i)
	{
		QString name = src->GetColumnName(i);
		QString writeName = name;
		if (name.compare(srcJoinField) == 0
				|| name.compare(tarJoinField) == 0)
		{
			continue;
		}
		else if (allTarFields.contains(name, Qt::CaseInsensitive))
		{
			// come up with a new name for the column to appended
			writeName = QString(tr("copy_%1")).arg(name);
		}

		int pos = -1;
		pos = nameRegExp.indexIn(name);
		if (pos >= 0)
		{
			copyNames.push_back(name);
			writeNames.push_back(writeName);
		}
	}

	NMDebugAI( << "adding new columns to target table ..." << std::endl);
	// create new output columns for join fields
	vtkIdType tarnumrows = tar->GetNumberOfRows();
	for (int t=0; t < copyNames.size(); ++t)
	{
		int type = src->GetColumnByName(copyNames[t].toStdString().c_str())->GetDataType();
		vtkSmartPointer<vtkAbstractArray> ar = vtkAbstractArray::CreateArray(type);
		ar->SetName(writeNames.at(t).toStdString().c_str());
		ar->SetNumberOfComponents(1);
		ar->Allocate(tarnumrows);

		vtkVariant v;
		if (type == VTK_STRING)
			v = vtkVariant("");
		else
			v = vtkVariant(0);

		for (int tr=0; tr < tarnumrows; ++tr)
			ar->InsertVariantValue(tr, v);

		tar->AddColumn(ar);
		NMDebugAI(<< copyNames.at(t).toStdString() << "->"
				<< writeNames.at(t).toStdString() << " ");
		this->mAlteredColumns.append(writeNames.at(t));
	}
	NMDebug(<< endl);

	NMDebugAI(<< "copying field contents ...." << std::endl);
	// copy field values
	vtkAbstractArray* tarJoin = tar->GetColumnByName(tarJoinField.toStdString().c_str());
	vtkAbstractArray* srcJoin = src->GetColumnByName(srcJoinField.toStdString().c_str());
	vtkIdType cnt = 0;
	vtkIdType srcnumrows = src->GetNumberOfRows();
	for (vtkIdType row=0; row < tarnumrows; ++row)
	{
		vtkVariant vTarJoin = tarJoin->GetVariantValue(row);
		vtkIdType search = row < srcnumrows ? row : 0;
		vtkIdType cnt = 0;
		bool foundyou = false;
		while (cnt < srcnumrows)
		{
			if (srcJoin->GetVariantValue(search) == vTarJoin)
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
			for (int c=0; c < copyNames.size(); ++c)
			{
				tar->SetValueByName(row, writeNames[c].toStdString().c_str(),
						src->GetValueByName(search, copyNames[c].toStdString().c_str()));
			}
		}
	}

	emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);


}

void NMTableView::exportTable()
{
	NMDebugCtx(__ctxtabview, << "...");

	QString tabName = this->windowTitle().split(" ", QString::SkipEmptyParts).last();
	QString proposedName = QString(tr("%1/%2.txt")).arg(getenv("HOME")).arg(tabName);

	// take the first layer and save as vtkpolydata
	QString selectedFilter = tr("Delimited Text (*.csv)");
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Table"), proposedName,
			tr("Delimited Text (*.csv);;SQLite Database (*.sqlite *.sdb *.db)"),
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
	else if (suffix.compare(tr("sqlite"), Qt::CaseInsensitive) == 0 ||
			 suffix.compare(tr("sdb"), Qt::CaseInsensitive) == 0 ||
			 suffix.compare(tr("db"), Qt::CaseInsensitive) == 0)
	{
		QString dbName;
		QString tableName;
		if (fnList.size() == 3)
		{
			dbName = QString(tr("%1.%2")).arg(fnList.first()).arg(fnList.last());
			tableName = fnList.at(1);
			NMDebugAI(<< "insert table '" << tableName.toStdString() << "' "
					  << "into database '" << dbName.toStdString() << "'" << endl);
		}
		else
		{
			NMErr(__ctxtabview, << "ivalid data base and table name!");
			return;
		}

		this->writeSqliteDb(dbName, tableName, false);
	}


	NMDebugCtx(__ctxtabview, << "done!");
}
void NMTableView::colStats()
{
	NMDebugCtx(__ctxtabview, << "...");

	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	vtkAbstractArray* aa = tab->GetColumnByName(this->mLastClickedColumn.toStdString().c_str());
	if (!aa->IsNumeric())
	{
		NMErr(__ctxtabview, << "Select a numeric column for statistics calculation!");
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	QScopedPointer<NMTableCalculator> calc(new NMTableCalculator(tab));
	if (this->mlNumSelRecs)
		calc->setRowFilterModeOn("nm_sel");
	std::vector<double> stats = calc->calcColumnStats(this->mLastClickedColumn);

	if (stats.size() < 4)
		return;

	// min, max, mean, sum
	QString res = QString(tr("min:   %1\nmax:  %2\nmean:  %3\nsum:  %4")).
			arg(stats[0], 0, 'f', 4).
			arg(stats[1], 0, 'f', 4).
			arg(stats[2], 0, 'f', 4).
			arg(stats[3], 0, 'f', 4);

	QString title = QString(tr("%1")).arg(this->mLastClickedColumn);

	QMessageBox::information(this, title, res);


	NMDebugCtx(__ctxtabview, << "done!");
}

void
NMTableView::loadRasLayer(void)
{
	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);

	vtkLongArray* oidar = vtkLongArray::SafeDownCast(this->mBaseTable->GetColumnByName("oid"));
	vtkStringArray* collar = vtkStringArray::SafeDownCast(this->mBaseTable->GetColumnByName("coll_name"));
	vtkStringArray* covar = vtkStringArray::SafeDownCast(this->mBaseTable->GetColumnByName("covername"));

	QString imagespec = QString("%1:%2").arg(collar->GetValue(srcIdx.row()).c_str())
			                            .arg(oidar->GetValue(srcIdx.row()));

	QString covname = QString(covar->GetValue(srcIdx.row()).c_str());

	emit notifyLoadRasLayer(imagespec, covname);

}

void
NMTableView::deleteRasLayer(void)
{
	QModelIndex sortIdx = this->mSortFilter->index(this->mlLastClickedRow, 0, QModelIndex());
	QModelIndex srcIdx = this->mSortFilter->mapToSource(sortIdx);

	vtkLongArray* oidar = vtkLongArray::SafeDownCast(this->mBaseTable->GetColumnByName("oid"));
	vtkStringArray* collar = vtkStringArray::SafeDownCast(this->mBaseTable->GetColumnByName("coll_name"));

	QString imagespec = QString("%1:%2").arg(collar->GetValue(srcIdx.row()).c_str())
			                            .arg(oidar->GetValue(srcIdx.row()));

	emit notifyDeleteRasLayer(imagespec);
}

void NMTableView::setTable(vtkTable* tab)
{
	NMDebugCtx(__ctxtabview, << "...");

	this->mVtkTableAdapter->setTable(tab);
	this->mSortFilter->setSourceModel(this->mVtkTableAdapter);
	this->mTableView->setModel(this->mSortFilter);


	this->mBaseTable = tab;
	this->mDeletedColumns.clear();
	this->mAlteredColumns.clear();

	this->updateSelection();

	NMDebugCtx(__ctxtabview, << "done!");
}

bool NMTableView::writeDelimTxt(QString fileName, bool bselectedRecs)
{
	NMDebugCtx(__ctxtabview, << "...");

	// ToDo: account for selected rows i.e. filter before export

	vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
	writer->SetInput(this->mVtkTableAdapter->GetVTKDataObject());
	writer->SetFieldDelimiter(",");

	NMDebugAI( << "field delimiter: '" << writer->GetFieldDelimiter() << "'" << endl);
	writer->SetFileName(fileName.toStdString().c_str());
	writer->Update();

	writer->Delete();

	NMDebugCtx(__ctxtabview, << "done!");
	return true;
}

vtkSmartPointer<vtkSQLiteDatabase> NMTableView::writeSqliteDb(QString dbName, QString tableName, bool bselectedRecs)
{
	NMDebugCtx(__ctxtabview, << "...");

	// ToDo: account for selected rows i.e. filter before export
	QString url = QString(tr("sqlite://%1")).arg(dbName);

	NMDebugAI(<< "setting url: " << url.toStdString() << endl);
	vtkSmartPointer<vtkSQLiteDatabase> db = vtkSQLiteDatabase::SafeDownCast(
			vtkSQLDatabase::CreateFromURL(url.toStdString().c_str()));
	if (db->IsA("vtkSQLiteDatabase") == 0)
	{
		NMDebugAI( << "failed creating db '" << dbName.toStdString() << ";" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return 0;
	}
	if (!db->Open("", vtkSQLiteDatabase::USE_EXISTING_OR_CREATE))
	{
		NMDebugAI( << "failed connecting to sqlite db '" << dbName.toStdString() << "'" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return 0;
	}

	vtkSmartPointer<vtkTableToSQLiteWriter> dbwriter = vtkSmartPointer<vtkTableToSQLiteWriter>::New();
	dbwriter->SetDatabase(db);
	dbwriter->SetInput(this->mVtkTableAdapter->GetVTKDataObject());
	dbwriter->SetTableName(tableName.toStdString().c_str());
	dbwriter->Update();

	// we only return the memory version of the data base
	if (dbName.compare(tr(":memory:")) == 0)
	{
		NMDebugAI( << "returning in-memory db" << endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return db;
	}
	else
	{
		NMDebugCtx(__ctxtabview, << "done!");
		return 0;
	}
}

vtkSmartPointer<vtkTable> NMTableView::queryTable(QString sqlStmt)
{
	NMDebugCtx(__ctxtabview, << "...");
	vtkSmartPointer<vtkSQLiteDatabase> sdb = this->writeSqliteDb(
			tr(":memory:"), tr("memtab1"), false);
	if (sdb == 0)
	{
		NMDebugAI( << "couldn't create memory sql db for querying!" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return 0;
	}
//	if (!sdb->Open(0, vtkSQLiteDatabase::USE_EXISTING_OR_CREATE))
//	{
//		NMDebugAI( << "failed connecting to in-memory sql db!" << std::endl);
//		NMDebugCtx(__ctxtabview, << "done!");
//		return 0;
//	}
	if (!sdb->IsOpen())
	{
		NMDebugAI( << "in-memory db is not open!" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");

		return 0;
	}

	vtkSmartPointer<vtkSQLiteQuery> sq = vtkSQLiteQuery::SafeDownCast(
			sdb->GetQueryInstance());
	sq->SetQuery(sqlStmt.toStdString().c_str());

	// filter to a new table
	vtkSmartPointer<vtkRowQueryToTable> rowtotab = vtkSmartPointer<vtkRowQueryToTable>::New();
	rowtotab->SetQuery(sq);
	rowtotab->Update();

	vtkSmartPointer<vtkTable> outtab = rowtotab->GetOutput();
	NMDebugCtx(__ctxtabview, << "done!");
	return outtab;
}

int NMTableView::getColumnIndex(QString attr)
{
	int attrCol = -1;

	if (this->mVtkTableAdapter->GetVTKDataObject() == 0)
		return attrCol;

	// determine the column number
	int ncols = this->mVtkTableAdapter->columnCount(QModelIndex());
	QString colname;
	for (int c=0; c < ncols; ++c)
	{
		colname = this->mVtkTableAdapter->headerData(c, Qt::Horizontal).toString();
		if (colname.compare(attr, Qt::CaseInsensitive) == 0)
			return c;
	}

	return attrCol;
}

void NMTableView::setRowKeyColumn(QString rowKeyCol)
{
	NMDebugCtx(__ctxtabview, << "...");
	int colidx = this->getColumnIndex(rowKeyCol);
	if (colidx < 0)
	{
		NMDebugAI(<< "you have to set the vtk data object before you can set "
				<< " the row's key column!" << std::endl);
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	this->mVtkTableAdapter->SetKeyColumn(colidx);
	this->mTableView->verticalHeader()->show();

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::hideAttribute(QString attr)
{
	int colidx = this->getColumnIndex(attr);
	if (colidx >= 0)
		this->mTableView->hideColumn(colidx);
}

void NMTableView::filterAttribute(QString attr, QString regexp)
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

void NMTableView::mousePressEvent(QMouseEvent* event)
{
	NMDebugCtx(__ctxtabview, << "...");

	//QWidget::mousePressEvent(event);



	NMDebugCtx(__ctxtabview, << "done!");
}

bool NMTableView::eventFilter(QObject* object, QEvent* event)
{
	// -------------------------- CONTEXT MENU CALLING -----------------------------------------
	if (object == this->mTableView->horizontalHeader()->viewport() &&
			event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* me = static_cast<QMouseEvent*>(event);
		if (me->button() == Qt::RightButton)
		{
			int col = this->mTableView->columnAt(me->pos().x());
			this->mLastClickedColumn = this->mSortFilter->
					headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();

			this->mColHeadMenu->move(me->globalPos());
			this->mColHeadMenu->exec();
		}
		return false;
	}
	// ----------------------- ROW SELECTION HANDLING ----------------------------------------
	else if (object == this->mTableView->verticalHeader()->viewport() &&
				event->type() == QEvent::MouseButtonPress)
	{
		//		NMDebugAI(<< "row header clicked!" << endl);
		QMouseEvent* me = static_cast<QMouseEvent*>(event);
		if (me->button() == Qt::LeftButton)
		{
			if (me->modifiers() != Qt::ControlModifier)
			{
					this->clearSelection();
			}

			vtkTable* tab = vtkTable::SafeDownCast(
					this->mVtkTableAdapter->GetVTKDataObject());
			vtkDataArray* sa = vtkDataArray::SafeDownCast(tab->GetColumnByName("nm_sel"));

			int row = this->mTableView->rowAt(me->pos().y());
			//			NMDebugAI(<< "sel proc in row: " << row << endl);
			QModelIndex ridx = this->mSortFilter->index(row, 0, QModelIndex());
			if (sa->GetTuple1(row))
			{
				sa->SetTuple1(row, 0);
				this->deselectRow(row);
				--this->mlNumSelRecs;
			}
			else
			{
				sa->SetTuple1(row, 1);
				this->selectRow(row);
				++this->mlNumSelRecs;

			}
			this->updateSelectionAdmin(-1);
			emit selectionChanged ();
		}
		return true;
	}
	// ----------------------------- PREVENT CELL SELECTION ------------------------------------
	else if (   object == this->mTableView->viewport()
			 && event->type() == QEvent::MouseButtonPress
			 )
	{
			QMouseEvent* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::RightButton)
			{
				this->mlLastClickedRow = this->mTableView->rowAt(me->pos().y());
				NMDebugAI(<< "clicked at row: " << mlLastClickedRow << std::endl);

				this->mManageLayerMenu->move(me->globalPos());
				this->mManageLayerMenu->exec();
			}
			return false;
	}


	return false;
}

vtkSmartPointer<vtkAbstractArray> NMTableView::createVTKArray(int datatype)
{
	vtkSmartPointer<vtkAbstractArray> a;
	switch(datatype)
	{
		case VTK_BIT: a = vtkSmartPointer<vtkBitArray>::New(); break;
		case VTK_CHAR: a = vtkSmartPointer<vtkCharArray>::New(); break;
		case VTK_SIGNED_CHAR: a = vtkSmartPointer<vtkSignedCharArray>::New(); break;
		case VTK_UNSIGNED_CHAR: a = vtkSmartPointer<vtkUnsignedCharArray>::New(); break;
		case VTK_SHORT: a = vtkSmartPointer<vtkShortArray>::New(); break;
		case VTK_UNSIGNED_SHORT: a = vtkSmartPointer<vtkUnsignedShortArray>::New(); break;
		case VTK_INT: a = vtkSmartPointer<vtkIntArray>::New(); break;
		case VTK_UNSIGNED_INT: a = vtkSmartPointer<vtkUnsignedIntArray>::New(); break;
		case VTK_LONG: a = vtkSmartPointer<vtkLongArray>::New(); break;
		case VTK_UNSIGNED_LONG: a = vtkSmartPointer<vtkUnsignedLongArray>::New(); break;
		case VTK_FLOAT: a = vtkSmartPointer<vtkFloatArray>::New(); break;
		case VTK_DOUBLE: a = vtkSmartPointer<vtkDoubleArray>::New(); break;
		case VTK_STRING: a = vtkSmartPointer<vtkStringArray>::New(); break;
		default: a = vtkSmartPointer<vtkVariantArray>::New(); break;
	}

	return a;
}

const vtkTable* NMTableView::getTable(void)
{
	return vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
}

void NMTableView::selectionQuery(void)
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

	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());
	QScopedPointer<NMTableCalculator> selector(new NMTableCalculator(tab));
	selector->setFunction(query);
	selector->setSelectionModeOn("nm_sel");//, this);
	bool res = selector->calculate();

	if (!res)
	{
		NMErr(__ctxtabview, << "Selection Query failed!");
		NMDebugCtx(__ctxtabview, << "done!");
		return;
	}

	this->updateSelection();
	//this->updateSelectionAdmin(selector->getSelectionCount());
	emit selectionChanged();

	NMDebugCtx(__ctxtabview, << "done!");
}

void NMTableView::clearSelection()
{
	vtkTable* tab = vtkTable::SafeDownCast(this->mVtkTableAdapter->GetVTKDataObject());

	QScopedPointer<NMTableCalculator> selector(new NMTableCalculator(tab));
	selector->setResultColumn("nm_sel");
	selector->setFunction("0");
	if (!selector->calculate())
	{
		NMErr(__ctxtabview, << "Error deselecting rows!");
		return;
	}

	this->mlNumSelRecs = 0;
	this->mTableView->clearSelection();
	this->mTableView->update();
	this->mSortFilter->invalidate();

	this->updateSelectionAdmin(-1);
	emit selectionChanged();
}

void NMTableView::selectRow(int row)
{
	QModelIndex idx = this->mVtkTableAdapter->index(row,0,QModelIndex());
	QModelIndex mappedIndex = this->mSortFilter->mapFromSource(idx);
	this->mTableView->selectionModel()->select(mappedIndex, QItemSelectionModel::Select |
			QItemSelectionModel::Rows);
	this->mTableView->update(idx);
}

void NMTableView::deselectRow(int row)
{
	QModelIndex idx = this->mVtkTableAdapter->index(row,0,QModelIndex());
	QModelIndex mappedIndex = this->mSortFilter->mapFromSource(idx);
	this->mTableView->selectionModel()->select(mappedIndex, QItemSelectionModel::Deselect |
			QItemSelectionModel::Rows);
	this->mTableView->update(idx);
}

