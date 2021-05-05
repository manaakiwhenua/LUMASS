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
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>

#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlIndex>
#include <QSqlError>

#include "NMMacros.h"
#include "NMSqlTableView.h"
#include "NMAddColumnDialog.h"
#include "NMTableCalculator.h"
#include "NMSelSortSqlTableProxyModel.h"
#include "NMGlobalHelper.h"
#include "modelcomponentlist.h"
#include "NMLayer.h"
#include "NMImageLayer.h"
#include "NMLogger.h"
#include "NMChartView.h"

#include "nmqsql_sqlite_p.h"
#include "nmqsqlcachedresult_p.h"

#include "lumassmainwin.h"

#include "vtkQtTableModelAdapter.h"
#include "vtkDelimitedTextReader.h"
#include "vtkSmartPointer.h"
#include "vtkChartXYZ.h"
#include "vtkChartXY.h"
#include "vtkContextView.h"
#include "vtkTable.h"
#include "vtkPlotPoints3D.h"
#include "vtkPlotPoints.h"
#include "vtkNew.h"
#include "vtkCallbackCommand.h"
#include "vtkAxis.h"
#include "vtkContextScene.h"
#include "vtkFloatArray.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"

#include "itkNumericTraits.h"


#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif

//#include "valgrind/callgrind.h"

const std::string NMSqlTableView::ctx = "NMSqlTableView";
double NMSqlTableView::angle = 0;

NMSqlTableView::NMSqlTableView(QSqlTableModel* model, QWidget* parent)
    : QWidget(parent), mViewMode(NMTABVIEW_ATTRTABLE),
      mModel(model), mbSwitchSelection(false), mbClearSelection(false),
      mSelectionModel(0), mbIsSelectable(true), mSortFilter(0), mQueryCounter(0), mbHaveCoords(false)
{
    this->mTableView = new QTableView(this);

    mSortFilter = new NMSelSortSqlTableProxyModel(this);
    mSortFilter->setSourceModel(mModel);

    mModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    this->mTableView->setModel(mModel);

    //this->mProxySelModel = new NMFastTrackSelectionModel(mSortFilter, this);
    this->mProxySelModel = new NMFastTrackSelectionModel(mModel, this);
    this->mProxySelModel->setObjectName("FastProxySelection");
    this->mTableView->setSelectionModel(mProxySelModel);

    // we create our own 'source' selection model when not in ATTRTABLE mode
    mSelectionModel = new NMFastTrackSelectionModel(model, this);

    this->initView();
}

NMSqlTableView::NMSqlTableView(QSqlTableModel *model, ViewMode mode, QWidget* parent)
    : QWidget(parent), mViewMode(mode),
      mModel(model), mbSwitchSelection(false), mbClearSelection(false),
      mSelectionModel(0), mbIsSelectable(true), mSortFilter(0), mQueryCounter(0), mbHaveCoords(false)
{
    this->mTableView = new QTableView(this);
    mSortFilter = new NMSelSortSqlTableProxyModel(this);
    mSortFilter->setSourceModel(mModel);


    this->mTableView->setModel(mModel);

    this->mProxySelModel = new NMFastTrackSelectionModel(mModel, this);
    this->mProxySelModel->setObjectName("FastProxySelection");
    this->mTableView->setSelectionModel(mProxySelModel);

    // we create our own 'source' selection model when not in ATTRTABLE mode
    if (mViewMode != NMTABVIEW_ATTRTABLE)
    {
        mSelectionModel = new NMFastTrackSelectionModel(model, this);
    }

    this->initView();
}


NMSqlTableView::~NMSqlTableView()
{
    //NMDebugAI(<< this->windowTitle().toStdString() << " destructs itself ..." << std::endl);
    //this->mModel->database().close();
    this->mModel = 0;
}

void
NMSqlTableView::zoomToCoords()
{
    if (!mbHaveCoords || this->mlNumSelRecs == 0)
    {
        return;
    }

    QString qstr = QString("select minXCoord, maxXCoord, minYCoord, maxYCoord");
    int minZIdx = this->getColumnIndex("minZCoord");
    int maxZIdx = this->getColumnIndex("maxZCoord");
    bool bZ = false;
    if (minZIdx >= 0 && maxZIdx >=0)
    {
        qstr += ", minZCoord, maxZCoord";
        bZ = true;
    }

    QSqlDatabase db = this->mModel->database();
    QSqlDriver* drv = db.driver();
    qstr += QString(" from %1 where %2;")
            .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName))
            .arg(this->updateQueryString());


    db.transaction();
    QSqlQuery corQuery(db);
    corQuery.setForwardOnly(true);

    if (!corQuery.exec(qstr))
    {
        NMLogError(<< ctx << ": " << corQuery.lastError().text().toStdString());
        corQuery.finish();
        db.rollback();
        return;
    }

    double box[6];
    for (int d=0; d < 3; ++d)
    {
        box[d*2] = itk::NumericTraits<double>::max();
        box[d*2+1] = itk::NumericTraits<double>::NonpositiveMin();
    }

    while (corQuery.next())
    {
        const double minX = corQuery.value(0).toDouble();
        const double maxX = corQuery.value(1).toDouble();
        const double minY = corQuery.value(2).toDouble();
        const double maxY = corQuery.value(3).toDouble();
        double minZ = 0;
        double maxZ = 0;
        if (bZ)
        {
            minZ = corQuery.value(4).toDouble();
            maxZ = corQuery.value(5).toDouble();
        }

        box[0] = minX < box[0] ? minX : box[0];
        box[1] = maxX > box[1] ? maxX : box[1];
        box[2] = minY < box[2] ? minY : box[2];
        box[3] = maxY > box[3] ? maxY : box[3];
        box[4] = minZ < box[4] ? minZ : box[4];
        box[5] = maxZ > box[5] ? maxZ : box[5];
    }

    corQuery.finish();
    db.commit();

    emit zoomToTableCoords(box);
}

void
NMSqlTableView::checkCoords(void)
{
    mbHaveCoords = false;
    QStringList reqCols;
    reqCols << "minXCoord" << "minYCoord" << "maxXCoord" << "maxYCoord";
    QList<QVariant::Type> allowedTypes;
    allowedTypes << QVariant::Double << QVariant::Int << QVariant::LongLong;
    QSet<QString> availCols;

    for (int c=0; c < this->mModel->columnCount(); ++c)
    {
        QString cname = this->mModel->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        if (reqCols.contains(cname))
        {
            QModelIndex mid = this->mModel->index(0, c);
            if (allowedTypes.contains(this->mModel->data(mid).type()))
            {
                availCols << cname;
            }
        }
    }

    if (availCols.count() == 4)
    {
        mbHaveCoords = true;
    }
}

void NMSqlTableView::cellEditorClosed(QWidget *widget, QAbstractItemDelegate::EndEditHint hint)
{
    NMLogDebug(<< ".. going to back to read-only mode ... ");
    //mModel->submitAll();
    //mSortFilter->openReadModel();
}

