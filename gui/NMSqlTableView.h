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
//#define __ctxsqltabview "NMSqlTableView"

//#include "nmlog.h"
#include "NMSelectableSortFilterProxyModel.h"
#include "NMSelSortSqlTableProxyModel.h"
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

class QGraphicsItem;
class QGraphicsSceneMouseEvent;
class NMLogger;

class NMSqlTableView : public QWidget
{
	Q_OBJECT

public:

    typedef enum {  NMTABVIEW_ATTRTABLE,
                    NMTABVIEW_RASMETADATA,
                    NMTABVIEW_PARATABLE,
                    NMTABVIEW_STANDALONE
                  }ViewMode;

    NMSqlTableView(QSqlTableModel* model, QWidget* parent=0);
    NMSqlTableView(QSqlTableModel* model, ViewMode mode, QWidget* parent=0);
    virtual ~NMSqlTableView();

	void setSelectionModel(NMFastTrackSelectionModel* selectionModel);
    void setSelection(const QItemSelection& isel);
    const QItemSelection getSelection();

	ViewMode getViewMode() {return mViewMode;}
	void setViewMode(ViewMode mode);

    void setLayerName(const QString& layer){mLayerName = layer;}
    QString getLayerName(){return mLayerName;}

    NMSelSortSqlTableProxyModel* getSortFilter(){return mSortFilter;}

	void hideAttribute(const QString& attr);
	void unhideAttribute(const QString& attr);
	int getColumnIndex(const QString& attr);
	void setTitle(const QString& title) {this->setWindowTitle(title);}
    QString getTitle(){return this->windowTitle();}
    void setBaseFilter(const QString& baseFilter)
        {mBaseFilter = baseFilter;}

    const QList<int>* getRaw2Source(void)
            {return 0;}

    QSqlTableModel* getModel(void) {return mModel;}

    void setLogger(NMLogger* logger){mLogger = logger;}

    static void ProcessEvents(vtkObject *caller, unsigned long,
                       void *clientData, void *callerData);

    static double angle;

public slots:

    void test();
	void calcColumn();
	void addColumn();
    void indexColumn();
    void addRow();
    void addRows();
	void deleteColumn();
	void exportTable();
	void colStats();
    void userQuery();
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
    void processParaTableRightClick(QGraphicsSceneMouseEvent *gsme, QGraphicsItem *gi);
    void processParaTableDblClick(QGraphicsSceneMouseEvent*);
    void plotScatter();
    void zoomToCoords();
    void refreshTableView(void);

signals:
	//void columnsChanged(int oldCount, int newCount);
	void tableDataChanged(QStringList& slAlteredColumns,
			QStringList& slDeletedColumns);
    void columnsInserted(QModelIndex parent, int fstIdx, int lastIdx);
	void selectionChanged();
    void notifyLastClickedRow(long long cellID);
	void notifyLoadRasLayer(const QString& imagespec,
			const QString& covname);
	void notifyDeleteRasLayer(const QString& imagespec);
    void tableViewClosed();
    void zoomToTableCoords(double* box);

protected slots:
    void procRowsInserted(QModelIndex parent, int first, int last);
    void updateProxySelection(const QItemSelection& selected,
			const QItemSelection& deselected);
	void updateSelRecsOnly(int state);
	void updateSelectionAdmin(const QItemSelection&
			selected, const QItemSelection& deselected);
	void updateSelectionAdmin(long numSel);

    void cellEditorClosed(QWidget* widget, QAbstractItemDelegate::EndEditHint hint);

protected:

	void initView();
	void sortColumn(int col);
    void processUserQuery(const QString& queryName,
                          const QString& sql);

    bool joinFields(const QString& targetField,
                    const QString& srcConnName,
                    const QString& srcTable,
                    const QString& srcField,
                    QStringList& joinFields);

    //    void updateModelSelection();
    void updateSelection(bool swap=false);
    QString updateQueryString(bool swap=false);

	bool writeDelimTxt(const QString& fileName, bool bselectedRecs);
    //	vtkSmartPointer<vtkSQLiteDatabase> writeSqliteDb(
    //			const QString& dbName,
    //			const QString& tableName,
    //			bool bselectedRecs);

	//vtkSmartPointer<vtkTable> queryTable(const QString& sqlStmt);

	bool eventFilter(QObject* object, QEvent* event);
	void showEvent(QShowEvent* event);

	// DEBUG ONLY
	void printSelRanges(const QItemSelection& selection,
			const QString& msg);
	void connectSelModels(bool bconnect);
	void prepareProgressDlg(NMTableCalculator* obj,
			const QString& msg, int maxrange=0);
	void cleanupProgressDlg(NMTableCalculator* obj, int maxrange=0);
    void checkCoords(void);

    void closeEvent(QCloseEvent *event);

	QProgressDialog* mProgressDialog;
	ViewMode mViewMode;

    long mlNumRecs;
	long mlNumSelRecs;
	int mRowKeyColIndex;
	QString mLastClickedColumn;
	long mlLastClickedRow;
	bool mbSwitchSelection;
	bool mbClearSelection;
    //bool mbColumnCalc;
	bool mbIsSelectable;
    bool mbSelectionInProgress;

	QMap<int, bool> mMapColSortAsc;
	QStringList mHiddenColumns;

    std::vector<std::vector<long long > > mvFullSel;

    int mQueryCounter;
    QString mPrimaryKey;
    QString mCurrentQuery;
    QString mCurrentSwapQuery;
    QString mBaseFilter;
    QList<int> mPickedRows;

    QString mLayerName;
    QTableView* mTableView;
    QSqlTableModel* mModel;
    NMSelSortSqlTableProxyModel* mSortFilter;
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
    QAction* mActZoom;
    bool mbHaveCoords;

    NMLogger* mLogger;

private:
    static const std::string ctx;

};

#endif /* NMSqlTableView_H_ */
