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
 * NMTableView.h
 *
 *  Created on: 22/09/2011
 *      Author: alex
 */

#ifndef NMTABLEVIEW_H_
#define NMTABLEVIEW_H_
#define __ctxtabview "NMTableView"

#include "nmlog.h"
#include "NMLayer.h"
#include "NMSelectableSortFilterProxyModel.h"

#include <QObject>
#include <QtGui/QMainWindow>
#include <QMouseEvent>
#include <QActionEvent>
#include <QChildEvent>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include <QSharedPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QTableView>
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QItemSelection>

#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "QVTKWin32Header.h"
#include "vtkConfigure.h"
#include "vtkSmartPointer.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkSQLiteDatabase.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"
#include "vtkUnicodeStringArray.h"

class NMLayer;

class QVTK_EXPORT NMTableView : public QWidget
{
	Q_OBJECT

public:
	NMTableView(QWidget* parent=0);
	NMTableView(vtkTable* tab, QWidget* parent=0);
	virtual ~NMTableView();

	void hideAttribute(QString attr);
	void filterAttribute(QString attr, QString regexp);
	int getColumnIndex(QString attr);
	void setTitle(QString title) {this->setWindowTitle(title);};
	void setRowKeyColumn(QString rowKeyCol);
	const vtkTable* getTable(void);
	vtkSmartPointer<vtkAbstractArray> createVTKArray(int datatype);

public slots:

	void calcColumn();
	void addColumn();
	void deleteColumn();
	void exportTable();
	void colStats();
	void userQuery();
	void normalise();
	void setTable(vtkTable* tab);
	void selectionQuery();
	void clearSelection();
	void selectRow(int row);
	void deselectRow(int row);
	void updateSelection(void);
	void switchSelection(void);

signals:
	void tableDataChanged(QStringList& slAlteredColumns,
			QStringList& slDeletedColumns);
	void selectionChanged();

protected slots:
	void updateSelRecsOnly(int state);


protected:

	void initView();

	bool writeDelimTxt(QString fileName, bool bselectedRecs);
	vtkSmartPointer<vtkSQLiteDatabase> writeSqliteDb(QString dbName, QString tableName, bool bselectedRecs);

	vtkSmartPointer<vtkTable> queryTable(QString sqlStmt);


	void mousePressEvent(QMouseEvent* event);
	bool eventFilter(QObject* object, QEvent* event);
	void updateSelectionAdmin(long numsel);

	long mlNumSelRecs;
	int mRowKeyColIndex;
	QString mLastClickedColumn;

	QTableView* mTableView;
	QItemSelection mRowSelection;
	vtkQtTableModelAdapter* mVtkTableAdapter;
	NMSelectableSortFilterProxyModel* mSortFilter;

	QVBoxLayout* mLayout;
	QStatusBar* mStatusBar;
	QLabel* mRecStatusLabel;
	QPushButton* mBtnClearSelection;
	QPushButton* mBtnSwitchSelection;
	QCheckBox* mChkSelectedRecsOnly;

	QMenu* mColHeadMenu;

	vtkSmartPointer<vtkTable> mBaseTable;
	NMLayer* mLayer;

	QStringList mDeletedColumns;
	QStringList mAlteredColumns;

};

#endif /* NMTABLEVIEW_H_ */