void NMSqlTableView::initView()
{
    this->setWindowIcon(QIcon(":table_object.png"));

    mbSelectionInProgress = false;
    mLayerName.clear();

    this->mTableView->setCornerButtonEnabled(false);
    this->mTableView->setAlternatingRowColors(true);
    this->mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // get notified if a cell editor is closed
//    connect(this->mTableView->itemDelegate(),
//            SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
//            this, SLOT(cellEditorClosed(QWidget*,QAbstractItemDelegate::EndEditHint)));

    this->setWindowFlags(Qt::Window);
    this->mHiddenColumns.clear();

    mPrimaryKey = mSortFilter->getSourcePK();

    // ---------------------------- THE PROGRESS DIALOG -------------------
    //mProgressDialog = new QProgressDialog(this);

    // ------------------ check for coords zoom support--------------------
    checkCoords();


    // init some other members that need to be established
    // before we initially update the selection
    this->mColHeadMenu = new QMenu(this);

    // and the
    mActZoom = new QAction(this->mColHeadMenu);
    mActZoom->setText(tr("Zoom To Selection"));
    if (!mbHaveCoords)
    {
        mActZoom->setEnabled(false);
    }


    // ------------------ SET UP STATUS BAR ------------------------------
    this->mStatusBar = new QStatusBar(this);

    this->mRecStatusLabel = new QLabel(tr(""), this->mStatusBar);


    this->mBtnClearSelection = new QPushButton(this->mStatusBar);
    this->mBtnClearSelection->setText(tr("Clear Selection"));
    mBtnClearSelection->setVisible(false);


    this->mBtnSwitchSelection = new QPushButton(this->mStatusBar);
    this->mBtnSwitchSelection->setText(tr("Swap Selection"));
    mBtnSwitchSelection->setVisible(false);

    this->mChkSelectedRecsOnly = new QCheckBox(this->mStatusBar);
    this->mChkSelectedRecsOnly->setText(tr("Selected Records Only"));
    this->mChkSelectedRecsOnly->setCheckState(Qt::Unchecked);
    mChkSelectedRecsOnly->setVisible(false);

    this->mBtnEditCells = new QPushButton(this->mStatusBar);
    this->mBtnEditCells->setCheckable(true);
    this->mBtnEditCells->setChecked(false);
    this->mBtnEditCells->setText("Edit Cells");
    connect(mBtnEditCells, SIGNAL(clicked(bool)), SLOT(updateEditCells(bool)));


    this->mStatusBar->addWidget(this->mRecStatusLabel);
    this->mStatusBar->addWidget(this->mBtnEditCells);
    if (mViewMode != NMTABVIEW_PARATABLE)
    {
        this->mStatusBar->addWidget(this->mBtnClearSelection);
        this->mStatusBar->addWidget(this->mBtnSwitchSelection);
        this->mStatusBar->addWidget(this->mChkSelectedRecsOnly);

        mBtnClearSelection->setVisible(true);
        mBtnSwitchSelection->setVisible(true);
        mChkSelectedRecsOnly->setVisible(true);
    }

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

    //    if (this->mViewMode == NMTABVIEW_PARATABLE)
    //    {
    //        Qt::WindowFlags flags = mColHeadMenu->windowFlags();
    //        flags |= Qt::BypassGraphicsProxyWidget;
    //        this->mColHeadMenu->setWindowFlags(flags);
    //    }

    // the graph menu and actions
    QMenu* graphMenu = new QMenu(mColHeadMenu);
    graphMenu->setTitle(tr("Graph Column"));

    QAction* actScatter = new QAction(graphMenu);
    actScatter->setText(tr("Scatter Plot ..."));
    graphMenu->addAction(actScatter);

    // other actions
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

    QAction* actAddRow = 0;
    QAction* actAddRows = 0;
    if (mViewMode == NMTABVIEW_PARATABLE || mViewMode == NMTABVIEW_STANDALONE)
    {
        actAddRow = new QAction(this->mColHeadMenu);
        actAddRow->setText(tr("Add Row"));

        actAddRows = new QAction(this->mColHeadMenu);
        actAddRows->setText(tr("Add Rows ..."));
    }

    QAction* actDelete;
    QAction* actIndex;
    QAction* actAdd;
    QAction* actCalc;
    QAction* actNorm;
    QAction* actJoin;

    if (mViewMode != NMTABVIEW_RASMETADATA)
    {
        actDelete = new QAction(this->mColHeadMenu);
        actDelete->setText(tr("Delete Column"));

        actIndex = new QAction(this->mColHeadMenu);
        actIndex->setText(tr("Create Column Index"));

        actAdd = new QAction(this->mColHeadMenu);
        actAdd->setText(tr("Add Column ..."));

        actCalc = new QAction(this->mColHeadMenu);
        actCalc->setText(tr("Calculate Column ..."));

        actNorm = new QAction(this->mColHeadMenu);
        //actNorm->setText(tr("Normalise Attributes ..."));

        actJoin = new QAction(this->mColHeadMenu);
        actJoin->setText(tr("Join Attributes ..."));
    }

    QAction* actRefresh = new QAction(this->mColHeadMenu);
    actRefresh->setText(tr("Refresh Table View"));


    this->mColHeadMenu->addAction(actSel);
    this->mColHeadMenu->addAction(actFilter);
    this->mColHeadMenu->addSeparator();
    this->mColHeadMenu->addAction(actStat);

    if (mViewMode != NMTABVIEW_RASMETADATA)
    {
        this->mColHeadMenu->addAction(actCalc);
        //this->mColHeadMenu->addAction(actNorm);
        this->mColHeadMenu->addSeparator();
        if (mViewMode != NMTABVIEW_PARATABLE)
        {
            this->mColHeadMenu->addMenu(graphMenu);
            this->mColHeadMenu->addSeparator();
        }

        this->mColHeadMenu->addAction(actIndex);
        this->mColHeadMenu->addAction(actAdd);

        if (    mViewMode == NMTABVIEW_PARATABLE
            ||  mViewMode == NMTABVIEW_STANDALONE
           )
        {
            this->mColHeadMenu->addAction(actAddRow);
            this->mColHeadMenu->addAction(actAddRows);
        }
        this->mColHeadMenu->addAction(actDelete);
    }
    this->mColHeadMenu->addSeparator();

    this->mColHeadMenu->addAction(mActZoom);

    this->mColHeadMenu->addSeparator();
    this->mColHeadMenu->addAction(actHide);
    this->mColHeadMenu->addAction(actUnHide);

    this->mColHeadMenu->addSeparator();
    if (    mViewMode == NMTABVIEW_ATTRTABLE
        ||  mViewMode == NMTABVIEW_STANDALONE
       )
    {
        this->mColHeadMenu->addAction(actJoin);
    }
    this->mColHeadMenu->addAction(actExp);

    this->mColHeadMenu->addSeparator();
    this->mColHeadMenu->addAction(actRefresh);



#ifdef LUMASS_DEBUG
    QAction* actTest = new QAction(this->mColHeadMenu);
    actTest->setText(tr("Test ..."));

    this->mColHeadMenu->addSeparator();
    this->mColHeadMenu->addAction(actTest);

    this->connect(actTest, SIGNAL(triggered()),
                  this, SLOT(test()));
#endif


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
    this->connect(mActZoom, SIGNAL(triggered()), this, SLOT(zoomToCoords()));

    if (mViewMode != NMTABVIEW_RASMETADATA)
    {
        this->connect(actIndex, SIGNAL(triggered()), this, SLOT(indexColumn()));
        this->connect(actAdd, SIGNAL(triggered()), this, SLOT(addColumn()));
        this->connect(actDelete, SIGNAL(triggered()), this, SLOT(deleteColumn()));
        this->connect(actCalc, SIGNAL(triggered()), this, SLOT(calcColumn()));
        this->connect(actNorm, SIGNAL(triggered()), this, SLOT(normalise()));
        if (    mViewMode == NMTABVIEW_ATTRTABLE
            ||  mViewMode == NMTABVIEW_STANDALONE
           )
        {
            this->connect(actJoin, SIGNAL(triggered()), this, SLOT(joinAttributes()));
            this->connect(actScatter, SIGNAL(triggered()), this, SLOT(plotScatter()));
        }

        if (    mViewMode == NMTABVIEW_PARATABLE
            ||  mViewMode == NMTABVIEW_STANDALONE
           )
        {
            this->connect(actAddRow, SIGNAL(triggered()), this, SLOT(addRow()));
            this->connect(actAddRows, SIGNAL(triggered()), this, SLOT(addRows()));
        }
    }

    this->connect(actRefresh, SIGNAL(triggered()), this, SLOT(refreshTableView()));

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
NMSqlTableView::refreshTableView(void)
{
    if (mSortFilter != nullptr)
    {
        mSortFilter->reloadSourceModel();
    }
}

void
NMSqlTableView::updateEditCells(bool bChecked)
{
    if (bChecked)
    {
        this->mSortFilter->openWriteModel();
        this->mBtnEditCells->setText("Stop Editing");
    }
    else
    {
        this->mModel->submitAll();
        this->mSortFilter->openReadModel();
        this->mBtnEditCells->setText("Edit Cells");
    }
}

void
NMSqlTableView::ProcessEvents(vtkObject *caller, unsigned long,
                   void *clientData, void *callerData)
{
  vtkChartXYZ *chart = reinterpret_cast<vtkChartXYZ *>(clientData);
  vtkRenderWindowInteractor *interactor =
      reinterpret_cast<vtkRenderWindowInteractor *>(caller);
  angle += 2;
  chart->SetAngle(angle);
  interactor->Render();
  if (angle >= 90)
    {
    int timerId = *reinterpret_cast<int *>(callerData);
    interactor->DestroyTimer(timerId);
    }
}


void
NMSqlTableView::test()
{
}

void
NMSqlTableView::plotScatter()
{

    NMChartView* chartView = new NMChartView(this);
    vtkContextView* view = chartView->getContextView();

    vtkNew<vtkChartXY> chart;
    view->GetScene()->AddItem(chart.GetPointer());


    int nrows = mSortFilter->getNumTableRecords();

    QStringList colnames;
    int ncols = mModel->columnCount();
    for (int c=0; c < ncols; ++c)
    {
        colnames << mModel->headerData(c, Qt::Horizontal).toString();
    }

    // pick x-col
    QString xcolname = QInputDialog::getItem(this, "Pick x column", "", colnames);

    // Create a table with some points in it...
    vtkNew<vtkTable> table;
    vtkNew<vtkFloatArray> arrX;
    arrX->SetName(xcolname.toStdString().c_str());
    arrX->SetNumberOfComponents(1);
    arrX->SetNumberOfValues(nrows);
    table->AddColumn(arrX.GetPointer());


    vtkNew<vtkFloatArray> arrY;
    arrY->SetName(this->mLastClickedColumn.toStdString().c_str());
    arrY->SetNumberOfComponents(1);
    arrY->SetNumberOfValues(nrows);

    table->AddColumn(arrY.GetPointer());


    NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(mModel);

    QSqlDatabase db = sqlModel->database();
    QSqlDriver* drv = db.driver();

    // query the max y value
    QString maxYStr = QString("select min(%1),max(%1), min(%2), max(%2) from %3")
            .arg(drv->escapeIdentifier(mLastClickedColumn, QSqlDriver::FieldName))
            .arg(drv->escapeIdentifier(xcolname, QSqlDriver::FieldName))
            .arg(drv->escapeIdentifier(sqlModel->tableName(), QSqlDriver::TableName));
    double maxY = 0.0;
    double minY = 0.0;
    double minX = 0.0;
    double maxX = 0.0;
    QSqlQuery maxQ(db);
    if (maxQ.exec(maxYStr))
    {
        minY = maxQ.value(0).toDouble();
        maxY = maxQ.value(1).toDouble();
        minX = maxQ.value(2).toDouble();
        maxX = maxQ.value(3).toDouble();
    }
    else
    {
        NMLogError(<< maxQ.lastError().text().toStdString());
        return;
    }
    maxQ.finish();

    // now query the actual y values for the plot
    QString queryStr = QString("select %1, %2 from %3")
                .arg(drv->escapeIdentifier(xcolname, QSqlDriver::FieldName))
                .arg(drv->escapeIdentifier(mLastClickedColumn, QSqlDriver::FieldName))
                .arg(drv->escapeIdentifier(sqlModel->tableName(), QSqlDriver::TableName));

    QSqlQuery q(db);
    if (q.exec(queryStr))
    {
        int id = 0;
        while (q.next())
        {
            float x = q.value(0).toFloat();
            float y = q.value(1).toFloat();
            table->SetValue(id, 0, vtkVariant(x));
            table->SetValue(id, 1, vtkVariant(y));

            ++id;
        }
    }
    q.finish();

    // Add the three dimensions we are interested in visualizing.
    vtkNew<vtkPlotPoints> plot;
//    plot->SetInputData(table.GetPointer(),
//                       xcolname.toStdString().c_str(),
//                       mLastClickedColumn.toStdString().c_str());
    plot->SetInputData(table.GetPointer(), 0,1);

    plot->SetColor(0.7, 0.7, 0.7);
    plot->SetMarkerSize(-8);

    chart->AddPlot(plot.GetPointer());
    chart->GetAxis(0)->SetUnscaledRange(minX, maxX);
    chart->GetAxis(1)->SetTitle(xcolname.toStdString().c_str());
    chart->GetAxis(1)->SetUnscaledRange(minY, maxY);
    chart->GetAxis(0)->SetTitle(mLastClickedColumn.toStdString().c_str());

    vtkTextProperty* xprop = chart->GetAxis(0)->GetLabelProperties();
    xprop->SetFontSize(16);

    vtkTextProperty* yprop = chart->GetAxis(1)->GetLabelProperties();
    yprop->SetFontSize(16);

    // TESTING INTERACTION ===============
    view->GetInteractor()->Initialize();

    vtkNew<vtkCallbackCommand> callback;
    callback->SetClientData(chart);
    callback->SetCallback(NMSqlTableView::ProcessEvents);

    view->GetInteractor()->AddObserver(vtkCommand::TimerEvent,
                                       callback, 0);
    view->GetInteractor()->CreateRepeatingTimer(1000/25);
    view->GetInteractor()->Start();

    // ==========================

    chartView->show();
    chartView->getRenderWindow()->Render();
}

void
NMSqlTableView::closeEvent(QCloseEvent *event)
{
    switch(mViewMode)
    {
    case NMTABVIEW_ATTRTABLE:
    case NMTABVIEW_STANDALONE:
        emit tableViewClosed();
        event->accept();
        break;
    case NMTABVIEW_RASMETADATA:
        event->accept();
        break;
    case NMTABVIEW_PARATABLE:
        break;
    }
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
        connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &,
                                                         const QItemSelection &)),
                this, SLOT(updateProxySelection(const QItemSelection &,
                                                const QItemSelection &)));
    }
    else
    {
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
        this->updateProxySelection(mSelectionModel->getSelection(), QItemSelection());
    }
}

