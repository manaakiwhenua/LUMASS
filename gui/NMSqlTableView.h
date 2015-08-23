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
 * NMSqlTableView.h
 *
 *  Created on: 17/08/2015
 *      Author: alex
 */

#ifndef NMSqlTableView_H_
#define NMSqlTableView_H_
#define __ctxsqltabview "NMSqlTableView"

#include "nmlog.h"
#include "NMSelectableSortFilterProxyModel.h"
#include "NMFastTrackSelectionModel.h"
#include "NMTableCalculator.h"

#include <QAbstractItemModel>
#include <QSqlTableModel>
#include <QObject>
#include <QMainWindow>
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
#include <QProgressDialog>

#include "vtkTable.h"
/*
#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "QVTKWin32Header.h"
#include "vtkConfigure.h"
#include "vtkSmartPointer.h"
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
*/

//class NMLayer;

class NMSqlTableView : public QWidget
{
	Q_OBJECT

public:

	enum ViewMode{NMTABVIEW_ATTRTABLE,
		NMTABVIEW_RASMETADATA
	};

        NMSqlTableView(QSqlTableModel* model, QWidget* parent=0);
        NMSqlTableView(QSqlTableModel* model, ViewMode mode, QWidget* parent=0);
        virtual ~NMSqlTableView();

	void setSelectionModel(NMFastTrackSelectionModel* selectionModel);
	//void setSelectionModel(QItemSelectionModel* selectionModel);


	ViewMode getViewMode() {return mViewMode;}
	void setViewMode(ViewMode mode);

	void hideAttribute(const QString& attr);
    //void filterAttribute(const QString& attr, const QString& regexp);
	void unhideAttribute(const QString& attr);
	int getColumnIndex(const QString& attr);
	void setTitle(const QString& title) {this->setWindowTitle(title);}
    void setBaseFilter(const QString& baseFilter)
        {mBaseFilter = baseFilter;}

	void hideRow(int row);
    //void hideSource(const QList<int>& rows);
//	const QList<int>* getRaw2Source(void)
//			{return mSortFilter->getRaw2Source();}
    const QList<int>* getRaw2Source(void)
            {return 0;}

public slots:

	void calcColumn();
	void addColumn();
	void deleteColumn();
	void exportTable();
	void colStats();
	//void userQuery();
	void normalise();
	void selectionQuery();
	void clearSelection();
	void update();
	void toggleRow(int row);
	void switchSelection(void);
	void joinAttributes(void);
	void loadRasLayer(void);
	void deleteRasLayer(void);
	void callHideColumn(void);
	void callUnHideColumn(void);
	void setSelectable(bool);

signals:
	//void columnsChanged(int oldCount, int newCount);
	void tableDataChanged(QStringList& slAlteredColumns,
			QStringList& slDeletedColumns);
	void selectionChanged();
	void notifyLastClickedRow(long cellID);
	void notifyLoadRasLayer(const QString& imagespec,
			const QString& covname);
	void notifyDeleteRasLayer(const QString& imagespec);

protected slots:
	void updateProxySelection(const QItemSelection& selected,
			const QItemSelection& deselected);
	void updateSelRecsOnly(int state);
	void updateSelectionAdmin(const QItemSelection&
			selected, const QItemSelection& deselected);
	void updateSelectionAdmin(long numSel);
protected:

	void initView();
	void sortColumn(int col);

	bool writeDelimTxt(const QString& fileName, bool bselectedRecs);
//	vtkSmartPointer<vtkSQLiteDatabase> writeSqliteDb(
//			const QString& dbName,
//			const QString& tableName,
//			bool bselectedRecs);

	//vtkSmartPointer<vtkTable> queryTable(const QString& sqlStmt);

	void appendAttributes(const int tarJoinColIdx,
			const int srcJoinColIdx,
			QAbstractItemModel* srcTable);

	bool eventFilter(QObject* object, QEvent* event);
	void showEvent(QShowEvent* event);

	// DEBUG ONLY
	void printSelRanges(const QItemSelection& selection,
			const QString& msg);
	void connectSelModels(bool bconnect);
	void prepareProgressDlg(NMTableCalculator* obj,
			const QString& msg, int maxrange=0);
	void cleanupProgressDlg(NMTableCalculator* obj, int maxrange=0);

	QProgressDialog* mProgressDialog;
	ViewMode mViewMode;

    long mlNumRecs;
	long mlNumSelRecs;
	int mRowKeyColIndex;
	QString mLastClickedColumn;
	long mlLastClickedRow;
	bool mbSwitchSelection;
	bool mbClearSelection;
	bool mbColumnCalc;
	bool mbIsSelectable;

	QMap<int, bool> mMapColSortAsc;
	QStringList mHiddenColumns;

    QString mBaseFilter;

	QTableView* mTableView;
        QSqlTableModel* mModel;
    //NMSelectableSortFilterProxyModel* mSortFilter;
    QSortFilterProxyModel* mSortFilter;
    NMFastTrackSelectionModel* mSelectionModel;
	NMFastTrackSelectionModel* mProxySelModel;

	QVBoxLayout* mLayout;
	QStatusBar* mStatusBar;
	QLabel* mRecStatusLabel;
	QPushButton* mBtnClearSelection;
	QPushButton* mBtnSwitchSelection;
	QCheckBox* mChkSelectedRecsOnly;

	QMenu* mColHeadMenu;
	QMenu* mManageLayerMenu;

	QAction* mActSel;




	//vtkSmartPointer<vtkTable> mBaseTable;
	//otb::AttributeTable::Pointer mOtbTable;
	//NMLayer* mLayer;

	//QStringList mDeletedColumns;
	//QStringList mAlteredColumns;

};

#endif /* NMSqlTableView_H_ */