void
NMSqlTableView::setViewMode(ViewMode mode)
{
    switch(mode)
    {
    case NMTABVIEW_ATTRTABLE:
    case NMTABVIEW_STANDALONE:
    case NMTABVIEW_PARATABLE:
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
    NMDebugCtx(ctx, << "...");

    if (!this->mChkSelectedRecsOnly->isEnabled())
    {
        return;
    }

    updateSelection(mbSwitchSelection);

    NMDebugCtx(ctx, << "done!");
}

void NMSqlTableView::switchSelection()
{
    //NMDebugCtx(ctx, << "...");

    mbSwitchSelection = !mbSwitchSelection;
    updateSelection(mbSwitchSelection);


    //NMDebugCtx(ctx, << "done!");
}

void NMSqlTableView::normalise()
{
    // NOTE: for progress maxrange needs to be ncols * rows!

    NMDebugCtx(ctx, << "...");

    // -----------------------------------------------------------------------
    // get user input

    // get the names of the fields to normalise
    bool bOk = false;
    QString fieldNames = QInputDialog::getText(NMGlobalHelper::getMainWindow(),
                                              tr("Normalise Fields"),
                                              tr("List of Field Names (separated by whitespace):"), QLineEdit::Normal,
                                              tr(""), &bOk);
    if (!bOk || fieldNames.isEmpty())
    {
        NMDebugAI(<< "No input fields for normalisation specified!" << endl);
        NMDebugCtx(ctx, << "done!");
        return;
    }

    // ask for the criterion type
    QStringList slModes;
    slModes.append(tr("Cost Criterion"));
    slModes.append(tr("Benefit Criterion"));

    QString sMode = QInputDialog::getItem(NMGlobalHelper::getMainWindow(),
                                          tr("Normalisation Mode"),
                                          tr("Select mode"),
                                          slModes, 0, false, &bOk, 0);
    if (!bOk)
    {
        NMDebugAI(<< "No normalisation mode specified!" << endl);
        NMDebugCtx(ctx, << "done!");
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

    //prepareProgressDlg(calc.data(), "Normalise Columns ...", maxrange);
    QStringList normCols = calc->normaliseColumns(columnNames, bCostCriterion);
    //cleanupProgressDlg(calc.data(), maxrange);

    // since we've added columns, we want to make sure, that any selection is expanded to
    // those columns as well
    this->updateProxySelection(QItemSelection(), QItemSelection());

    // inform listeners
    //emit tableDataChanged(this->mAlteredColumns, this->mDeletedColumns);

    NMDebugCtx(ctx, << "done!");
}


void NMSqlTableView::userQuery()
{
    NMDebugCtx(ctx, << "...");

    if (mModel == 0)
    {
        return;
    }

    // ask for the name and the type of the new data field
    bool bOk = false;
    QSqlDriver* drv = mModel->database().driver();
    QString queryTemplate = QString("select * from %1")
            .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName));

    NMGlobalHelper helper;
    QString sqlStmt = helper.getMultiLineInput("Arbitrary SQL-SELECT Query",
                                               queryTemplate, this);

    if (sqlStmt.isEmpty())
    {
        NMDebugCtx(ctx, << "done!");
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

    const QString connname = NMGlobalHelper::getMainWindow()->getDbConnection(
                NMGlobalHelper::getMainWindow()->getSessionDbFileName());
    QSqlDatabase dbTarget = QSqlDatabase::database(connname);
    if (!dbTarget.isOpen())
    {
        NMBoxErr("User Query", "The session database is not open!");
        return;
    }


    QStringList sessionTables = dbTarget.tables();

    QMap<QString, QPair<otb::SQLiteTable::Pointer, QSharedPointer<NMSqlTableView> > >& tableList =
            NMGlobalHelper::getMainWindow()->getTableList();
    QStringList allTables = tableList.keys();

    while (allTables.contains(tableName) || sessionTables.contains(tableName))
    {
        tableName = QString("%1_%2").arg(queryName).arg(++mQueryCounter);
    }

    QSqlDriver* drv = mModel->database().driver();
    QString queryStr = QString("Create table %1 as %2")
            .arg(drv->escapeIdentifier(tableName, QSqlDriver::TableName))
            .arg(sql);

    QStringList externalDbs = NMGlobalHelper::identifyExternalDbs(dbTarget, sql);
    NMGlobalHelper::attachMultipleDbs(dbTarget, externalDbs);

    dbTarget.transaction();
    QSqlQuery userQuery(dbTarget);
    if (!userQuery.exec(queryStr))
    {
        userQuery.finish();
        dbTarget.rollback();
        NMGlobalHelper::detachMultipleDbs(dbTarget, externalDbs);
        NMBoxErr("User Query", userQuery.lastError().text().toStdString() << std::endl);
        NMDebugCtx(ctx, << "done!");
        return;
    }
    userQuery.finish();
    userQuery.clear();
    dbTarget.commit();
    NMGlobalHelper::detachMultipleDbs(dbTarget, externalDbs);
    NMGlobalHelper::getMainWindow()->createTableView(
                NMGlobalHelper::getMainWindow()->getSessionDbFileName(), tableName, tableName);

    //NMDebugCtx(ctx, << "done!");
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

void
NMSqlTableView::addRow()
{
    if (mSortFilter)
    {
        if (mSortFilter->addRow())
        {
            mlNumRecs = mSortFilter->getNumTableRecords();
            this->updateSelectionAdmin(mlNumRecs);
        }
    }
}

void
NMSqlTableView::addRows()
{
    bool bok = true;
    int nrows = QInputDialog::getInt(this, QString(tr("Add Rows ...")),
                                     QString(tr("Number of rows:")),
                                     1, 1, 2147483647, 1, &bok);
    if (!bok)
    {
        return;
    }

    if (mSortFilter)
    {
        if (mSortFilter->addRows(nrows))
        {
            mlNumRecs = mSortFilter->getNumTableRecords();
            this->updateSelectionAdmin(mlNumRecs);
        }
    }
}

void NMSqlTableView::indexColumn()
{
    int colidx = this->getColumnIndex(mLastClickedColumn);
    if (colidx < 0)
    {
        return;
    }

    if (!this->mSortFilter->createColumnIndex(colidx))
    {
        NMLogError(<< "Failed to index column '"
                   << mLastClickedColumn.toStdString() << "'!");
    }
    else
    {
        NMBoxInfo("Index Column", "'" << mLastClickedColumn.toStdString() << "' successfully indexed!");
    }
}

void NMSqlTableView::calcColumn()
{
    NMDebugCtx(ctx, << "...");

    // get user input

    QSqlDriver* drv = mModel->database().driver();
    QString label = QString(tr("UPDATE %1 SET %2 = ")
                            .arg(drv->escapeIdentifier(this->mModel->tableName(), QSqlDriver::TableName))
                            .arg(drv->escapeIdentifier(this->mLastClickedColumn, QSqlDriver::FieldName)));
    QString func = NMGlobalHelper::getMultiLineInput(label, "", this);
    if (func.isEmpty())
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QString error;
    int colidx = this->getColumnIndex(mLastClickedColumn);
    int rowsAffected = mSortFilter->updateData(colidx, mLastClickedColumn, func, error);
    if (!error.isEmpty())
    {
        NMBoxErr2(NMGlobalHelper::getMainWindow(), "Calculate Column", error.toStdString());
        NMDebugCtx(ctx, << "done!");
        return;
    }
    NMDebugAI(<< rowsAffected << " records updated!" << std::endl);
    updateProxySelection(QItemSelection(), QItemSelection());

    this->refreshTableView();

    NMDebugCtx(ctx, << "done!");
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
        if (mbHaveCoords)
        {
            this->mActZoom->setEnabled(true);
        }
    }
    else
    {
        this->mBtnClearSelection->setEnabled(false);
        this->mBtnSwitchSelection->setEnabled(false);
        this->mChkSelectedRecsOnly->setEnabled(false);
        this->mChkSelectedRecsOnly->setChecked(false);
        this->mActZoom->setEnabled(false);
    }
}

void
NMSqlTableView::updateSelectionAdmin(const QItemSelection& sel,
        const QItemSelection& desel)
{
    NMDebugCtx(ctx, << "...");
    if (this->mProxySelModel == 0)
    {
        NMDebugAI(<< "Haven't got any selectiom model set up!" << endl);
        this->updateSelectionAdmin(0);
        NMDebugCtx(ctx, << "done!");
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
    NMDebugCtx(ctx, << "done!");
}

void NMSqlTableView::addColumn()
{
    NMDebugCtx(ctx, << "...");

    NMAddColumnDialog* dlg = new NMAddColumnDialog(this);
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
        NMDebugCtx(ctx, << "done!");
        return;
    }

    delete dlg;

    //int ncols = this->mSortFilter->columnCount();
    int ncols = this->mModel->columnCount();

    NMDebugAI(<< "add column ? " << ret << endl);
    NMDebugAI(<< "name: " << name.toStdString() << endl);
    NMDebugAI(<< "type: " << type << endl);
    NMDebugAI(<< "ncols in tab: " << ncols << endl);

    if (mSortFilter->insertColumn(name, type))
    {
        updateSelection(mbSwitchSelection);
    }

    NMDebugCtx(ctx, << "done!");
}

void NMSqlTableView::deleteColumn()
{
    // we don't delete the primary key ... , oh no!
    if (mLastClickedColumn.compare(mPrimaryKey, Qt::CaseInsensitive) == 0)
    {
        NMBoxErr("Delete Column", "The primary key cannot be deleted!");
        return;
    }

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
    {
        return;
    }

    if (!mSortFilter->removeColumn(mLastClickedColumn))
    {
        NMBoxErr("Delete Column", "Failed to delete column!");
    }
    else
    {
        // update hidden columns
        mHiddenColumns.removeAll(mLastClickedColumn);
        foreach (const QString& col, mHiddenColumns)
        {
            this->hideAttribute(col);
        }
        updateSelection(mbSwitchSelection);
    }
}

void NMSqlTableView::joinAttributes()
{
    NMDebugCtx(ctx, << "...");

    // ============================================================
    //          List of TABLES : names & models
    // ============================================================

    NMGlobalHelper h;
    QStringList tableNameList;
    QMap<QString, NMSqlTableModel*> tableModelList;

    QMap<QString, QPair<otb::SQLiteTable::Pointer, QSharedPointer<NMSqlTableView> > > tableList =
            h.getMainWindow()->getTableList();
    //ModelComponentList* layerList = h.getMainWindow()->getLayerList();

    QMap<QString, QPair<otb::SQLiteTable::Pointer, QSharedPointer<NMSqlTableView> > >::iterator it =
            tableList.begin();
    while(it != tableList.end())
    {
        if (it.key().compare(this->windowTitle()) != 0)
        {
            tableNameList.append(it.key());
            tableModelList.insert(it.key(), qobject_cast<NMSqlTableModel*>(it.value().second->getModel()));
        }
        ++it;
    }

    // ============================================================
    //          LET THE USER PICK THE SOURCE TABLE
    // ============================================================

    bool bOk = false;
    QInputDialog ipd(this);
    ipd.setOption(QInputDialog::UseListViewForComboBoxItems);
    ipd.setComboBoxItems(tableNameList);
    ipd.setComboBoxEditable(false);
    ipd.setWindowTitle("Source Tables");
    ipd.setLabelText("Select the source table:");
    int ret = ipd.exec();

    QString tableName = ipd.textValue();

    if (!ret || tableName.isEmpty())
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    NMSqlTableModel* srcModel = 0;
    QMap<QString, NMSqlTableModel*>::iterator mit =
            tableModelList.find(tableName);
    if (mit != tableModelList.end())
    {
        srcModel = mit.value();
    }

    if (srcModel == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }

    QString srcDbFileName;
    QString srcTableName;
    QString srcConnName;
    NMSqlTableView* srcView = nullptr;
    QMap<QString, QPair<otb::SQLiteTable::Pointer, QSharedPointer<NMSqlTableView> > >::iterator srcIt =
            tableList.find(tableName);
    if (srcIt != tableList.end())
    {
        srcView = srcIt.value().second.data();
        if (srcView != nullptr)
        {
            QSqlTableModel* m = srcView->getModel();
            if (m != nullptr)
            {
                srcTableName = m->tableName();
                srcDbFileName = m->database().databaseName();
                srcConnName = m->database().connectionName();
            }
        }
    }

    // if we didn't get the table name from the view, we just use the one
    // we got earlier
    if (srcTableName.isEmpty())
    {
        srcTableName = tableName;
    }

    if (srcDbFileName.isEmpty())
    {
        // if we couldn't get the dbfilename from the otb::SQLiteTable,
        // we try the NMSqlTableModel, it should have it too ...
        // (NOTE: this is the case for standalone tables draged into
        // LUMASS
        srcDbFileName = srcModel->getDatabaseName();

        // however, if that didn't work either, we'd better
        // bail out at this point ...
        if (srcDbFileName.isEmpty())
        {
            NMDebugCtx(ctx, << "done!");
            return;
        }
    }


    // ==========================================================================
    //                  ASK FOR JOIN FIELDS
    // ==========================================================================


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
    bOk = false;
    QString tarFieldName = QInputDialog::getItem(this,
            tr("Select Target Join Field"), tr("Select Target Join Field"),
            tarJoinFields, 0, false, &bOk, 0);
    int tarJoinColIdx = tarJoinFields.indexOf(tarFieldName);
    if (!bOk || tarJoinColIdx < 0)
    {
        NMDebugCtx(ctx, << "done!");
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
        NMDebugCtx(ctx, << "done!");
        return;
    }
    NMDebugAI(<< "source field name=" << srcFieldName.toStdString()
              << " at index " << srcJoinColIdx << std::endl);


    NMDebugAI(<< "src data base: " << srcDbFileName.toStdString()
              << std::endl);


    // ======================================================================
    //              ask user which fields to join from the source
    // ======================================================================

    QStringList userJoinFields = NMGlobalHelper::getMultiItemSelection(
                "Select Fields", "Select source fields to join:",
                srcJoinFields, this);

    std::vector<std::string> vJnFields;
    foreach(const QString& f, userJoinFields)
    {
        vJnFields.push_back(f.toStdString());
    }

    if (vJnFields.size() == 0)
    {
        NMDebugCtx(ctx, << "done!");
        return;
    }


    // ======================================================================
    //             NMSqlTableView is doing the join
    // ======================================================================

    this->joinFields(tarFieldName,
                     srcConnName,
                     srcTableName,
                     srcFieldName,
                     userJoinFields);

    const QString tartabName = mModel->tableName();
    this->mModel->clear();
    this->mModel->setTable(tartabName);
    this->mModel->select();
    this->update();

    updateSelection(mbSwitchSelection);

    NMDebugCtx(ctx, << "done!");
}

bool
NMSqlTableView::joinFields(const QString &targetField,
                           const QString &srcConnName,
                           const QString &srcTable,
                           const QString &srcField,
                           QStringList &joinFields)
{
    bool ret = false;

    // ---------------- prepare read/write target DB with source DB attached --------------------------

    // get database connections
    if (!mSortFilter->openWriteModel())
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - no write connection to target table!");
        mSortFilter->openReadModel();
        return ret;
    }

    QSqlDatabase srcDb = QSqlDatabase::database(srcConnName, true);
    if (!srcDb.isValid() || !srcDb.isOpen())
    {
        NMLogError(<< ctx << "::" << __FUNCTION__ << "() - failed getting the source table!");
        mSortFilter->openReadModel();
        return ret;
    }

    QSqlDatabase tarDb = mModel->database();
    QSqlDriver* drv = tarDb.driver();

    // attach the source database
    const QString attachStr = QString("ATTACH DATABASE '%1' as srcdb")
            .arg(srcDb.databaseName());

    QSqlQuery qAttach(tarDb);
    if (!qAttach.exec(attachStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__
                   << "() - attaching source database failed : "
                   << qAttach.lastError().text().toStdString());
        qAttach.finish();
        mSortFilter->openReadModel();
        return ret;
    }
    qAttach.finish();

    // create a detach query
    const QString detachStr = QString("DETACH DATABASE srcdb");
    QSqlQuery qDetach(tarDb);

    // ---------------- list of source and final join fields --------------------------------------------

    // create escaped list of join fields
    QString srcFields;
    for (int f=0; f < joinFields.size(); ++f)
    {
        srcFields += drv->escapeIdentifier(joinFields.at(f), QSqlDriver::FieldName);
        if (f < joinFields.size()-1)
        {
            srcFields += ", ";
        }
    }

    // create a list of field names to be 'selected' from the tmp join table
    QStringList finalFieldList;
    for (int col=0; col < mModel->columnCount(); ++col)
    {
        QString colname = mModel->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
        finalFieldList << drv->escapeIdentifier(colname, QSqlDriver::FieldName);
    }
    QString finalFieldsStr = finalFieldList.join(',');
    finalFieldsStr += QStringLiteral(",") + srcFields;

    // add the source join field to source fields, if not already present
    // - can't do the join without the source join field!
    if (!joinFields.contains(srcField))
    {
        srcFields += QStringLiteral(",") + drv->escapeIdentifier(srcField, QSqlDriver::FieldName);
    }

    // ---------------- create a prelim (temp) joined table-----------------------------------------

    // temp join table
    QString tmpJoinStr = QString(
            "CREATE TEMP TABLE tmpjoin AS "
                "SELECT * FROM main.%1 "
                    "LEFT OUTER JOIN "
                        "(SELECT %2 FROM srcdb.%3) AS s "
                    "ON main.%1.%4 = s.%5;")
            .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName))
            .arg(srcFields)
            .arg(drv->escapeIdentifier(srcTable, QSqlDriver::TableName))
            .arg(drv->escapeIdentifier(targetField, QSqlDriver::FieldName))
            .arg(drv->escapeIdentifier(srcField, QSqlDriver::FieldName));

    QSqlQuery qTmpJoin(tarDb);
    if (!qTmpJoin.exec(tmpJoinStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__
                   << "() - failed creating temporary join table\nQUERY: "
                   << tmpJoinStr.toStdString() << "\n"
                   << qTmpJoin.lastError().text().toStdString());
        qTmpJoin.finish();
        qDetach.exec(detachStr);
        qDetach.finish();
        mSortFilter->openReadModel();
        return ret;
    }
    qTmpJoin.finish();

    // ---------------- create 'proper' join table and clear up  -----------------------------------

    // drop the current original table
    const QString dropOrigStr = QString("DROP TABLE main.%1;")
            .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName));
    QSqlQuery qDropOrig(tarDb);
    if (!qDropOrig.exec(dropOrigStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__
                   << "() - failed droping original target table! : "
                   << qDropOrig.lastError().text().toStdString());
        qDropOrig.finish();
        qDetach.exec(detachStr);
        qDetach.finish();
        mSortFilter->openReadModel();
        return ret;
    }
    qDropOrig.finish();

    // re-create target table from temp-joined table
    const QString finalTabStr = QString(
            "CREATE TABLE %1 "
                "AS SELECT %2 FROM tmpjoin;")
            .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName))
            .arg(finalFieldsStr);

    QSqlQuery qFinalTab(tarDb);
    if (!qFinalTab.exec(finalTabStr))
    {
        NMLogError(<< ctx << "::" << __FUNCTION__
                   << "() - failed re-creating original target table! : "
                   << qFinalTab.lastError().text().toStdString());
        qFinalTab.finish();
        qDetach.exec(detachStr);
        qDetach.finish();
        mSortFilter->openReadModel();
        return ret;
    }
    qFinalTab.finish();

    // ---------------- re-open target DB read-only  -----------------------------------

    // go back to readonly connection
    // this also auto deletes the TEMP tmpjoin table
    // and auto detaches the src database upon
    // closing the database connection
    mSortFilter->openReadModel();

    return true;
}


void NMSqlTableView::exportTable()
{
    NMDebugCtx(ctx, << "...");

    QString tabName = this->windowTitle().split(" ", QString::SkipEmptyParts).last();
    QString proposedName = QString(tr("%1/%2.csv")).arg(getenv("HOME")).arg(tabName);

    // take the first layer and save as vtkpolydata
    QString selectedFilter = tr("Delimited Text (*.csv)");
    NMGlobalHelper h;
    QString fileName = QFileDialog::getSaveFileName(h.getMainWindow(),
            tr("Export Table"), proposedName,
            tr("Delimited Text (*.csv)"), //;;SQLite Database (*.sqlite *.sdb *.db)"),
            &selectedFilter);
    if (fileName.isNull())
    {
        NMDebugAI( << "got an empty filename from the user!" << endl);
        NMDebugCtx(ctx, << "done!");
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
    //		NMErr(ctx, << "invalid database and table name!");
    //		return;
    //	}
    //
    //	this->writeSqliteDb(dbName, tableName, false);
    //}

    NMDebugCtx(ctx, << "done!");
}
void NMSqlTableView::colStats()
{
    NMDebugCtx(ctx, << "...");

    if (mLastClickedColumn.isEmpty() || mModel == 0)
    {
        return;
    }

    QMap<QString, QPair<otb::SQLiteTable::Pointer, QSharedPointer<NMSqlTableView> > >& tableList =
            NMGlobalHelper::getMainWindow()->getTableList();
    QStringList alltables = tableList.keys();

    QSqlDriver* drv = mModel->database().driver();
    QString tabName = mModel->tableName().replace("\"", "");
    QString name = QString("%1_%2_stats").arg(tabName).arg(mLastClickedColumn);
    int lfdnr = 2;
    while (alltables.contains(name))
    {
        name = QString("%1_%2_stats_%3").arg(mModel->tableName()).arg(mLastClickedColumn).arg(lfdnr);
        ++lfdnr;
    }

    std::string col = drv->escapeIdentifier(mLastClickedColumn, QSqlDriver::FieldName).toStdString();
    std::stringstream sql;

    sql << "select count(" << col << ") as count, "
        << "min(" << col << ") as minimum, "
        << "max(" << col << ") as maximum, "
        << "avg(" << col << ") as mean, "
        << "(sum(" << col << " * " << col << ") / count(" << col << ") "
        << "- (avg(" << col << ") * avg(" << col << "))) as stddev, "

        << "sum(" << col << ") as sum "
        << " from "
        << drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName).toStdString();

    if (mSortFilter->getSelCount())
    {
        sql << " where " << mSortFilter->getFilter().toStdString();
    }


    QString queryStr = QString("Create table %1 as %2")
            .arg(drv->escapeIdentifier(name, QSqlDriver::TableName))
            .arg(sql.str().c_str());

    const QString connname = NMGlobalHelper::getMainWindow()->getDbConnection(
                NMGlobalHelper::getMainWindow()->getSessionDbFileName());
    QSqlDatabase dbTarget = QSqlDatabase::database(connname);

    // if the current table is part of the session database, there's no need for attaching anything!
    bool bIsSessionDb = true;
    if (dbTarget.connectionName().compare(mModel->database().connectionName()) != 0)
    {
        if (!NMGlobalHelper::attachDatabase(dbTarget, mModel->database().databaseName(), name))
        {
            NMBoxErr("User Query", "Failed attaching the table database to the session database!");
            return;
        }
        bIsSessionDb = false;
    }

    QSqlQuery userQuery(dbTarget);
    if (!userQuery.exec(queryStr))
    {
        NMBoxErr("User Query", userQuery.lastError().text().toStdString() << std::endl);
        userQuery.finish();
        NMGlobalHelper::detachDatabase(dbTarget, name);
        NMDebugCtx(ctx, << "done!");
        return;
    }
    userQuery.finish();


    queryStr = QString("Select stddev from %1")
            .arg(drv->escapeIdentifier(name, QSqlDriver::TableName));

    dbTarget.transaction();
    QSqlQuery qRes(dbTarget);
    if (!qRes.exec(queryStr))
    {
         NMDebugCtx(ctx, << "done!");
         qRes.finish();
         dbTarget.rollback();
         NMGlobalHelper::detachDatabase(dbTarget, name);
         return;
    }
    if (!qRes.next())
    {
        NMDebugCtx(ctx, << "done!");
        qRes.finish();
        dbTarget.rollback();
        NMGlobalHelper::detachDatabase(dbTarget, name);
        return;
    }
    double var = qRes.value(0).toDouble();
    double sdev = std::sqrt(var);
    qRes.finish();
    dbTarget.commit();

    queryStr = QString("Update %1 set stddev = %2")
            .arg(drv->escapeIdentifier(name, QSqlDriver::TableName))
            .arg(sdev);

    dbTarget.transaction();
    QSqlQuery qUpd(dbTarget);
    if (!qUpd.exec(queryStr))
    {
        NMBoxErr("User Query", qUpd.lastError().text().toStdString() << std::endl);
        NMDebugCtx(ctx, << "done!");
        qUpd.finish();
        dbTarget.rollback();
        NMGlobalHelper::detachDatabase(dbTarget, name);
        return;
    }
    qUpd.finish();
    dbTarget.commit();
    ++mQueryCounter;

    NMGlobalHelper::detachDatabase(dbTarget, name);
    NMGlobalHelper::getMainWindow()->createTableView(
                NMGlobalHelper::getMainWindow()->getSessionDbFileName(), name, name);

    NMDebugCtx(ctx, << "done!");
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
    NMDebugCtx(ctx, << "...");

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        NMBoxErr("Export Table", "Couldn't create file '" << fileName.toStdString() << "'!");
        NMDebugCtx(ctx, << "done!");
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

    QSqlDatabase db = mModel->database();
    QSqlDriver* drv = db.driver();
    QString qStr = QString("select * from %1 %2")
                .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName))
                .arg(whereClause);


    db.transaction();
    QSqlQuery qTable(db);
    if (!qTable.exec(qStr))
    {
        NMBoxErr("Export Table", qTable.lastError().text().toStdString());
        file.close();
        qTable.finish();
        qTable.clear();
        db.commit();
        return false;
    }

    const int ncols = mModel->columnCount(QModelIndex());

    //mProgressDialog->setWindowModality(Qt::WindowModal);
    //mProgressDialog->setLabelText("Export Table ...");
    //mProgressDialog->setRange(0, maxrange);

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
        //mProgressDialog->setValue(progress);
        if (progress % 1000 == 0)
        {
            out.flush();
        }
    }

    //mProgressDialog->setValue(maxrange);
    out.flush();
    file.close();

    qTable.finish();
    qTable.clear();
    db.commit();

    NMDebugCtx(ctx, << "done!");
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

void
NMSqlTableView::processParaTableDblClick(QGraphicsSceneMouseEvent* gsme)
{
    QPoint pA = this->mapFromGlobal(gsme->screenPos());
    QPoint pB = this->mTableView->viewport()->mapFrom(this, pA);

    QScopedPointer<QMouseEvent> me(new QMouseEvent(
                                       QEvent::MouseButtonDblClick,
                                       pB, Qt::LeftButton,
                                       Qt::LeftButton, Qt::NoModifier));

    this->eventFilter(mTableView->viewport(), me.data());
}

void
NMSqlTableView::processParaTableRightClick(QGraphicsSceneMouseEvent* gsme, QGraphicsItem* gi)
{
    //NMDebugCtx(ctx, << "...");

    // translate the scene based event into a local table view one ...

    QGraphicsProxyWidget* pwi = qgraphicsitem_cast<QGraphicsProxyWidget*>(gi);
    if (pwi == 0)
    {
        return;
    }

    NMDebugAI(<< "objectName: " << pwi->objectName().toStdString() << std::endl);
    NMDebugAI(<< "windowTitle: " << this->windowTitle().toStdString() << std::endl);

    //QCursor::pos();

    QPoint localPos = this->mapFromGlobal(QCursor::pos());//this->mapFromGlobal(gsme->screenPos());

    QScopedPointer<QMouseEvent> me(new QMouseEvent(
                                       QEvent::MouseButtonPress,
                                       localPos, Qt::RightButton,
                                       Qt::RightButton, gsme->modifiers()));

    this->eventFilter(mTableView->horizontalHeader()->viewport(), me.data());

    //NMDebugCtx(ctx, << "done!");
}

bool
NMSqlTableView::eventFilter(QObject* object, QEvent* event)
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
            NMDebugAI(<< "SORTING COLUMN #" << col << "..." << std::endl);
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
                this->toggleRow(row);
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
                const QModelIndex ridx = mModel->index(row, 0, QModelIndex());
                const int srcRow = ridx.row();
                emit notifyLastClickedRow(static_cast<long long>(srcRow));
                this->mManageLayerMenu->move(me->globalPos());
                this->mManageLayerMenu->exec();
            }
            return true;
        }
        // ------------------------LEFT BUTTON DBL CLICK EDITS THE CELL ----------------------------------------
        else if (	me->button() == Qt::LeftButton
                 && event->type() == QEvent::MouseButtonDblClick)
        {
            if (this->mViewMode != NMSqlTableView::NMTABVIEW_RASMETADATA)
            {
                int row = this->mTableView->rowAt(me->pos().y());
                int col = this->mTableView->columnAt(me->pos().x());
                if (row >= 0 && col >= 0)
                {
                    QModelIndex idx = this->mModel->index(row, col, QModelIndex());
                    //mSortFilter->openWriteModel();
                    //mModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
                    if (mBtnEditCells->isChecked())
                    {
                        this->mTableView->edit(idx);
                    }
                    else
                    {
                        NMLogInfo(<< this->getTitle().toStdString()
                                  << ": Click the 'Edit Cells' button to enable cell editing!");
                    }
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
    NMDebugCtx(ctx, << "...");

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
    //updateProxySelection(QItemSelection(), QItemSelection());
    updateSelection(mbSwitchSelection);


    NMDebugCtx(ctx, << "done!");
}

void
NMSqlTableView::selectionQuery(void)
{
    NMDebugCtx(ctx, << "...");

    bool bOk = false;
    QSqlDriver* drv = mModel->database().driver();
    QString queryStr = QInputDialog::getText(this, tr("Selection Query"),
                         tr("Where Clause"), QLineEdit::Normal,
                         QString("%1 = ").arg(drv->escapeIdentifier(this->mLastClickedColumn, QSqlDriver::FieldName)),
                         &bOk);
    if (!bOk || queryStr.isEmpty())
    {
        NMDebugCtx(ctx, << "done!");
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

    NMDebugCtx(ctx, << "done!");
}

QString
NMSqlTableView::updateQueryString(bool swap)
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
        QSqlDriver* drv = mModel->database().driver();
        QString handPicked = QString("%1 in (").arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName));
        if (swap)
        {
            handPicked = QString("%1 not in (").arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName));
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

    return queryStr;
}

void
NMSqlTableView::updateSelection(bool swap)
{
    // udpate the query string
    QString queryStr = this->updateQueryString(swap);

    NMGlobalHelper h;
    h.startBusy();

    mbSelectionInProgress = true;
    if (mChkSelectedRecsOnly->isChecked())
    {
        mProxySelModel->clearSelection();
        mSortFilter->selectRows(queryStr, true);
        mSelectionModel->setSelection(mSortFilter->getSourceSelection());
        this->updateSelectionAdmin(mSortFilter->getSelCount());
    }
    else
    {
        mSortFilter->selectRows(queryStr, false);
        mProxySelModel->setSelection(mSortFilter->getProxySelection());
        mSelectionModel->setSelection(mSortFilter->getSourceSelection());
        this->updateSelectionAdmin(mSortFilter->getSelCount());
    }

    if (mSortFilter->getSelCount() == 0 && !mCurrentQuery.isEmpty())
    {
        this->clearSelection();
    }
    mbSelectionInProgress = false;
    h.endBusy();
}

void NMSqlTableView::clearSelection()
{
    mSortFilter->clearSelection();
    mProxySelModel->clearSelection();
    mSelectionModel->setSelection(QItemSelection());
    mSelectionModel->clearSelection();
    updateSelectionAdmin(0);
    mTableView->reset();
    mCurrentQuery.clear();
    mCurrentSwapQuery.clear();
    mPickedRows.clear();
    mChkSelectedRecsOnly->setChecked(false);
}

const QItemSelection
NMSqlTableView::getSelection()
{
    return this->mSelectionModel->getSelection();
}

void
NMSqlTableView::setSelection(const QItemSelection &isel)
{
    this->clearSelection();
    this->mSelectionModel->setSelection(isel);

//    QItemSelection dsel;
//    this->updateProxySelection(isel, dsel);
//    QItemSelection asel;
//    this->updateSelectionAdmin(asel, dsel);
}

void
NMSqlTableView::updateProxySelection(const QItemSelection& sel,
                                     const QItemSelection& desel)
{
    NMDebugCtx(ctx, << "...");

    // note: to actually update the proxy selection, we run the internal
    // selection process / method to ensure the mapping from source to proxy
    // indices is update-to-date; however this selection process (::updateSelection(bool))
    // also takes care of updating the source selection in mSelectionModel whose
    // 'selectionChanged' signal is hooked up to this very method here ...
    // long story short: to avoid an endless loop we track whether we're running
    // the internal selection process AT THE MOMENT, and if so, we bail out !!!
    if (mbSelectionInProgress)
    {
        NMDebugAI(<< "... already selection in progress ..."<< std::endl);
        NMDebugCtx(ctx, << "done!");
        return;
    }

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
    else if (mSelectionModel && desel.count() > 0)
    {
        this->clearSelection();
    }
    else
    {
        this->mProxySelModel->setSelection(mSortFilter->getProxySelection());
    }

    this->printSelRanges(this->mProxySelModel->selection(), "new selection ...");

    NMDebugCtx(ctx, << "done!");
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

    // =======================================================================
    // ... initial query ...
    // query the rowid of the source model for the given
    // row in the table view
    QModelIndex mir = mModel->index(row, 0);
    QVariant varData = this->mTableView->model()->data(mir);
    QString cellData;
    if (varData.type() == QVariant::String)
    {
        cellData = QString("'%1'").arg(varData.toString());
    }
    else
    {
        cellData = varData.toString();
    }

    QSqlDriver* drv = mModel->database().driver();
    QString rowQueryStr = QString("select %1 from %2 where %3 = %4")
                        .arg(drv->escapeIdentifier(mPrimaryKey, QSqlDriver::FieldName))
                        .arg(drv->escapeIdentifier(mModel->tableName(), QSqlDriver::TableName))
                        .arg(drv->escapeIdentifier(mSortFilter->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QSqlDriver::FieldName))
                        .arg(cellData);


    QSqlQuery rowQuery(mModel->database());
    if (!rowQuery.exec(rowQueryStr))
    {
        NMLogError(<< "NMSqlTableView::toggleRow(): "
                   << rowQuery.lastError().text().toStdString() << std::endl);
        rowQuery.finish();
        return;
    }

    if (!rowQuery.next())
    {
        NMLogError(<< "NMSqlTableView::toggleRow(): "
                   << rowQuery.lastError().text().toStdString() << std::endl);
        rowQuery.finish();
        return;
    }

    int therowid = rowQuery.value(0).toInt();

    // =======================================================================
    // ... subsequent queries, if required ...
    // just in case we did get multiple records ... which does happen ...

    int colcounter = 1;
    while (rowQuery.next())
    {
        rowQuery.finish();

        mir = mModel->index(row, colcounter);
        varData = this->mTableView->model()->data(mir);

        if (varData.type() == QVariant::String)
        {
            cellData = QString("'%1'").arg(varData.toString());
        }
        else
        {
            cellData = varData.toString();
        }

        rowQueryStr = QString("%1 and %2 = %3")
                            .arg(rowQueryStr)
                            .arg(drv->escapeIdentifier(mSortFilter->headerData(colcounter, Qt::Horizontal, Qt::DisplayRole).toString(), QSqlDriver::FieldName))
                            .arg(cellData);

        if (rowQuery.exec(rowQueryStr))
        {
            if (rowQuery.next())
            {
                therowid = rowQuery.value(0).toInt();
            }
        }

        ++colcounter;
    }
    rowQuery.finish();


    // =======================================================================
    // finalise
    mCurrentQuery.clear();
    mCurrentSwapQuery.clear();
    mbSwitchSelection = false;

    int srcRow = therowid;
    int idx = mPickedRows.indexOf(srcRow);
    if (idx == -1)
    {
        mPickedRows << srcRow;
    }
    else
    {
        mPickedRows.removeAt(idx);
    }
    emit notifyLastClickedRow(static_cast<long long>(srcRow));
    updateSelection(false);
}


