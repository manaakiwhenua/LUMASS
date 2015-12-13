/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013,2014 Landcare Research New Zealand Ltd
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
// system and standardSt
#include <limits>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <list>
#include <deque>
#include <map>
#include "math.h"

// GDAL support
#include "gdal.h"
#include "gdal_priv.h"


// NM stuff
#include "nmlog.h"
#include "NMMacros.h"
//#include "NMLayer.h"
#include "NMImageLayer.h"
#include "NMVectorLayer.h"
#include "NMMosra.h"
#include "NMTableView.h"
#include "NMSqlTableView.h"
//#include "NMChartWidget.h"
#include "NMImageReader.h"
#include "NMItk2VtkConnector.h"
#include "NMImageLayer.h"
#include "NMItkDataObjectWrapper.h"
#include "NMOtbAttributeTableWrapper.h"
#include "NMQtOtbAttributeTableModel.h"
#include "NMModelComponent.h"
#include "NMProcess.h"

#include "modelcomponentlist.h"
#include "NMProcCompList.h"
#include "NMFastTrackSelectionModel.h"
#include "NMRATBandMathImageFilterWrapper.h"
#include "NMStreamingImageFileWriterWrapper.h"
#include "NMModelComponentFactory.h"
#include "NMProcessFactory.h"
#include "NMModelSerialiser.h"
#include "NMEditModelComponentDialog.h"
#include "NMModelScene.h"
#include "NMWidgetListView.h"

#include "nmqsql_sqlite_p.h"
#include "nmqsqlcachedresult_p.h"
#include "otbSQLiteTable.h"

#include "LpHelper.h"

// QT stuff
#include "otbmodellerwin.h"
#include "ui_otbmodellerwin.h"
#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QPointF>
#include <QPointer>
#include <QtDebug>
#include <QInputDialog>
#include <QHash>
#include <QVariant>
#include <QTime>
#include <QDateTime>
#include <QTableView>
#include <QFuture>
#include <QtConcurrentRun>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QMessageBox>
#include <QPushButton>
#include <QDialog>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMetaType>
#include <QMetaObject>
#include <QMetaProperty>
#include <QFile>
#include <QtXml>
#include <QTextStream>
#include "qttreepropertybrowser.h"
#include "qteditorfactory.h"
#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>
#include <QScopedPointer>
#include <QSqlDriver>

// orfeo
//#include "ImageReader.h"
//#include "ImageDataWrapper.h"
#include "itkTimeProbe.h"
#include "itkIndent.h"
#include "otbGDALRATImageFileReader.h"
#include "otbImage.h"
#include "itkShiftScaleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkPasteImageFilter.h"
#include "otbBandMathImageFilter.h"
#include "otbRATBandMathImageFilter.h"
#include "itkVTKImageExport.h"
#include "otbAttributeTable.h"
#include "otbStreamingRATImageFileWriter.h"
#include "otbImageFileWriter.h"
#include "otbImageFileReader.h"
#include "itkExtractImageFilter.h"
#include "itkObjectFactoryBase.h"
#include "itkNMCostDistanceBufferImageFilter.h"
#include "otbProcessLUPotentials.h"
#include "itkRescaleIntensityImageFilter.h"
#include "otbImageIOBase.h"
#include "otbImage2DToCubeSliceFilter.h"
#include "otbMetaDataKey.h"
#include "itkMetaDataObject.h"
#include "otbFocalDistanceWeightingFilter.h"
#include "otbPotentialBasedAllocation.h"
#include "itkArray2D.h"
#include "itkRandomImageSource.h"
#include "otbSortFilter.h"
#include "otbImageRegionAdaptativeSplitter.h"
#include "itkVectorContainer.h"
#include "itkStreamingImageFilter.h"

// FOR ::test function
//#include "itkFloodFilledImageFunctionConditionalIterator.h"
//#include "itkBinaryThresholdImageFunction.h"
//#include "otbDEMSlopeAspectFilter.h"
//#include "otbFlowAccumulationFilter.h"

// VTK
#include "vtkNew.h"
#include "vtkAbstractArray.h"
#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDelimitedTextWriter.h"
#include "vtkDoubleArray.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleImage.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObject.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSQLDatabase.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkExtractPolyDataGeometry.h"
#include "vtkCylinder.h"
#include "vtkTransform.h"
#include "vtkSphere.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkImageWrapPad.h"
#include "vtkGreedyTerrainDecimation.h"
#include "vtkPolyDataNormals.h"
#include "vtkCoordinate.h"
#include "QVTKWidget.h"

#include "otbSortFilter.h"
#include "otbExternalSortFilter.h"
#include "itkImageRegionSplitterMultidimensional.h"
#include "itkNMImageRegionSplitterMaxSize.h"


#include <sqlite3.h>
#include "sqlite3extfunc.h"

#include <QSqlTableModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QProcess>
#include "NMSqlTableModel.h"

//// TOKYO CABINET
//#include "tcutil.h"
//#include "tchdb.h"
//#include <stdlib.h>
//#include <stdbool.h>
//#include <stdint.h>

//#include "valgrind/callgrind.h"

class NMMdiSubWindow : public QMdiSubWindow
{
    public:
        NMMdiSubWindow(QWidget* parent=0)
            : QMdiSubWindow(parent)
        {
        }
        ~NMMdiSubWindow()
        {
        }

        void closeEvent(QCloseEvent *closeEvent)
        {
            closeEvent->ignore();
            this->hide();
        }
};


OtbModellerWin::OtbModellerWin(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::OtbModellerWin)
{
    // **********************************************************************
    // *                    META TYPES and other initisalisations           *
    // **********************************************************************


#ifdef BUILD_RASSUPPORT
	this->mpRasconn = 0;
	this->mbNoRasdaman = true;
#endif
	this->mbNoRasdaman = false;
    this->mpPetaView = 0;

	//Qt::WindowFlags flags = this->windowFlags() | Qt::WA_DeleteOnClose;
	//this->setWindowFlags(flags);

	// some meta type registration for supporting the given types for
	// properties and QVariant
	qRegisterMetaType< QList< QStringList> >();
	qRegisterMetaType< QList< QList< QStringList > > >();
	qRegisterMetaType< NMItkDataObjectWrapper::NMComponentType >();
	qRegisterMetaType< NMProcess::AdvanceParameter >();
	qRegisterMetaType< NMLayer::NMLegendType >();
	qRegisterMetaType< NMLayer::NMLayerType >();
	qRegisterMetaType< NMLayer::NMLegendClassType >();
	qRegisterMetaType< NMLayer::NMColourRamp >();
#ifdef BUILD_RASSUPPORT
	qRegisterMetaType< NMRasdamanConnectorWrapper*>("NMRasdamanConnectorWrapper*");
#endif
	qRegisterMetaType<NMItkDataObjectWrapper>("NMItkDataObjectWrapper");
    //qRegisterMetaType<QSharedPointer<NMItkDataObjectWrapper> >("QSharedPointer<NMItkDataObjectWrapper>");
	qRegisterMetaType<NMOtbAttributeTableWrapper>("NMOtbAttributeTableWrapper");
    //qRegisterMetaType<NMDataComponent>("NMDataComponent");
    //qRegisterMetaType<NMDataComponent*>("NMDataComponent*");

    // **********************************************************************
    // *                    SQLITE3                                         *
    // **********************************************************************

    //sqlite3_auto_extension((void(*)(void))sqlite3_extmathstrfunc_init);
    sqlite3_temp_directory = getenv("HOME");

	// **********************************************************************
    // *                    MAIN WINDOW - MENU BAR AND DOCKS                *
	// **********************************************************************

	// set up the qt designer based controls
    ui->setupUi(this);

    // ================================================
    // INFO COMPONENT DOCK
    // ================================================

    QTableWidget* tabWidget = new QTableWidget(ui->infoWidgetList);
    tabWidget->setObjectName(QString::fromUtf8("layerInfoTable"));
    tabWidget->setAlternatingRowColors(true);
    ui->infoWidgetList->addWidgetItem(tabWidget, QString::fromUtf8("Layer Attributes"));

    mTreeCompEditor = new NMComponentEditor(ui->infoWidgetList);
    mTreeCompEditor->setObjectName(QString::fromUtf8("treeCompEditor"));
    ui->infoWidgetList->addWidgetItem(mTreeCompEditor, QString::fromUtf8("Component Properties"));

    ui->componentInfoDock->setMinimumWidth(160);
    ui->componentInfoDock->setVisible(true);

    // ================================================
    // MODEL COMPONENT DOCK (Source)
    // ================================================

    mLayerList = new ModelComponentList(ui->compWidgetList);
    mLayerList->setObjectName(QString::fromUtf8("modelCompList"));
    ui->compWidgetList->addWidgetItem(mLayerList, QString::fromUtf8("Map Layers"));

    NMProcCompList* procList = new NMProcCompList(ui->compWidgetList);
    procList->setObjectName(QString::fromUtf8("processComponents"));
    ui->compWidgetList->addWidgetItem(procList, QString::fromUtf8("Model Components"));

    ui->componentsWidget->setMinimumWidth(150);

    // ================================================
    // BAR(s) SETUP - MENU - PROGRESS - STATUS
    // ================================================

    this->mMapToolBar = new QToolBar("View Tools");

////    QAction* rubberBandZoom = new QAction("Zoom In", this->mMapToolBar);
////    rubberBandZoom->setCheckable(true);
////    rubberBandZoom->setChecked(false);
////    this->mMapToolBar->addAction(rubberBandZoom);

//    connect(rubberBandZoom, SIGNAL(triggered(bool)), this, SLOT(toggleRubberBandZoom(bool)));

    //this->mMdiArea = new QMdiArea(this);
    //this->setCentralWidget(mMdiArea);

    //this->mMdiArea->addSubWindow(this->ui->qvtkWidget);
//    NMMdiSubWindow* mapSub = new NMMdiSubWindow(mMdiArea);
//    this->mMapWindow = new QMainWindow(mapSub);
//    this->mMapWindow->setParent(this);
//    this->mMapWindow->setWindowTitle("Map View");
//    this->mMapWindow->setWindowFlags(Qt::Widget);
//    this->mMapWindow->setMouseTracking(true);
//    this->mMapWindow->addToolBar(this->mMapToolBar);
//    this->mMapWindow->setCentralWidget(this->ui->qvtkWidget);
//    mapSub->setWidget(mMapWindow);
//    mapSub->setWindowTitle(mMapWindow->windowTitle());


    // we remove the rasdaman import option, when we haven't
    // rasdaman suppor
#ifndef BUILD_RASSUPPORT
    ui->menuObject->removeAction(ui->actionImportRasdamanLayer);
#endif
#ifndef DEBUG
    //ui->menuMOSO->removeAction(ui->actionTest);
#endif

    // since we havent' go an implementation for odbc import
    // we just remove the action for now
    ui->menuObject->removeAction(ui->actionImportODBC);

    // add a label to the status bar for displaying
    // the coordinates of the map window
    this->m_coordLabel = new QLabel("", this, 0);
    this->mPixelValLabel = new QLabel("", this, 0);
    this->ui->statusBar->addWidget(m_coordLabel);
    this->ui->statusBar->addWidget(mPixelValLabel);

    // add a label to show random messages in the status bar
    this->mBusyProcCounter = 0;
    this->m_StateMsg = new QLabel("",  this, 0);
    this->ui->statusBar->addWidget(this->m_StateMsg);
    // progress bar
    this->mProgressBar = new QProgressBar(this);
    this->mProgressBar->setMaximumHeight(15);
    this->mProgressBar->setVisible(false);
    this->ui->statusBar->addPermanentWidget(mProgressBar, 1);

    // connect menu actions to member functions
#ifdef BUILD_RASSUPPORT
	connect(ui->actionImportRasdamanLayer, SIGNAL(triggered()), this, SLOT(loadRasdamanLayer()));
#endif
    connect(ui->actionLoad_Layer, SIGNAL(triggered()), this, SLOT(loadImageLayer()));
    connect(ui->actionImport_3D_Point_Set, SIGNAL(triggered()), this, SLOT(import3DPointSet()));
    connect(ui->actionToggle3DStereoMode, SIGNAL(triggered()), this, SLOT(toggle3DStereoMode()));
    connect(ui->actionToggle3DSimpleMode, SIGNAL(triggered()), this, SLOT(toggle3DSimpleMode()));
    connect(ui->actionLoad_VTK_PolyData, SIGNAL(triggered()), this, SLOT(loadVTKPolyData()));
    connect(ui->actionLoad_Vector_Layer, SIGNAL(triggered()), this, SLOT(loadVectorLayer()));
    connect(ui->actionMOSO, SIGNAL(triggered()), this, SLOT(doMOSO()));
    //connect(ui->actionMOSO_batch, SIGNAL(triggered()), this, SLOT(doMOSObatch()));
    connect(ui->actionComponents_View, SIGNAL(toggled(bool)), this, SLOT(showComponentsView(bool)));
    connect(ui->actionShow_Components_Info, SIGNAL(toggled(bool)), this, SLOT(showComponentsInfoView(bool)));
    //connect(ui->actionModel_View, SIGNAL(triggered()), this, SLOT(showModelView()));
    connect(ui->actionRemoveAllObjects, SIGNAL(triggered()), this, SLOT(removeAllObjects()));
    connect(ui->actionFullExtent, SIGNAL(triggered()), this, SLOT(zoomFullExtent()));
    connect(ui->actionSaveAsVTKPolyData, SIGNAL(triggered()), this, SLOT(saveAsVtkPolyData()));
    connect(ui->actionSave_As_Image_File, SIGNAL(triggered()), this ,SLOT(saveImageFile()));
    connect(ui->actionSave_Visible_Extent_Overview_As, SIGNAL(triggered()), this ,SLOT(saveImageFile()));
    connect(ui->actionTest, SIGNAL(triggered()), this, SLOT(test()));
    connect(ui->actionSaveAsVectorLayerOGR, SIGNAL(triggered()), this, SLOT(saveAsVectorLayerOGR()));
    connect(ui->actionImportODBC, SIGNAL(triggered()), this, SLOT(importODBC()));
    connect(ui->actionLUMASS, SIGNAL(triggered()), this, SLOT(aboutLUMASS()));
    connect(ui->actionBackground_Colour, SIGNAL(triggered()), this,
            SLOT(setMapBackgroundColour()));

    connect(ui->actionShow_Map_View, SIGNAL(toggled(bool)), this, SLOT(showMapView(bool)));
    connect(ui->actionShow_Model_View, SIGNAL(toggled(bool)), this, SLOT(showModelView(bool)));
    connect(ui->actionMap_View_Mode, SIGNAL(triggered()), this, SLOT(mapViewMode()));
    connect(ui->actionModel_View_Mode, SIGNAL(triggered()), this, SLOT(modelViewMode()));

    connect(ui->componentsWidget, SIGNAL(visibilityChanged(bool)),
            ui->actionComponents_View, SLOT(setChecked(bool)));
    connect(ui->componentInfoDock, SIGNAL(visibilityChanged(bool)),
            ui->actionShow_Components_Info, SLOT(setChecked(bool)));

    // TEST TEST TEST
    connect(ui->actionImage_Polydata, SIGNAL(triggered()), this, SLOT(Image2PolyData()));

    // **********************************************************************
	// *                    MODEL BUILDER WINDOW                            *
	// **********************************************************************

//    NMMdiSubWindow* modelSub = new NMMdiSubWindow(mMdiArea);


//    mModelBuilderWindow = new QMainWindow(modelSub);
    mModelBuilderWindow = new QMainWindow(this);
//    mModelBuilderWindow->setWindowTitle("Model Builder");
    mModelBuilderWindow->setWindowFlags(Qt::Widget);
    mModelBuilderWindow->setMouseTracking(true);
    mModelBuilderWindow->addToolBar(this->ui->mainToolBar);
    mModelBuilderWindow->setCentralWidget(ui->modelViewWidget);
//    modelSub->setWidget(mModelBuilderWindow);
//    modelSub->setWindowTitle(mModelBuilderWindow->windowTitle());


    //ui->modelViewWidget->setMouseTracking(true);
    //ui->modelViewWidget->setParent(this);

    //mModelBuilderWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    //mMapWindow->setAttribute(Qt::WA_DeleteOnClose, false);


//    this->mMdiArea->addSubWindow(modelSub);
//    this->mMdiArea->addSubWindow(mapSub);
//    this->mMdiArea->addSubWindow(this->ui->modelViewWidget);
//    this->mMdiArea->tileSubWindows();


//    QList<QMdiSubWindow*> wl = this->mMdiArea->subWindowList();
//    foreach (QMdiSubWindow* s, wl)
//    {
//        s->installEventFilter(this);
//        s->setWindowFlags(s->windowFlags() |
//                          Qt::CustomizeWindowHint |
//                          Qt::WindowMinMaxButtonsHint);
//    }

    // =============================================================
    // set up the tool bar
    this->ui->mainToolBar->setWindowTitle("Model Builder Tools");

    // .....................
    // zoom actions
    QIcon zoomInIcon(":zoom-in-icon.png");
    QAction* zoomInAction = new QAction(zoomInIcon, "Zoom In", this->ui->mainToolBar);
    zoomInAction->setAutoRepeat(true);

    QIcon zoomOutIcon(":zoom-out-icon.png");
    QAction* zoomOutAction = new QAction(zoomOutIcon, "Zoom Out", this->ui->mainToolBar);
    zoomOutAction->setAutoRepeat(true);

    QIcon zoomContentIcon(":zoom-fit-best-icon.png");
    QAction* zoomToContent = new QAction(zoomContentIcon, "Zoom To Content", this->ui->mainToolBar);

    // ..........................
    // component management actions
    QIcon moveIcon(":move-icon.png");
    QAction* moveAction = new QAction(moveIcon, "Move Scene",
            this->ui->mainToolBar);
    moveAction->setCheckable(true);
    moveAction->setChecked(false);

    QIcon linkIcon(":link-icon.png");
    QAction* linkAction = new QAction(linkIcon, "Link Components", this->ui->mainToolBar);
    linkAction->setCheckable(true);

//    QIcon selIcon(":select-icon.png");
//    QAction* selAction = new QAction(selIcon, "Select Components", this->ui->mainToolBar);
//    selAction->setCheckable(true);

//    QActionGroup* modelToolGroup = new QActionGroup(this->ui->mainToolBar);
//    modelToolGroup->addAction(moveAction);
//    modelToolGroup->addAction(linkAction);
//    modelToolGroup->addAction(selAction);

    // ..........................
    // model execution actions
    QIcon execIcon(":model-execute-icon.png");
    QAction* execAction = new QAction(execIcon, "Execute Model", this->ui->mainToolBar);

    QIcon stopIcon(":model-stop-icon.png");
    QAction* stopAction = new QAction(stopIcon, "Stop Model Execution", this->ui->mainToolBar);

    QIcon resetIcon(":model-reset-icon.png");
    QAction* resetAction = new QAction(resetIcon, "Reset Model",  this->ui->mainToolBar);


    //this->ui->mainToolBar->addActions(modelToolGroup->actions());
    this->ui->mainToolBar->addAction(linkAction);
    this->ui->mainToolBar->addSeparator();

    this->ui->mainToolBar->addAction(zoomInAction);
    this->ui->mainToolBar->addAction(zoomOutAction);
    this->ui->mainToolBar->addAction(zoomToContent);
    this->ui->mainToolBar->addAction(moveAction);

    this->ui->mainToolBar->addSeparator();
    this->ui->mainToolBar->addAction(resetAction);
    this->ui->mainToolBar->addAction(stopAction);
    this->ui->mainToolBar->addAction(execAction);


    // connect model view widget signals / slots
    connect(linkAction, SIGNAL(toggled(bool)),
    		this->ui->modelViewWidget, SIGNAL(linkToolToggled(bool)));
//    connect(selAction, SIGNAL(toggled(bool)),
//    		this->ui->modelViewWidget, SIGNAL(selToolToggled(bool)));
    connect(moveAction, SIGNAL(toggled(bool)),
            this->ui->modelViewWidget, SIGNAL(moveToolToggled(bool)));

    connect(zoomInAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomIn()));
    connect(zoomOutAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomOut()));
    connect(zoomToContent, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomToContent()));

    connect(execAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(executeModel()));
    connect(stopAction, SIGNAL(triggered()), this->ui->modelViewWidget, SIGNAL(requestModelAbortion()));
    connect(resetAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(resetModel()));


    // **********************************************************************
	// *                    VTK DISPLAY WIDGET                              *
	// **********************************************************************

	// suppress the vtkOutputWindow to pop up
#ifdef _WIN32
	vtkObject::GlobalWarningDisplayOff();
#endif

    // create the render window
    vtkSmartPointer<vtkRenderWindow> renwin = vtkSmartPointer<vtkRenderWindow>::New();
    renwin->SetStereoCapableWindow(1);
    renwin->SetStereoTypeToCrystalEyes();
    //renwin->SetStereoRender(1);
    //NMDebugAI(<< renwin->ReportCapabilities() << std::endl);

    // set the number of allowed layers in the window
    renwin->SetAlphaBitPlanes(1);
    renwin->SetMultiSamples(0);
    renwin->SetNumberOfLayers(2);

	// set-up the background renderer
	this->mBkgRenderer = vtkSmartPointer<vtkRenderer>::New();
	this->mBkgRenderer->SetLayer(0);
    //this->mBkgRenderer->SetBackground(1,1,1);
    this->mBkgRenderer->SetBackground(0.7,0.7,0.7);
	//this->mBkgRenderer->SetUseDepthPeeling(1);
	//this->mBkgRenderer->SetMaximumNumberOfPeels(100);
	//this->mBkgRenderer->SetOcclusionRatio(0.1);


    //renwin->SetStereoTypeToCrystalEyes();
    //this->mBkgRenderer->SetBackground(0,0,0);
	renwin->AddRenderer(this->mBkgRenderer);

    // set the render window
		// for supporting 3D mice (3DConnexion Devices)
		//this->ui->qvtkWidget->SetUseTDx(true);
    this->ui->qvtkWidget->SetRenderWindow(renwin);

    // listen in on events received by the QtVTKWidget
    this->ui->qvtkWidget->installEventFilter(this);


    this->m_b3D = false;
    this->ui->actionToggle3DStereoMode->setChecked(false);
    this->ui->actionToggle3DStereoMode->setEnabled(false);

	// display orientation marker axes
	vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
	axes->SetShaftTypeToLine();
	axes->SetXAxisLabelText("X");
	axes->SetYAxisLabelText("Y");
	axes->SetZAxisLabelText("Z");
	axes->SetTotalLength(10,10,10);
	axes->SetOrigin(0,0,0);

    this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
                vtkInteractorStyleImage::New());

	m_orientwidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
	m_orientwidget->SetOrientationMarker(axes);
	m_orientwidget->SetInteractor(static_cast<vtkRenderWindowInteractor*>(this->ui->qvtkWidget->GetInteractor()));
	m_orientwidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	m_orientwidget->InteractiveOff();
	m_orientwidget->SetEnabled(0);

    // QVTKWidget Events---------------------------------------------------------
    this->m_vtkConns = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::MouseMoveEvent,
    		this, SLOT(updateCoords(vtkObject*)));
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::LeftButtonPressEvent,
    		this, SLOT(pickObject(vtkObject*)));

    // **********************************************************************
    // *                    CENTRAL WIDGET                                  *
    // **********************************************************************

    QVBoxLayout* boxL = new QVBoxLayout();
    if (ui->centralWidget->layout())
        delete ui->centralWidget->layout();

    ui->centralWidget->setLayout(boxL);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(true);
    splitter->addWidget(ui->qvtkWidget);
    splitter->addWidget(mModelBuilderWindow);
    boxL->addWidget(splitter);

    // ================================================
    // INITIAL WIDGET's VISIBILITY
    // ================================================

    this->ui->componentInfoDock->setVisible(true);
    this->ui->actionShow_Components_Info->setChecked(true);

    this->ui->infoWidgetList->setWidgetItemVisible(1, false);

    this->ui->componentsWidget->setVisible(true);
    this->ui->actionComponents_View->setChecked(true);

    // set menu's check buttons to right state
    this->ui->actionShow_Map_View->setChecked(true);
    this->ui->actionShow_Model_View->setChecked(true);
}

OtbModellerWin::~OtbModellerWin()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

#ifdef BUILD_RASSUPPORT
	// close the table view and delete;
	if (this->mpPetaView != 0)
	{
		this->mpPetaView->close();
		delete this->mpPetaView;
	}


	if (this->mpRasconn)
		delete this->mpRasconn;
#endif

	NMDebugAI(<< "delete ui ..." << endl);
	delete ui;

//    const QObjectList& kids = this->children();
//    QObject *child;

//    NMDebugAI(<< "going to delete child objects ..." << endl);
//    foreach (child, kids)
//    {
//        if (child->metaObject()->className() != "NMMdiSubWindow")
//        {
//            NMDebugAI( << "... " << child->metaObject()->className() << endl);
//            delete child;
//            child = 0;
//        }
//    }

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

bool
OtbModellerWin::notify(QObject* receiver, QEvent* event)
{
	try
	{
		return qApp->notify(receiver, event);
	}
	catch (std::exception& e)
	{
		qDebug() << "Exception thrown: " << e.what() << endl;
	}

	return true;
}

bool
OtbModellerWin::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel && !this->m_b3D)
    {
        vtkInteractorStyleImage* iai =
                vtkInteractorStyleImage::SafeDownCast(
                    this->ui->qvtkWidget->GetInteractor()->GetInteractorStyle());

        QWheelEvent* we = static_cast<QWheelEvent*>(event);
        if (    we != 0
            &&  we->modifiers().testFlag(Qt::ControlModifier)
            &&  we->modifiers().testFlag(Qt::ShiftModifier)
           )
        {
            iai->SetMotionFactor(0.2);
        }
        else if (   we != 0
                 && we->modifiers().testFlag(Qt::ControlModifier)
                )
        {
            iai->SetMotionFactor(1.0);
        }
        else
        {
            iai->SetMotionFactor(10.0);
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (!ke)
            return false;

        vtkRenderWindow* renwin = this->ui->qvtkWidget->GetRenderWindow();
        if (!renwin)
            return false;

        if (    ke->modifiers().testFlag(Qt::ControlModifier)
            &&  ke->modifiers().testFlag(Qt::AltModifier)
            &&  this->m_b3D && renwin->GetStereoRender())
        {

            if (ke->key() == Qt::Key_C)
            {
                renwin->SetStereoTypeToCrystalEyes();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Crystal Eye" << std::endl);
            }
            else if (ke->key() == Qt::Key_Y)
            {
                renwin->SetStereoTypeToAnaglyph();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Anaglyph" << std::endl);
            }
            else if (ke->key() == Qt::Key_I)
            {
                renwin->SetStereoTypeToInterlaced();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Interlaced" << std::endl);
            }
            else if (ke->key() == Qt::Key_B)
            {
                renwin->SetStereoTypeToRedBlue();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to RedBlue" << std::endl);
            }
            else if (ke->key() == Qt::Key_G)
            {
                renwin->SetStereoTypeToSplitViewportHorizontal();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Split Viewport Horizontal" << std::endl);
            }
            else if (ke->key() == Qt::Key_D)
            {
                renwin->SetStereoTypeToDresden();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Dresden" << std::endl);
            }
            else if (ke->key() == Qt::Key_U)
            {
                renwin->SetStereoTypeToCheckerboard();
                renwin->StereoUpdate();
                NMDebugAI(<< "Stereo mode switched to Checkerboard" << std::endl);
            }
        }
    }
    else if (event->type() == QEvent::Close)
    {
        QMdiSubWindow* sub = qobject_cast<QMdiSubWindow*>(obj);
        if (!sub)
        {
            return false;
        }

        if (sub->windowTitle().compare("Map View") == 0)
        {
            this->showMapView(false);
            ui->actionShow_Map_View->setChecked(false);
        }
        else if (sub->windowTitle().compare("Model Builder") == 0)
        {
            this->showModelView(false);
            ui->actionShow_Model_View->setChecked(false);
        }
        return true;
    }

    return false;
}

void
OtbModellerWin::setMapBackgroundColour()
{
    double* bc = this->mBkgRenderer->GetBackground();
    QColor bclr;
    bclr.setRgbF(bc[0], bc[1], bc[2]);
    bclr = QColorDialog::getColor(bclr, this, "Background Colour");
    this->mBkgRenderer->SetBackground(bclr.redF(),
                                      bclr.greenF(),
                                      bclr.blueF());
}

vtkRenderWindow*
OtbModellerWin::getRenderWindow(void)
{
    return this->ui->qvtkWidget->GetRenderWindow();
}

void
OtbModellerWin::toggleRubberBandZoom(bool bzoom)
{
    if (this->m_b3D || !bzoom)
    {
        this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
                    vtkInteractorStyleImage::New());
    }
     else
    {
        this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
                    vtkInteractorStyleRubberBand2D::New());
    }
}

void
OtbModellerWin::Image2PolyData()
{
    NMDebugCtx(ctxOtbModellerWin, << "...");

    // check, whether we've got a selected image
    NMLayer* l = this->mLayerList->getSelectedLayer();
    if (l == 0)
    {
        NMDebugAI(<< "No layer selected!" << std::endl);
        NMDebugCtx(ctxOtbModellerWin, << "done!");
        return;
    }

    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (il == 0)
    {
        NMDebugAI(<< "No image layer selected!" << std::endl);
        NMDebugCtx(ctxOtbModellerWin, << "done!");
        return;
    }

    vtkDataSet* ids = const_cast<vtkDataSet*>(il->getDataSet());
    vtkImageData* id = vtkImageData::SafeDownCast(ids);
    double spacing[3];
    id->GetSpacing(spacing);
    int* extent = id->GetExtent();

    // pad the image by 1
    // to be able to create cells afterwards
    // note: image data stores points at cell
    // centres
    vtkNew<vtkImageWrapPad> pad;
    pad->SetInputData(ids);
    pad->SetOutputWholeExtent(
                extent[0], extent[1] + 1,
                extent[2], extent[3] + 1,
                extent[4], extent[5]);

    // convert to polydata
    vtkNew<vtkImageDataGeometryFilter> imgConv;
    //vtkNew<vtkGreedyTerrainDecimation> imgConv;
    imgConv->SetInputConnection(pad->GetOutputPort());

    // translate by -dx/2, -dy/2
    vtkNew<vtkTransform> transform;
    transform->Translate(-spacing[0]/2.0, -spacing[1]/2.0, 0);
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetTransform(transform.GetPointer());
    transformFilter->SetInputConnection(imgConv->GetOutputPort());
    transformFilter->Update();

    // get polygons
    vtkPolyData* pdraw = transformFilter->GetOutput();
    int ncells = pdraw->GetNumberOfCells();

    //  ----------------------------------------------------
    //  CREATE NEW POLYDATA OBJECTS
    //  ----------------------------------------------------
    vtkNew<vtkPolyData> pd;
    pd->Allocate(ncells);

    vtkNew<vtkCellArray> polys;
    polys->Allocate(ncells*5 + ncells);

    // create attr table
    vtkNew<vtkLongArray> nmid;
    nmid->SetName("nm_id");
    nmid->Allocate(ncells);

    vtkNew<vtkUnsignedCharArray> nmhole;
    nmhole->SetName("nm_hole");
    nmhole->Allocate(ncells);

    vtkNew<vtkUnsignedCharArray> nmsel;
    nmsel->SetName("nm_sel");
    nmsel->Allocate(ncells);

    vtkNew<vtkLongArray> polyid;
    polyid->SetName("PolyID");
    polyid->Allocate(ncells);


    //  ----------------------------------------------------
    //  COPY PTS AND CLOSE POLYS
    //  ----------------------------------------------------


    for (int p=0; p < ncells; ++p)
    {
        vtkCell* cell = pdraw->GetCell(p);
        const int npts = cell->GetNumberOfPoints();
        vtkIdList* ids = cell->GetPointIds();

        polys->InsertNextCell(npts+1);
        polys->InsertCellPoint(ids->GetId(0));
        for (int i=npts-1; i >= 0; --i)
        {
            polys->InsertCellPoint(ids->GetId(i));
        }

        // the attributes
        nmid->InsertNextValue(p+1);
        nmhole->InsertNextValue(0);
        nmsel->InsertNextValue(0);
        polyid->InsertNextValue(p+1);
    }

    pd->SetPoints(pdraw->GetPoints());
    pd->SetPolys(polys.GetPointer());
    pd->GetCellData()->SetScalars(nmid.GetPointer());
    pd->GetCellData()->AddArray(nmhole.GetPointer());
    pd->GetCellData()->AddArray(nmsel.GetPointer());
    pd->GetCellData()->AddArray(polyid.GetPointer());

    pd->BuildCells();
    pd->BuildLinks();

    //  ----------------------------------------------------
    //  ADD LAYER TO LUMASS
    //  ----------------------------------------------------

    NMVectorLayer* newPolys = new NMVectorLayer(this->getRenderWindow());
    QString name = il->objectName() + QString("_polys");
    newPolys->setObjectName(name);
    newPolys->setDataSet(pd.GetPointer());
    newPolys->setVisible(true);
    this->mLayerList->addLayer(newPolys);

    NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void
OtbModellerWin::mapViewMode()
{
    //ui->qvtkWidget->setVisible(true);
    ui->actionShow_Map_View->setChecked(true);
    this->showMapView(true);

    ui->actionShow_Model_View->setChecked(false);
    this->showModelView(false);
    //ui->modelViewWidget->parentWidget()->setVisible(false);

    ui->infoDock->setVisible(true);
    ui->actionShow_Components_Info->setChecked(true);

    ui->componentsWidget->setVisible(true);
    ui->actionComponents_View->setChecked(true);

    ui->compWidgetList->setWidgetItemVisible(0, true);
    ui->compWidgetList->setWidgetItemVisible(1, false);

    ui->infoWidgetList->setWidgetItemVisible(0, true);
    ui->infoWidgetList->setWidgetItemVisible(1, false);
}

void
OtbModellerWin::modelViewMode()
{
    //ui->qvtkWidget->setVisible(false);
    ui->actionShow_Map_View->setChecked(false);
    this->showMapView(false);

    //ui->modelViewWidget->parentWidget()->setVisible(true);
    ui->actionShow_Model_View->setChecked(true);
    this->showModelView(true);

    ui->componentsWidget->setVisible(true);
    ui->actionComponents_View->setChecked(true);

    ui->infoDock->setVisible(true);
    ui->actionShow_Components_Info->setChecked(true);

    ui->compWidgetList->setWidgetItemVisible(0, false);
    ui->compWidgetList->setWidgetItemVisible(1, true);

    ui->infoWidgetList->setWidgetItemVisible(0, false);
    ui->infoWidgetList->setWidgetItemVisible(1, true);
}

void
OtbModellerWin::showMapView(bool vis)
{
//    QList<QMdiSubWindow*> wl = this->mMdiArea->subWindowList();
//    foreach (QMdiSubWindow* s, wl)
//    {
//        if (s->windowTitle().compare("Map View") == 0)
//        {
//            vis ? ui->actionMap_View_Mode->isChecked()
//                  ? s->setWindowState(
//                        s->windowState() |
//                        Qt::WindowMaximized |
//                        Qt::WindowActive)
//                    : s->showNormal()
//                : s->hide();
//        }
//    }
    ui->qvtkWidget->setVisible(vis);
}

void
OtbModellerWin::showModelView(bool vis)
{
//    QList<QMdiSubWindow*> wl = this->mMdiArea->subWindowList();
//    foreach (QMdiSubWindow* s, wl)
//    {
//        if (s->windowTitle().compare("Model Builder") == 0)
//        {
//            vis ? ui->actionMap_View_Mode->isChecked()
//                    ? s->setWindowState(
//                          s->windowState() |
//                          Qt::WindowMaximized |
//                          Qt::WindowActive)
//                    : s->showNormal()
//                : s->hide();
//        }
//    }
    ui->modelViewWidget->parentWidget()->setVisible(vis);
}

const vtkRenderer*
OtbModellerWin::getBkgRenderer(void)
{
	return this->mBkgRenderer;
}

const NMComponentEditor*
OtbModellerWin::getCompEditor(void)
{
    if (mTreeCompEditor)
        return mTreeCompEditor;
    else
        return 0;
    //return this->ui->compEditor;
}

#ifdef BUILD_RASSUPPORT
RasdamanConnector*
OtbModellerWin::getRasdamanConnector(void)
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	// for now, this means that once rasdaman has been checked,
	// and was found to be unavailable, it won't be available
	// for the rest of the session, and the user has to close
	// and restart LUMASS - this is not really clever design
	// and needs revision --> TODO:
	if (mbNoRasdaman)
    {
        NMDebugCtx(ctxOtbModellerWin, << "done!");
		return 0;
    }

	if (this->mpRasconn == 0)
	{
		std::string connfile = std::string(getenv("HOME")) + "/.rasdaman/rasconnect";
		this->mpRasconn = new RasdamanConnector(connfile);
	}

	// get the sql for creating required views and functions to
	// facilitate metadata browsing
	std::string geospatial_fn = std::string(_lumass_binary_dir) +
			"/shared/ps_view_geometadata.sql";
	std::string extrametadata_fn = std::string(_lumass_binary_dir) +
			"/shared/ps_view_extrametadata.sql";
	std::string metadatatable_fn = std::string(_lumass_binary_dir) +
			"/shared/ps_func_metadatatable.sql";

	QFile geofile(QString(geospatial_fn.c_str()));
	bool fileopen = true;
	if (!geofile.open(QIODevice::ReadOnly | QIODevice::Text))
		fileopen = false;

	QTextStream in(&geofile);
	QString geosql(in.readAll());

	QFile extrafile(QString(extrametadata_fn.c_str()));
	if (!extrafile.open(QIODevice::ReadOnly | QIODevice::Text))
		fileopen = false;

	QTextStream in2(&extrafile);
	QString extrasql(in2.readAll());

	QFile tabfile(QString(metadatatable_fn.c_str()));
	if (!tabfile.open(QIODevice::ReadOnly | QIODevice::Text))
		fileopen = false;

	if (!fileopen)
	{
		NMErr(ctxOtbModellerWin, << "func_metadatatable: Failed installing petascope browsing support!");
		if (this->mpRasconn)
		{
			this->mpRasconn->disconnect();
			delete this->mpRasconn;
			this->mpRasconn = 0;
		}
        NMDebugCtx(ctxOtbModellerWin, << "done!");
		return 0;
	}

	QTextStream in3(&tabfile);
	QString tabsql(in3.readAll());

	// now execute the files
	std::string query = extrasql.toStdString() + geosql.toStdString() +
			   tabsql.toStdString();

	try
	{
		this->mpRasconn->connect();
	}
	catch (r_Error& raserr)
	{
		//NMBoxErr("Rasdaman Connection Error", raserr.what()
		//		 << ": Check whether the rasdaman database is up and running!");
		NMErr(ctxOtbModellerWin, << raserr.what());
		if (this->mpRasconn)
		{
			this->mpRasconn->disconnect();
			delete this->mpRasconn;
			this->mpRasconn = 0;
		}
		this->mbNoRasdaman = true;
        NMDebugCtx(ctxOtbModellerWin, << "done!");
		return 0;
	}

	const PGconn* conn = this->mpRasconn->getPetaConnection();
	PGresult* res = PQexec(const_cast<PGconn*>(conn), query.c_str());
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		NMBoxErr("Failed initialising rasdaman metadata browser!",
				 "Check whether Postgres is up and running and access is "
				"is configured properly!");
		NMErr(ctxOtbModellerWin, << "PQexec: Failed installing petascope browsing support!"
				                 << PQresultErrorMessage(res));
		NMDebugCtx(ctxOtbModellerWin, << "done!");
		return 0;
	}
	PQclear(res);

	return this->mpRasconn;
	NMDebugCtx(ctxOtbModellerWin, << "done!");
}
#endif

void OtbModellerWin::aboutLUMASS(void)
{
    QString vinfo = QString("Version %2.%3.%4 (%1)\nCommit %5\nLast updated %6")
                      .arg(_lumass_build_type)
                      .arg(_lumass_version_major)
                      .arg(_lumass_version_minor)
                      .arg(_lumass_version_revision)
                      .arg(_lumass_commit_hash)
                      .arg(_lumass_commit_date);

	QString year = QString(_lumass_commit_date).split(" ").at(4);
	QString title = tr("About LUMASS");
	stringstream textstr;
	textstr << "LUMASS - The Land-Use Management Support System" << endl
			<< vinfo.toStdString() << endl
			<< "Developed by Alexander Herzig" << endl
			<< "Copyright 2010-" << year.toStdString() << " Landcare Research New Zealand Ltd" << endl
			<< "www.landcareresearch.co.nz" << endl << endl
			<< "LUMASS is free software and licenced under the GPL v3." << endl
			<< "Contact: herziga@landcareresearch.co.nz" << endl
			<< "Code: http://code.scenzgrid.org/index.php/p/lumass/" << endl
            << "User group: https://groups.google.com/forum/#!forum/lumass-users" << endl
            << endl
            << "LUMASS builds on the following open source libraries " << endl
            << "Qt " << _lumass_qt_version << " - http://www.qt.io/" << endl
            << "OTB " << _lumass_otb_version << " - https://www.orfeo-toolbox.org/" << endl
            << "VTK " << _lumass_vtk_version << " - http://www.vtk.org/ " << endl
            << "GDAL " << _lumass_gdal_version << " - http://www.gdal.org/ " << endl
            << "lp_solve 5.5 - http://sourceforge.net/projects/lpsolve/ " << endl
            << "SQLite " << _lumass_sqlite_version << " - http://www.sqlite.org" << endl
#ifdef BUILD_RASSUPPORT
  #ifdef PACKAGE_STRING
            << PACKAGE_STRING
  #else
            << "Rasdaman ?.?.?"
  #endif
            << " - http://www.rasdaman.org/ " << endl
#endif
            << "";



    QString text(textstr.str().c_str());
	QMessageBox::about(this, title, text);

}

void OtbModellerWin::importODBC(void)
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

//	QString fileName = QFileDialog::getOpenFileName(this,
//	     tr("Import ODBC Data Base Table"), "~", tr("ODBC supported DB (*.*)"));
//	if (fileName.isNull()) return;
//
//	NMDebugAI( << "opening '" << fileName.toStdString() << "' ..." << endl);
//
//	QString dsn = QString("odbc://%1").arg(fileName);
//	NMDebugAI(<< "DSN: " << dsn.toStdString() << endl);

	// ask for the name and the type of the new data field
	bool bOk = false;
#ifdef BUILD_RASSUPPORT
	QString dsn = QInputDialog::getText(this, tr("ODBC Data Source Name"),
			tr("..."),
			QLineEdit::Normal, tr("psql://rasdaman:rasdaman@localhost:5432/RASBASE"), &bOk);
#else
	QString dsn = "";
#endif
	if (!bOk || dsn.isEmpty())
	{
		NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}

	vtkSmartPointer<vtkSQLDatabase> db =
			vtkSQLDatabase::CreateFromURL(dsn.toStdString().c_str());
	if (db.GetPointer() == 0)
	{
		NMErr(ctxOtbModellerWin, << "create from URL failed!");
		NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}

	if (!db->Open(0))
	{
		NMErr(ctxOtbModellerWin, << "couldn't open db!");
		NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}


//	QFileInfo finfo(fileName);
//	QString dbName = finfo.baseName();
//	NMDebugAI(<< "db name: " << dbName.toStdString() << endl);

	db->Print(std::cout);
	db->Close();



	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void OtbModellerWin::saveAsVectorLayerOGR(void)
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	// get the selected layer
    NMLayer* l = this->mLayerList->getSelectedLayer();
	if (l == 0)
		return;

	// make sure, we've got a vector layer
	if (l->getLayerType() != NMLayer::NM_VECTOR_LAYER)
		return;

	// get the vector layer
	NMVectorLayer* vL = qobject_cast<NMVectorLayer*>(l);

	// what kind of feature type are we dealing with?
	NMVectorLayer::NMFeatureType intype = vL->getFeatureType();
	NMDebugAI( << "the feature type is: " << intype << endl);

	// get a list of available drivers
	OGRRegisterAll();

	OGRSFDriverRegistrar* reg = OGRSFDriverRegistrar::GetRegistrar();

	QStringList sDriverList;
	for (int i=0; i < reg->GetDriverCount(); i++)
	{
		OGRSFDriver* driver = reg->GetDriver(i);
		sDriverList.append(driver->GetName());
	}

	// show a list of all available drivers and ask user which driver to use
	bool bOk = false;
	QString theDriverName = QInputDialog::getItem(this, tr("Save As Vector Layer"),
			tr("Select Data Format (Driver)"),
			sDriverList, 0, false, &bOk, 0);

	// if the user pressed cancel
	if (!bOk)
		return;

	NMDebugAI( << "chosen driver: " << theDriverName.toStdString() << endl);

	// ask the user for a filename
	QString sFileDlgTitle = tr("Save As ");
	sFileDlgTitle += theDriverName;

	QString fileName = QFileDialog::getSaveFileName(this,
			sFileDlgTitle, tr("~/") + l->objectName(), theDriverName);
	if (fileName.isNull())
		return;

	NMDebugAI( << "new file name is: " << fileName.toStdString() << endl);

    // create the data set
	// ToDo: adjust code for DB-based formats
	// (i.e. do we need port information, host, user, passwd, etc.)
	OGRSFDriver* theDriver = reg->GetDriverByName(theDriverName.toStdString().c_str());

	// create the new data source
	OGRDataSource* ds = theDriver->CreateDataSource(fileName.toStdString().c_str(), 0);

    switch(vL->getFeatureType())
    {
    case NMVectorLayer::NM_POLYGON_FEAT:
        this->vtkPolygonPolydataToOGR(ds, vL);
        break;
    default:
        NMDebugAI( << "Export of this feature type is not supported!" << std::endl);
        break;

    }


    // clean up OGR stuff
	// destroy the data source object
	OGRDataSource::DestroyDataSource(ds);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void
OtbModellerWin::saveImageFile()
{
    QObject* obj = this->sender();
    QAction* aobj = qobject_cast<QAction*>(obj);
    if (aobj->text().compare(QString("Save Visible Extent/Overview As ...")) == 0)
    {
        this->saveAsImageFile(true);
    }
    else
    {
        this->saveAsImageFile(false);
    }
}

void OtbModellerWin::saveAsImageFile(bool onlyVisImg)
{
    // get the selected layer
    NMLayer* l = this->mLayerList->getSelectedLayer();
    if (l == 0)
        return;

    // make sure, we've got a vector layer
    if (l->getLayerType() != NMLayer::NM_IMAGE_LAYER)
        return;

    // get the vector layer
    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);

    QString fileName = QFileDialog::getSaveFileName(this,
            QString("Save As Image File"),
            QString("~/%1.kea").arg(l->objectName()));
    if (fileName.isNull())
        return;

    NMModelController* ctrl = NMModelController::getInstance();

    // ---------------- SET UP INPUT ----------------------
    QSharedPointer<NMItkDataObjectWrapper> dw = il->getImage();
    NMDataComponent* dc = new NMDataComponent();
    dc->setObjectName("DataBuffer");
    QString bufCompName = ctrl->addComponent(dc);
    dc->setInput(dw);

    // ----------------- SET UP READER IF APPLICABLE--------
    QString srcFileName = il->getFileName();
    QFileInfo fifo(srcFileName);
    QString readerCompName;
    bool bReader = false;
    if (!onlyVisImg && fifo.isFile() && fifo.isReadable())
    {
        bReader = true;

        // SET UP THE READER Process
        NMImageReader* readerProc = new NMImageReader();
        readerProc->setFileNames(QStringList(srcFileName));
        readerProc->setImgTypeSpec(dw);

        NMSequentialIterComponent* readerComp =
                new NMSequentialIterComponent();
        readerComp->setObjectName("ImageReader");
        readerComp->setProcess(readerProc);
        readerCompName = ctrl->addComponent(readerComp);
    }

    // ---------------- SET UP WRITER ---------------------
    // create the process
    NMStreamingImageFileWriterWrapper* writerProc =
                new NMStreamingImageFileWriterWrapper();
    writerProc->setFileNames(QStringList(fileName));
    writerProc->setImgTypeSpec(dw);
    writerProc->setInputTables(QStringList(bufCompName));
    writerProc->setPyramidResamplingType(QString("NEAREST"));

    // create host component
    NMSequentialIterComponent* writerComp =
                new NMSequentialIterComponent();
    writerComp->setObjectName("ImageWriter");
    writerComp->setProcess(writerProc);

    QString writerCompName = ctrl->addComponent(writerComp);
    QList<QStringList> llst;
    QStringList lst;

    if (bReader)
    {
        lst << readerCompName;
    }
    else
    {
        lst << bufCompName;
    }

    llst << lst;
    writerComp->setInputs(llst);

    // ---- CONTROLLER DOES THE REST ------
    ctrl->executeModel(writerComp->objectName());
    QStringList del;
    del << bufCompName << writerCompName;
    if (!onlyVisImg)
    {
        del << readerCompName;
    }
    ctrl->deleteLater(del);

}

void OtbModellerWin::updateLayerInfo(NMLayer* l, double cellId)
{
	//NMDebugCtx(ctxOtbModellerWin, << "...");

    //QDockWidget* dw = qobject_cast<QDockWidget*>(this->ui->infoDock);
    QTableWidget* ti = this->ui->infoWidgetList->findChild<QTableWidget*>(
                QString::fromUtf8("layerInfoTable"));
    if (ti == 0)
    {
        NMWarn(ctx, << "Couldn't find 'layerInfoTable'!");
        return;
    }
	ti->clear();

	if (cellId < 0 && l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
        //dw->setWindowTitle(tr("Layer Info"));
		ti->setColumnCount(0);
		ti->setRowCount(0);
        //NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}

    //dw->setWindowTitle(QString(tr("Layer Info '%1'")).arg(
    //		l->objectName()));

	ti->setColumnCount(2);
	ti->horizontalHeader()->setStretchLastSection(true);

	QStringList colHeaderLabels;
	colHeaderLabels << QString(tr("Attributes (%1)").arg((long)cellId)) << "Value";

	ti->setHorizontalHeaderLabels(colHeaderLabels);

	// =====================================================================
	// 					READ VECTOR ATTRIBUTES
	// =====================================================================
	if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
		vtkDataSet* ds = const_cast<vtkDataSet*>(l->getDataSet());
		vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
		int nfields = dsAttr->GetNumberOfArrays();

		// we substract 2 fields from that list (i.e. nm_sel, nm_hole)
		ti->setRowCount(nfields-2);

		long rowcnt = 0;
		for (int r=0; r < nfields; ++r)
		{
			vtkAbstractArray* aa = dsAttr->GetAbstractArray(r);
			if (	aa == 0
				||  strcmp(aa->GetName(),"nm_sel") == 0
				||  strcmp(aa->GetName(),"nm_hole") == 0
			   )
			{
				continue;
			}

			QTableWidgetItem* item1 = new QTableWidgetItem(aa->GetName());
			ti->setItem(rowcnt,0, item1);

			QTableWidgetItem* item2;
			if ((long)cellId < aa->GetNumberOfTuples())
			{
				item2 = new QTableWidgetItem(
					aa->GetVariantValue((long)cellId).ToString().c_str());
			}
			else
			{
				item2 = new QTableWidgetItem("CellID invalid!");
			}
			ti->setItem(rowcnt,1, item2);
			++rowcnt;
		}
	}
	// =====================================================================
	// 					READ IMAGE ATTRIBUTES
	// =====================================================================
	else if (l->getLayerType() == NMLayer::NM_IMAGE_LAYER)
	{
		NMImageLayer* il = qobject_cast<NMImageLayer*>(l);

		// TODO: we have to reasonably extend this to any table
		// we've got
		otb::AttributeTable::Pointer tab = il->getRasterAttributeTable(1);
		//if (tab.IsNull())
		//{
		//	NMDebugAI(<< __FUNCTION__ << ": Couldn't fetch the image attribute table!" << std::endl);
		//	return;
		//}
		//QAbstractItemModel* tab = il->getTable();

		int ncols = tab.IsNull() ? 1 : tab->GetNumCols();
		ti->setRowCount(ncols);

		long rowcnt = 0;
		for (int r=0; r < ncols; ++r)
		{
			QTableWidgetItem *item1;
			std::string colname = tab.IsNull() ? "Value" : tab->GetColumnName(r);
			if (!colname.empty())
				item1 = new QTableWidgetItem(colname.c_str());
			else
				item1 = new QTableWidgetItem("");
			ti->setItem(r, 0, item1);
			QTableWidgetItem *item2;
			if (tab.IsNotNull())
			{
				if (cellId < tab->GetNumRows())
					item2 = new QTableWidgetItem(tab->GetStrValue(r, cellId).c_str());
				else
					item2 = new QTableWidgetItem("CellID invalid!");
			}
			else
			{
				item2 = new QTableWidgetItem(QString("%1").arg(cellId, 0, 'g'));
			}
			ti->setItem(r, 1, item2);
		}
	}

    connect(l, SIGNAL(destroyed()), ti, SLOT(clear()));
    ui->infoWidgetList->setWidgetItemVisible(0, true);
    ui->compDock->setVisible(true);

	//NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void OtbModellerWin::test()
{
    NMDebugCtx(ctxOtbModellerWin, << "...");

    // =======================================================================
    // get selected layer
    // =======================================================================

    NMLayer* l = this->mLayerList->getSelectedLayer();
    if (l == 0)
        return;

    QAbstractItemModel* tableModel = const_cast<QAbstractItemModel*>(l->getTable());

    QSqlTableModel* sqlModel = qobject_cast<QSqlTableModel*>(tableModel);
    if (sqlModel)
    {
        while(sqlModel->canFetchMore())
        {
            sqlModel->fetchMore();
        }
    }


    int nrows = tableModel->rowCount();
    int ncols = tableModel->columnCount();

    int stopIdx = 0;
    int idIdx = -1;
    int dnIdx = -1;

    QStringList colnames;
    QMap<QString, int> nameIdxMap;

    for (int i=0; i < ncols; i++)
    {
        QString colname = tableModel->headerData(i, Qt::Horizontal,
                                                 Qt::DisplayRole).toString();
        colnames.append(colname);
        nameIdxMap.insert(colname, i);
    }

    // =======================================================================
    // ask users for id columns
    // =======================================================================
    bool ok;
    QString idColName = QInputDialog::getItem(this, tr("Tree Check"),
                                             tr("ID column:"), colnames,
                                             0, false, &ok, 0);
    if (ok && !idColName.isEmpty())
    {
        idIdx = nameIdxMap.find(idColName).value();
    }

    colnames.removeOne(idColName);


    QString dnColName = QInputDialog::getItem(this, tr("Tree Check"),
                                             tr("DownID column:"), colnames,
                                             0, false, &ok, 0);
    if (ok && !dnColName.isEmpty())
    {
        dnIdx = nameIdxMap.find(dnColName).value();
    }


    if (idIdx == -1 || dnIdx == -1)
        return;


    // =======================================================================
    // initiate tree check
    // =======================================================================

    // -------------------------
    // create a hash map for looking up next down ids
    // to speed up things a bit ...
    NMDebugAI( << "memorizing where all the ")
    QMap<int, int> treeMap;
    for (int r=0; r < nrows; ++r)
    {
        const QModelIndex mid = tableModel->index(r, idIdx);
        const QModelIndex mdn = tableModel->index(r, dnIdx);
        const int id = tableModel->data(mid).toInt();
        const int dn = tableModel->data(mdn).toInt();
        treeMap.insert(id, dn);
    }

    // -------------------------------
    // let's get rolling ...
    QSet<int> allLoops;

    for (int r=0; r < nrows; ++r)
    {
        QList<int> idHistory;
        //if (!checkTree(r, idIdx, dnIdx, idHistory, tableModel, nrows))
        if (!checkTree(r, idHistory, treeMap))
        {
            allLoops.insert(idHistory.last());
            NMDebugAI(<< "loop in tree: " << r << " tail: ");
            QList<int> ph;
            for (int i=idHistory.size()-1; i >= 0; --i)
            {
                const int id = idHistory.at(i);
                NMDebug( << id << " ");
                if (ph.contains(id))
                {
                    break;
                }
                else
                {
                    ph << id;
                }
            }
            NMDebug(<< std::endl << std::endl);
        }
        else
        {
            NMDebugAI(<< "tree " << r << " is fine!" << std::endl);
        }
    }

    NMDebugAI(<< "all loop bottoms ... " << std::endl);
    foreach (const int& tail, allLoops)
    {
        NMDebug(<< tail << " ");
    }
    NMDebugAI(<< std::endl);

    NMDebugCtx(ctxOtbModellerWin, << "done!");
}


bool
OtbModellerWin::checkTree(const int& rootId, QList<int>& idHistory,
                          const QMap<int, int> &treeMap)
{
    // still everything fine!
    bool ret = true;

    const int dn = treeMap.find(rootId).value();
    if (dn == 0)
    {
        idHistory.append(dn);
        return true;
    }
    else
    {
        if (idHistory.contains(dn))
        {
            idHistory.append(dn);
            return false;
        }
        else
        {
            idHistory.append(dn);
            return checkTree(dn, idHistory, treeMap);
        }
    }

    return ret;

//    for (int r=0; r < nrows; ++r)
//    {
//        const QModelIndex midx = tableModel->index(r, idIdx);
//        int id = tableModel->data(midx).toInt();
//        if (id == rootId)
//        {
//            const QModelIndex midx2 = tableModel->index(id, treeMap);
//            int dn = tableModel->data(midx2).toInt();
//            if (dn == 0)
//            {
//                idHistory.append(dn);
//                return true;
//            }
//            else
//            {
//                if (idHistory.contains(dn))
//                {
//                    idHistory.append(dn);
//                    return false;
//                }
//                else
//                {
//                    idHistory.append(dn);
//                    return checkTree(dn, idIdx, treeMap, idHistory, tableModel, nrows);
//                }
//            }
//        }
//    }

    return ret;
}


/// only for debug and testing purposes
int
OtbModellerWin::sqlite_resCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    //NMDebugAI(<< "SQLite query results:" << std::endl);

    int i;
    for(i=0; i<argc; i++)
    {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");

    return 0;
}

void OtbModellerWin::zoomFullExtent()
{
    int nlayers = this->mLayerList->getLayerCount();
	if (nlayers <= 0)
		return;

	this->mBkgRenderer->ResetCamera(const_cast<double*>(
            this->mLayerList->getMapBBox()));

	this->ui->qvtkWidget->update();
}

void OtbModellerWin::removeAllObjects()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

    int nlayers = this->mLayerList->getLayerCount();
	int seccounter = 0;
	while(nlayers > 0)
	{
        NMLayer* l = this->mLayerList->getLayer(nlayers-1);
        this->mLayerList->removeLayer(l->objectName());
        nlayers = this->mLayerList->getLayerCount();

		if (seccounter > 100) break;
		seccounter++;
	}

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void OtbModellerWin::pickObject(vtkObject* obj)
{
    // picking implementation only properly works in 2d mode
    if (m_b3D)
    {
        return;
    }

    // get interactor
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
			obj);

	if (iren->GetShiftKey())
		return;

	// get event position
	int event_pos[2];
	iren->GetEventPosition(event_pos);

	// get the selected layer or quit
    NMLayer* l = this->mLayerList->getSelectedLayer();
	if (l == 0)
		return;

	if (!l->isSelectable())
		return;

	double wPt[4];

	//	vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
	//	picker->Pick(event_pos[0], event_pos[1], 0, const_cast<vtkRenderer*>(l->getRenderer()));
	//	vtkIdType pickedId = picker->GetCellId();

	vtkInteractorObserver::ComputeDisplayToWorld(this->mBkgRenderer,
			event_pos[0], event_pos[1], 0, wPt);
	wPt[2] = 0;

	//	NMDebugAI(<< "wPt: " << wPt[0] << ", " << wPt[1] << ", " << wPt[2] << endl);

	double cellId = -1;
	// ==========================================================================
	// 									VECTOR PICKING
	// ==========================================================================
	if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
		if (vl->getFeatureType() != NMVectorLayer::NM_POLYGON_FEAT)
			return;

		vtkDataSet* ds = const_cast<vtkDataSet*>(l->getDataSet());
		vtkDataSetAttributes* dsAttr = ds->GetAttributes(vtkDataSet::CELL);
		vtkDataArray* nmids = dsAttr->GetArray("nm_id");
		vtkDataArray* hole = dsAttr->GetArray("nm_hole");
		vtkDataArray* sa = dsAttr->GetArray("nm_sel");

		vtkPolyData* pd = vtkPolyData::SafeDownCast(ds);
		vtkPoints* cellPoints = pd->GetPoints();
		int ncells = pd->GetNumberOfCells();

		vtkCell* cell;
		int subid, inout;
		double pcoords[3], clPt[3], dist2;
		double* weights = new double[pd->GetMaxCellSize()];
		bool in = false;
		QList<vtkIdType> vIds;
		QList<vtkIdType> holeIds;
		QList<vtkIdType> vnmIds;

        //NMDebugAI(<< "analysing cells ..." << endl);
		for (long c = 0; c < ncells; c++)
		{
			if (hole->GetTuple1(c) == 0)
			{
				//		NMDebugAI( << "cell (nmid) " << nmids->GetTuple1(c) << ":" << endl);
				cell = pd->GetCell(c);
                in = this->ptInPoly2D(wPt, cell);
//                vtkPolygon* poly = vtkPolygon::SafeDownCast(cell);
//                double n[3];
//                double* pts = static_cast<vtkDoubleArray*>(cell->GetPoints()->GetData())->GetPointer(0);
//                poly->ComputeNormal(poly->GetNumberOfPoints(), pts, n);
//                in = vtkPolygon::PointInPolygon(
//                            wPt, cell->GetNumberOfPoints(),
//                            pts,
//                            cell->GetBounds(),
//                            n);
                if (in)
				{
					vIds.push_back(c);
					vnmIds.push_back(nmids->GetTuple1(c));
					// don't really what this is for anymore
					if (hole->GetTuple1(c) == 1)
						  holeIds.push_back(c);
				}
			}
		}

		//	// remove holes from result vector
		//	for (int k=0; k < vIds.size(); ++k)
		//	{
		//		if (holeIds.contains(vIds.at(k)))
		//		{
		//			vIds.removeOne(vIds.at(k));
		//		}
		//	}

		if (vIds.size() > 1)
		{
			NMDebug(<< endl);
			NMDebugAI(<< "WARNING - multiple hits - cellIds: ");

			foreach(const int &id, vIds)
				NMDebug(<< id << " ");
			NMDebug(<< endl);

			NMDebugAI(<< "                          - nmids: ");
			foreach(const int &id, vnmIds)
				NMDebug(<< id << " ");
			NMDebug(<< endl);
		}

		// the doc widget hosting the layer info table

		QList<long> lstCellId;
		QList<long> lstNMId;
		if (vIds.size() == 0)
		{
            // this is mean if you're in a tedious selection process
            // and only because the bloody picking algorithm doesn't
            // work, you've got to start again - so we don't do it;
            // note, the user can deselect via the table view
            //l->selectCell(0, NMLayer::NM_SEL_CLEAR);
			this->updateLayerInfo(l, -1);
			return;
		}

		cellId = vIds.last();
		long nmid = nmids->GetTuple1(cellId);
		lstCellId.push_back(cellId);
		lstNMId.push_back(nmid);

        // select cell if the user pressed the Ctrl key,
        // otherwise just show the layer info,
        // this allows the user to be able to inspect
        // selected cells - much more useful this way round,
        // I guess ... ?
        if (iren->GetControlKey())
        {
            l->selectCell(cellId, NMLayer::NM_SEL_ADD);
        }
        else
        {
            this->updateLayerInfo(l, cellId);
        }
	}
	// ==========================================================================
	// 									PIXEL PICKING
	// ==========================================================================
	else
	{
		NMImageLayer *il = qobject_cast<NMImageLayer*>(l);
		if (il->getRasterAttributeTable(1).IsNull())
					return;

		vtkImageData *img = vtkImageData::SafeDownCast(
				const_cast<vtkDataSet*>(l->getDataSet()));

        //int* ext = img->GetExtent();

		if (img == 0)
			return;

		int did[3] = {-1,-1,-1};
        il->world2pixel(wPt, did, false, true);

        // normally, we're dealing with 2D images anyway,
        //did[2] = did[2] < 0 ? 0 : did[2];

        for (unsigned int d=0; d < 2; ++d)
		{
			if (did[d] < 0)
			{
				this->updateLayerInfo(l, -1);
				l->selectCell(cellId, NMLayer::NM_SEL_CLEAR);
				return;
			}
		}

        vtkDataArray* idxScalars = img->GetPointData()->GetArray(0);
        void* idxPtr = img->GetArrayPointer(idxScalars, did);

        switch(img->GetPointData()->GetArray(0)->GetDataType())
        {
        vtkTemplateMacro(getDoubleFromVtkTypedPtr(
                             static_cast<VTK_TT*>(idxPtr), &cellId)
                    );
        default:
            NMWarn(ctxOtbModellerWin, << "Scalar pointer type not supported!");
        }

		NMLayer::NMLayerSelectionType seltype;
		if (iren->GetControlKey())
			seltype = NMLayer::NM_SEL_ADD;
		else
			seltype = NMLayer::NM_SEL_NEW;

		// populate layer info table with currently picked cell
		this->updateLayerInfo(l, cellId);
		l->selectCell(cellId, seltype);
	}
}

template<class T>
void
OtbModellerWin::getDoubleFromVtkTypedPtr(T* in, double* out)
{
    *out = static_cast<double>(*in);
}

bool
OtbModellerWin::ptInPoly2D(double pt[3], vtkCell* cell)
{
//	NMDebugCtx(ctxOtbModellerWin, << "...");

	// although getting a 3-pt, we're only dealing with x/y coordinates;
	// ray function used: y = ax + b
	// here we're testing whether the ray 'y = 0x + pt[1]' intersects with
	// the line segments of the polygon and count the intersections
	bool ret = false;

	pt[2] = 0;
	double bnd[6];
    double delta[] = {0,0,0};
	cell->GetBounds(bnd);
	bnd[4] = -1;
	bnd[5] = 1;

    if (!vtkMath::PointIsWithinBounds(pt, bnd, delta))
        return false;

	// alloc memory
	double* x = new double[2]; // the load vector taking the 'b' coefficient of the ray function
	double** a = new double*[2];
	for (int i=0; i < 2; ++i)
		a[i] = new double[2];

    //        NMDebugAI(<< "test point: " << pt[0] << " " << pt[1] << " "
    //                  << pt[3] <<  endl);

	// we assume here, that the points are ordered either anti-clockwise or clockwise
	// iterate over polygon points, create line segments
	double s1[2];
	double s2[2];
	double bs;
	double as;
	double segbnd[4];
    double dy, dx;
	int onseg = 0;
	int retcnt = 0;
	for (int i=0; i < cell->GetNumberOfPoints()-1; ++i)
	{
		// get points
		s1[0] = cell->GetPoints()->GetPoint(i)[0];
		s1[1] = cell->GetPoints()->GetPoint(i)[1];
		s2[0] = cell->GetPoints()->GetPoint(i+1)[0];
		s2[1] = cell->GetPoints()->GetPoint(i+1)[1];

		segbnd[0] = (s1[0] < s2[0]) ? s1[0] : s2[0];
		segbnd[1] = (s1[0] > s2[0]) ? s1[0] : s2[0];
		segbnd[2] = (s1[1] < s2[1]) ? s1[1] : s2[1];
		segbnd[3] = (s1[1] > s2[1]) ? s1[1] : s2[1];


		// calc as and bs
        // avoid 0 slope of ray
        dy = s2[1] - s1[1];
        dx = s2[0] - s1[0];


        a[1][1] = 1;                                // coefficient for y of polygon segment
        if (dx == 0)
        {
            bs = s1[0];
            as = -1;
            a[1][1] = 0;
        }
        else
        {
            as = dy / dx;
            bs = s1[1] - (as * s1[0]);
        }

		// fill load vector and parameter matrix
		x[0] = pt[1];								// b of test ray
		x[1] = bs;      							// b of polygon segment
        a[0][0] = 0; 		 a[0][1] = 1;			// coefficients for x and y of test ray
        a[1][0] = as * (-1);            	    	// coefficient for x

        //		// DEBUG
        //        NMDebugAI(<< "S-" << i+1 << ": " << "S1(" << s1[0] << ", " << s1[1] << ") "
        //                  << "S2(" << s2[0] << ", " << s2[1] << "): y = " << as << "x + "
        //                  << bs << std::endl);

        //        NMDebugAI(<< "      Linear System: ..." << endl);
        //        NMDebugAI(<< "      " << x[0] << " = " << a[0][0] << " " << a[0][1] << endl);
        //        NMDebugAI(<< "      " << x[1] << " = " << a[1][0] << " " << a[1][1] << endl);

		onseg = 0;
		if (vtkMath::SolveLinearSystem(a, x, 2))
		{
			if ( x[0] >= pt[0] &&
				 ((x[0] >= segbnd[0] && x[0] <= segbnd[1]) &&	// test segment boundary
				  (x[1] >= segbnd[2] && x[1] <= segbnd[3])   )    )
			{
				onseg = 1;
				++retcnt;
			}
		}
        //        NMDebug( << "\t\thit: " << onseg << endl);

	}
    //    NMDebugAI(<< "total hits: " << retcnt << endl << endl);

	// check whether retcnt is odd (=inside) or even (=outside)
	if (retcnt > 0 && retcnt % 2 != 0)
		ret = true;

	// release memory
	delete[] x;
	for (int i=0; i < 2; ++i)
		delete[] a[i];
	delete[] a;

	//NMDebugCtx(ctxOtbModellerWin, << "done!");
	return ret;
}

void
OtbModellerWin::updateCoordLabel(const QString& newCoords)
{
	this->m_coordLabel->setText(newCoords);
}

//void
//OtbModellerWin::setCurrentInteractorLayer(const NMLayer* layer)
//{
////	for (int i=0; i < mLayerList->getLayerCount(); ++i)
////	{
////		NMLayer* l = const_cast<NMLayer*>(mLayerList->getLayer(i));
////		vtkRenderer* ren = const_cast<vtkRenderer*>(l->getRenderer());
////		ren->SetInteractive(0);
////	}
////
////	NMLayer* la = const_cast<NMLayer*>(layer);
////	vtkRenderer* renA = const_cast<vtkRenderer*>(la->getRenderer());
////	renA->SetInteractive(1);
//
//}

void OtbModellerWin::updateCoords(vtkObject* obj)
{
	// get interactor
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(
			obj);

	// get event position
	int event_pos[2];
	iren->GetEventPosition(event_pos);

	double wPt[4];
	vtkInteractorObserver::ComputeDisplayToWorld(this->mBkgRenderer,
			event_pos[0], event_pos[1], 0, wPt);
	wPt[2] = 0;

	// update label
	QString s = QString("Map Location: X: %1 Y: %2"). // Z: %3").
	arg(wPt[0], 0, 'f', 5).arg(wPt[1], 0, 'f', 5); //.arg(wPt[3],0,'f',2);

	this->m_coordLabel->setText(s);

    // generated pixel information is not meaningful in 3d, so
    // lets skip that
    if (m_b3D)
    {
        this->mPixelValLabel->setText("");
        return;
    }

    // =======================================================================================
	// get pixel value, if image layer is selected

	// update the pixel value label, if we've got an image layer selected
    NMLayer *l = this->mLayerList->getSelectedLayer();
	if (l == 0)
		return;

	if (l->getLayerType() != NMLayer::NM_IMAGE_LAYER)
		return;

	vtkImageData* img = vtkImageData::SafeDownCast(
			const_cast<vtkDataSet*>(l->getDataSet()));

	if (img == 0)
		return;


    int ext[6];
    double orig[3];
    img->GetOrigin(orig);
    img->GetExtent(ext);

	NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
	QString pixval = "";
    int did[3] = {-1,-1,-1};
    int lprpix[3] = {-1,-1,-1};

    il->world2pixel(wPt, did, false, false);
    il->world2pixel(wPt, lprpix, true, false);

    stringstream cvs;
    if (    (did[0] >= ext[0] && did[0] <= ext[1])
         && (did[1] >= ext[2] && did[1] <= ext[3])
         && (did[2] >= ext[4] && did[2] <= ext[5])
       )
    {
        for (unsigned int d=0; d < img->GetNumberOfScalarComponents(); ++d)
        {
            cvs << img->GetScalarComponentAsDouble(did[0], did[1], did[2], d)
                    << " ";
        }
    }
    else
    {
        cvs << "nodata ";
    }

    pixval = QString(" Pixel(%1, %2, %3) = %4 | LPRPixel(%5, %6, %7)").  // | Displ(%8, %9) | Orig(%10, %11)").
                arg(did[0]).arg(did[1]).arg(did[2]).
                arg(cvs.str().c_str()).
                arg(lprpix[0]).arg(lprpix[1]).arg(lprpix[2]);//.
                //                arg(event_pos[0]).arg(event_pos[1]).
                //                arg(orig[0], 0, 'f', 0).
                //                arg(orig[1], 0, 'f', 0);

	this->mPixelValLabel->setText(pixval);
}

void
OtbModellerWin::showBusyStart()
{
    if (this->sender())
    {
        NMDebugAI(<< this->sender()->objectName().toStdString() << " - turned busy!" << std::endl);
    }
    QString msg = QString("processing ..."); //.arg(this->sender()->objectName());
	this->m_StateMsg->setText(msg);
	this->mProgressBar->reset();
    this->mProgressBar->setMinimum(0);
    this->mProgressBar->setMaximum(0);
    this->mProgressBar->setVisible(true);
    this->mBusyProcCounter++;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void
OtbModellerWin::showBusyEnd()
{
    this->mBusyProcCounter--;
    if (mBusyProcCounter == 0)
    {
        if (this->sender())
        {
            NMDebugAI(<< this->sender()->objectName().toStdString() << " - stopped processing!" << std::endl);
        }
        this->m_StateMsg->setText("");
        this->mProgressBar->setVisible(false);
    }
    QApplication::restoreOverrideCursor();
}

void
OtbModellerWin::showBusyValue(int value)
{
    if (this->mBusyProcCounter)
    {
        this->mProgressBar->setValue(value);
    }
}

void OtbModellerWin::showComponentsView(bool vis)
{
    this->ui->componentsWidget->setVisible(vis);
}

void OtbModellerWin::showComponentsInfoView(bool vis)
{
    ui->infoDock->setVisible(vis);
    ui->componentInfoDock->setVisible(vis);

    // a bit of a dirty hack to avoid ill-aligned display
    // of the layer info table widget; should be really resolved in
    // NMWidgetListView
    bool livis = ui->infoWidgetList->getWidgetItem(0)->isVisible();
    ui->infoWidgetList->setWidgetItemVisible(0, true);
    ui->infoWidgetList->setWidgetItemVisible(0, false);
    ui->infoWidgetList->setWidgetItemVisible(0, livis);
}


//void OtbModellerWin::doMOSObatch()
//{
//	return;

//	NMDebugCtx(ctxOtbModellerWin, << "...");

//	QString fileName = "/home/alex/projects/HBRC_EnviroLink/sensitivity/scenario_files/r1_minNleach_ConstAgrProd.los";
//	QString dsName = "/home/alex/projects/HBRC_EnviroLink/sensitivity/data/r1sens.vtk";
//	//QString fileName = QFileDialog::getOpenFileName(this,
//	//     tr("Open Optimisation Settings"), "~", tr("LUMASS Optimisation Settings (*.los)"));
//    //
//	//if (fileName.isNull())
//	//{
//	//	NMDebugAI( << "Please provide a filename!" << endl);
//	//	return;
//	//}

//	QFileInfo fileinfo(fileName);
//	QFileInfo dsInfo(dsName);

//	QString path = fileinfo.path();
//	QString baseName = fileinfo.baseName();
//	if (!fileinfo.isReadable())
//	{
//		NMErr(ctxNMMosra, << "Could not read file '" << fileName.toStdString() << "'!");
//		return;
//	}

//	// create a new optimisation object
//	NMMosra* mosra = new NMMosra(this);

//	for (int runs=5; runs < 7; ++runs)
//	{
//		NMDebugAI(<< "******** PERTURBATION #" << runs+1 << " *************" << endl);
//		// load the file with optimisation settings
//		mosra->loadSettings(fileName);

//		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
//		reader->SetFileName(dsName.toStdString().c_str());
//		reader->Update();
//		vtkPolyData* pd = reader->GetOutput();
//		mosra->setDataSet(pd);
//		mosra->perturbCriterion("Nleach", 5);
//		vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();

//		mosra->setTimeOut(180);
//		if (!mosra->solveLp())
//			continue;

//		if (!mosra->mapLp())
//			continue;

//        vtkSmartPointer<vtkTable> chngmatrix;
//        vtkSmartPointer<vtkTable> sumres = mosra->sumResults(chngmatrix);

//		// get rid of admin fields
//		tab->RemoveColumnByName("nm_id");
//		tab->RemoveColumnByName("nm_hole");
//		tab->RemoveColumnByName("nm_sel");

//		// now write the input and the result table
//		QString perturbName = QString("%1/%2_p%3.csv").arg(dsInfo.path())
//				.arg(dsInfo.baseName()).arg(runs+1);

//		QString resName = QString("%1/res_%2_p%3.csv").arg(dsInfo.path())
//						.arg(dsInfo.baseName()).arg(runs+1);

//		vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
//		writer->SetFieldDelimiter(",");

//        writer->SetInputData(tab);
//		writer->SetFileName(perturbName.toStdString().c_str());
//		writer->Update();

//        writer->SetInputData(sumres);
//		writer->SetFileName(resName.toStdString().c_str());
//		writer->Update();

//		writer->Delete();
//	}

//	NMDebugCtx(ctxOtbModellerWin, << "done!");
//}

void OtbModellerWin::doMOSO()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Open Optimisation Settings"), "~", tr("LUMASS Optimisation Settings (*.los)"));

	if (fileName.isNull())
	{
		NMDebugAI( << "Please provide a filename!" << endl);
		return;
	}

	QFileInfo fileinfo(fileName);

    //QDir parentDir = fileinfo.absoluteDir();
    //parentDir.cdUp();
    QString path = fileinfo.absoluteDir().path();
	QString baseName = fileinfo.baseName();
	if (!fileinfo.isReadable())
	{
		NMErr(ctxNMMosra, << "Could not read file '" << fileName.toStdString() << "'!");
        NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}

    QString tresPath = ::getenv("MOSO_RESULT_PATH");
    if (tresPath.isEmpty())
    {
        tresPath = path;
    }
    QFileInfo pathInfo(tresPath);
    QDir parentDir(pathInfo.path());

    QStringList dirList;
    dirList << "opt_report" << "opt_lucmatrix" << "opt_result"
            << "opt_relative" << "opt_total" << "opt_lp" << "opt_tab";

    for (int d=0; d < dirList.size(); ++d)
    {
        QString dir = dirList.at(d);
        NMDebugAI(<< dirList.at(d).toStdString() << " -> ");
        if (!parentDir.exists(dir))
        {
            if (!parentDir.mkdir(dir))
            {
                dir = fileinfo.dir().dirName();
                dirList.replace(d, dir);
            }
        }
        NMDebug(<< dirList.at(d).toStdString() << endl );
    }

	// create a new optimisation object
	NMMosra* mosra = new NMMosra(this);

	// load the file with optimisation settings
	mosra->loadSettings(fileName);

	// look for the layer mentioned in the settings file
    NMLayer* layer = this->mLayerList->getLayer(mosra->getLayerName());
	if (layer == 0)
	{
		NMDebugAI( << "couldn't find layer '" << mosra->getLayerName().toStdString() << "'" << endl);
		delete mosra;
		return;
	}

	// now set the layer, do moso and clean up
	//mosra->setLayer(layer);
    mosra->setDataSet(layer->getDataSet());
    mosra->setTimeOut(0); // clears any time out setting ...
    mosra->setBreakAtFirst(true);
    //NMDebugAI(<< "split off solving to seperate thread ... " << endl);

    QDateTime started = QDateTime::currentDateTime();

    //QFuture<int> future = QtConcurrent::run(mosra, &NMMosra::solveLp);

    //    NMDebugAI(<< "waiting 10 secs ..." << endl);
    //    ::sleep(10);

    //    QMessageBox::StandardButton btn = QMessageBox::question(this, "Abort Optimisation?",
    //                    "Press 'Yes' to abort the current optimisation run!");
    //    if (btn == QMessageBox::Yes)
    //    {
    //        NMDebugAI(<< "cancel solving ..." << endl);
    //        mosra->cancelSolving();
    //    }

    //    // asking the user for the lp timeout
    //    bool timeok;
    //    int timeout = QInputDialog::getInt(this, tr("lp_solve timeout"), tr("timeout in secs:"), 60, 5, 86400, 30, &timeok);
    //    if (!timeok)
    //        return;
    //    mosra->setTimeOut(timeout);

    QString sRepName = QString(tr("%1/%2/report_%3.txt")).arg(pathInfo.path())
            .arg(dirList.at(0))
            .arg(baseName);

    //if (!future.result())
    if (!mosra->solveLp())
	{
        NMDebugAI(<< "We encountered trouble setting/solving the problem or the optimisation was aborted!" << std::endl);
        NMDebugAI( << "write report to '" << sRepName.toStdString() << "'" << endl);
        mosra->writeReport(sRepName);

        delete mosra;
		return;
	}

    QDateTime stopped = QDateTime::currentDateTime();
    int msec = started.msecsTo(stopped);
    int min = msec / 60000;
    double sec = (msec % 60000) / 1000.0;

    QString elapsedTime = QString("%1:%2").arg((int)min).arg(sec,0,'g',3);
    NMDebugAI(<< "Optimisation took (min:sec): " << elapsedTime.toStdString() << endl);
    NMMsg(<< "Optimisation took (min:sec): " << elapsedTime.toStdString() << endl);


	// ============================================================================
    NMDebugAI( << "write report to '" << sRepName.toStdString() << "'" << endl);
    mosra->writeReport(sRepName);


	int solved = mosra->mapLp();
	//layer->emitDataSetChanged();
	layer->tableDataChanged(QModelIndex(), QModelIndex());


	if (solved)
	{
        // ==========================================================================
        // SUMMARISE THE DATA BEFORE WE MESS AROUND WITH IT
        // once we've got a feasible result, we write the change matrix and
        // the optimisation result table out
        vtkSmartPointer<vtkTable> changeTab;
        vtkSmartPointer<vtkTable> resTab = mosra->sumResults(changeTab);

        // =========================================================================
        // export attribute table

        vtkSmartPointer<vtkTable> origtab = mosra->getDataSetAsTable();
        vtkSmartPointer<vtkTable> tab = vtkSmartPointer<vtkTable>::New();
        tab->DeepCopy(origtab);

        vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
                    tab->GetColumnByName("nm_hole"));

        // filter 'hole' polygons
        int nrows = tab->GetNumberOfRows();
        int r = 0;
        while(r < nrows)
        {
            if (hole->GetValue(r))
            {
                tab->RemoveRow(r);
                --nrows;
            }
            else
            {
                ++r;
            }
        }

        // get rid of admin fields
        tab->RemoveColumnByName("nm_id");
        tab->RemoveColumnByName("nm_hole");
        tab->RemoveColumnByName("nm_sel");

        // setup the writer
        vtkSmartPointer<vtkDelimitedTextWriter> writer =
                vtkSmartPointer<vtkDelimitedTextWriter>::New();
        writer->SetFieldDelimiter(",");

        QString tabName = QString("%1/%2/tab_%3.csv").arg(pathInfo.path())
                .arg(dirList.at(6))
                .arg(baseName);

        writer->SetInputData(tab);
        writer->SetFileName(tabName.toStdString().c_str());
        writer->Update();

        // =========================================================================
		NMDebugAI( << "visualising optimisation results ..." << endl);


        QString resName = QString("%1/%2/res_%3.csv").arg(pathInfo.path())
                .arg(dirList.at(2))
                .arg(baseName);

		// show table if we got one
		if (resTab.GetPointer() != 0)
		{
            NMDebugAI(<< "writing optimisation results -> '"
                      << resName.toStdString() << "'" << std::endl);
            writer->SetInputData(resTab);
            writer->SetFileName(resName.toStdString().c_str());
            writer->Update();

            int ncols = resTab->GetNumberOfColumns();
            int numCri = (ncols-1)/4;

            // create table only containing the absolute performance
            // after this optimisation run
            vtkSmartPointer<vtkTable> optTab = vtkSmartPointer<vtkTable>::New();
            optTab->AddColumn(resTab->GetColumn(0));
            for (int i=0, off=2; i < numCri; ++i, off += 4)
            {
                optTab->AddColumn(resTab->GetColumn(off));
            }

            QString totalName = QString("%1/%2/tot_%3.csv").arg(pathInfo.path())
                    .arg(dirList.at(4))
                    .arg(baseName);
            writer->SetInputData(optTab);
            writer->SetFileName(totalName.toStdString().c_str());
            writer->Update();


            // create table only containing relative changes (%)
            // from original result table
            for (int i=0; i < numCri; ++i)
            {
                for (int re=0; re < 3; ++re)
                {
                    resTab->RemoveColumn(i+1);
                }
            }

            QString relName = QString("%1/%2/rel_%3.csv").arg(pathInfo.path())
                    .arg(dirList.at(3))
                    .arg(baseName);
            writer->SetInputData(resTab);
            writer->SetFileName(relName.toStdString().c_str());
            writer->Update();

            vtkQtTableModelAdapter* tabModel = new vtkQtTableModelAdapter(this);
            NMFastTrackSelectionModel* ftsm = new NMFastTrackSelectionModel(tabModel, this);
            tabModel->setTable(resTab);
            NMTableView* tv = new NMTableView(tabModel, 0);
            tv->setSelectionModel(ftsm);
            tv->setTitle(tr("Performance Change!"));
            tv->show();
		}

        if (changeTab.GetPointer() != 0)
        {
            vtkQtTableModelAdapter* tabModel = new vtkQtTableModelAdapter(this);
            NMFastTrackSelectionModel* ftsm = new NMFastTrackSelectionModel(tabModel, this);
            tabModel->setTable(changeTab);
            NMTableView* tv = new NMTableView(tabModel, 0);
            tv->setSelectionModel(ftsm);
            tv->setTitle(tr("Change Matrix!"));
            tv->show();

            QString chgName = QString("%1/%2/chg_%3.csv").arg(pathInfo.path())
                    .arg(dirList.at(1))
                    .arg(baseName);

            NMDebugAI(<< "writing change matrix -> '"
                      << chgName.toStdString() << "'" << std::endl);
            writer->SetInputData(changeTab);
            writer->SetFileName(chgName.toStdString().c_str());
            writer->Update();
        }

        // obviously, we have to prepare the table a bit better
        // TODO: need to switch the NMChartWidget to VTK's new chart framework!
        //		QStandardItemModel* model = this->prepareResChartModel(resTab);
        //		if (model != 0)
        //		{
        //			NMChartWidget* cw = new NMChartWidget(this);
        //			cw->setChartModel(model);
        //			cw->setWinTitle(tr("Optimisation Change Chart"));
        //			cw->show();
        //		}
	}

    QString lpName = QString(tr("%1/%2/lp_%3.lp")).arg(pathInfo.path())
            .arg(dirList.at(5))
            .arg(baseName);
	mosra->getLp()->WriteLp(lpName.toStdString());
	delete mosra;

	if (solved)
	{
		NMDebugAI( << "mapping unique values ..." << endl);
        //        NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(layer);
        //        vl->mapUniqueValues(tr("OPT_STR"));
		layer->setLegendType(NMLayer::NM_LEGEND_INDEXED);
		layer->setLegendClassType(NMLayer::NM_CLASS_UNIQUE);
		layer->setLegendValueField("OPT_STR");
		layer->setLegendDescrField("OPT_STR");
        //layer->updateLegend();
        layer->updateMapping();
	}

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

QStandardItemModel*
OtbModellerWin::prepareResChartModel(vtkTable* restab)
{
	if (restab == 0)
		return 0;

	NMDebugCtx(ctxOtbModellerWin, << "...");


	int nDestCols = restab->GetNumberOfRows();
	int nSrcCols = restab->GetNumberOfColumns();
	int nDestRows = (nSrcCols-1) / 4;

	QStandardItemModel* model = new QStandardItemModel(nDestRows, nDestCols, this->parent());
	model->setItemPrototype(new QStandardItem());

	NMDebugAI( << "populating table ..." << endl);

	QStringList slVHeaderLabels;
	int srccol = 4;
	for (int row=0; row < nDestRows; ++row, srccol+=4)
	{
		QString sVHeader = restab->GetColumnName(srccol);
		slVHeaderLabels.append(sVHeader);
		model->setVerticalHeaderItem(row, new QStandardItem());
		model->verticalHeaderItem(row)->setData(QVariant((int)row*40), Qt::DisplayRole);

		for (int col=0; col < nDestCols; ++col)
		{
			if (row == 0)
			{
				QString sHHeader = restab->GetValue(col, 0).ToString().c_str();
				model->setHorizontalHeaderItem(col, new QStandardItem());
				model->horizontalHeaderItem(col)->setData(QVariant(sHHeader), Qt::DisplayRole);
			}

			model->setItem(row, col, new QStandardItem());
			model->item(row, col)->setData(QVariant(restab->GetValue(col, srccol).ToDouble()),
					Qt::DisplayRole);
		}
	}
	model->setVerticalHeaderLabels(slVHeaderLabels);

	NMDebugCtx(ctxOtbModellerWin, << "done!");

	return model;

}

void OtbModellerWin::displayChart(vtkTable* srcTab)
{

// NMChartWidget needs switching to VTK 6's new chart framework
//	NMChartWidget* cw = new NMChartWidget(srcTab, this);
//	cw->show();
}

void OtbModellerWin::loadVTKPolyData()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Open XML/Binary VTK PolyData File"), "~",
	     tr("PolyData (*.vtp *.vtk)"));

	if (fileName.isNull())
		return;

	vtkSmartPointer<vtkPolyData> pd;

	//check, what kind of format we've got
	if (fileName.endsWith(".vtp", Qt::CaseInsensitive))
	{
		//QSysInfo::ByteOrder == QSysInfo::BigEndian

		vtkSmartPointer<vtkXMLPolyDataReader> xr = vtkSmartPointer<vtkXMLPolyDataReader>::New();
		xr->SetFileName(fileName.toStdString().c_str());
		xr->Update();
		pd = xr->GetOutput();
	}
	else if (fileName.endsWith(".vtk", Qt::CaseInsensitive))
	{
		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(fileName.toStdString().c_str());
		reader->Update();
		pd = reader->GetOutput();
	}

	QFileInfo finfo(fileName);
	QString layerName = finfo.baseName();

	vtkRenderWindow* renWin = this->ui->qvtkWidget->GetRenderWindow();
	NMDebugAI( << "creating the vector layer and assigning data set..." << endl);
	NMVectorLayer* layer = new NMVectorLayer(renWin);
	layer->setFileName(fileName);
	layer->setObjectName(layerName);
	layer->setDataSet(pd);
	layer->setVisible(true);
    this->mLayerList->addLayer(layer);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void OtbModellerWin::saveSelectionAsVtkPolyData()
{
    NMDebugCtx(ctxOtbModellerWin, << "...");

//    // get the selected layer
//    NMLayer* l = this->mLayerList->getSelectedLayer();
//    if (l == 0)
//        return;

//    // make sure, we've got a vector layer
//    if (l->getLayerType() != NMLayer::NM_VECTOR_LAYER)
//        return;

//    QString layerName = l->objectName();

//    // take the first layer and save as vtkpolydata
//    QFileDialog dlg(this);
//    dlg.setAcceptMode(QFileDialog::AcceptSave);
//    dlg.setFileMode(QFileDialog::AnyFile);
//    dlg.setWindowTitle(tr("Save As VTK PolyData File"));
//    dlg.setDirectory("~/");
//    dlg.setNameFilter("VTK PolyData File (*.vtk)");

//    QString selectedFilter;
//    QString fileName;
//    if (dlg.exec())
//    {
//        fileName = dlg.selectedFiles().at(0);
//        if (fileName.isNull() || fileName.isEmpty())
//            return;
//        selectedFilter = dlg.selectedNameFilter();
//    }
//    else
//        return;

//    const QItemSelection sel = l->getSelection();




//    NMDebugAI(<< "writing ASCII *.vtk file ..." << endl);
//    vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
//    writer->SetFileName(fileName.toStdString().c_str());
//    writer->SetInputData(const_cast<vtkDataSet*>(l->getDataSet()));
//    writer->SetFileTypeToASCII();
//    writer->Update();

    NMDebugCtx(ctxOtbModellerWin, << "done!");

}

void OtbModellerWin::saveAsVtkPolyData()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	// get the selected layer
    NMLayer* l = this->mLayerList->getSelectedLayer();
	if (l == 0)
		return;

	// make sure, we've got a vector layer
	if (l->getLayerType() != NMLayer::NM_VECTOR_LAYER)
		return;

	QString layerName = l->objectName();

	// take the first layer and save as vtkpolydata
	QFileDialog dlg(this);
	dlg.setAcceptMode(QFileDialog::AcceptSave);
	dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setWindowTitle(tr("Save As VTK PolyData File"));
	dlg.setDirectory("~/");
    //dlg.setNameFilter("XML PolyData (*.vtp);;Binary PolyData (*.vtk)");
    dlg.setNameFilter("VTK PolyData File (*.vtk)");

	QString selectedFilter;
	QString fileName;
//	QString fileName = QFileDialog::getSaveFileName(this,
//			tr("Save As VTK PolyData File (binary/ASCII)"), tr("~/") + layerName + tr(".vtk"),
//			tr("Binary PolyData (*.vtk);;ASCII PolyData (*.vtk)"), &selectedFilter);
	if (dlg.exec())
	{
		fileName = dlg.selectedFiles().at(0);
		if (fileName.isNull() || fileName.isEmpty())
			return;
		selectedFilter = dlg.selectedNameFilter();
	}
	else
		return;

//	if (selectedFilter == "XML PolyData (*.vtp)")
//	{
//		NMDebugAI(<< "writing XML-file " << fileName.toStdString() << " ..." << endl);
//		vtkSmartPointer<vtkXMLPolyDataWriter> xw = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
//		//if (xw.GetPointer() == 0)
//		//{
//		//	NMDebugAI(<< "XML PolyDataWriter is NULL!");
//		//	return;
//		//}
//        vtkDataSet* ds = const_cast<vtkDataSet*>(l->getDataSet());
//        vtkPolyData* pd = vtkPolyData::SafeDownCast(ds);
//        xw->SetInputData(pd);//const_cast<vtkDataSet*>();
//		xw->SetFileName(fileName.toStdString().c_str());
//		xw->Write();
//		//xw->Update();
//	}
//	else

    // until we've resolved the issue with the vtkXMLPolyDataWriter we
    // use the *.vtk ASCII format for its portability across platforms
	{
        NMDebugAI(<< "writing ASCII *.vtk file ..." << endl);
		vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
		writer->SetFileName(fileName.toStdString().c_str());
        writer->SetInputData(const_cast<vtkDataSet*>(l->getDataSet()));
        writer->SetFileTypeToASCII();
		writer->Update();
	}

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void OtbModellerWin::import3DPointSet()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Open PointDataSet (x,y,z)"), "~", tr("All Text Files (*.*)"));

	QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    	return;

	// some vars we need
	vtkSmartPointer<vtkPolyData> pointCloud = vtkPolyData::New();
	vtkSmartPointer<vtkPoints> pts = vtkPoints::New();
	vtkSmartPointer<vtkCellArray> verts = vtkCellArray::New();

    QTextStream in(&file);

    // determine splitter (either ' ', ',', ';', '\t')
    QStringList splitChars;
    splitChars << " " << "," << ";" << "\t";
    QString splitter;
	QString line;
	QStringList entr;

	line = in.readLine();
    bool found = false;
	QStringList::iterator spcit = splitChars.begin();
	for (; spcit != splitChars.end(); spcit++)
	{
		entr = line.split(*spcit);
		if (entr.count() == 3)
		{
			splitter = *spcit;
			found = true;
			break;
		}
	}

	if (!found)
	{
		qDebug() << "Can't read content from: '" << fileName << "'! Check file format!";
		return;
	}


    double x, y, z;
    double lowPt[3] = {0.0, 0.0, 10000};
    double highPt[3] = {0.0, 0.0, -100000};
    bool bx, by, bz;
    int num=0;
    vtkIdType id;
    in.reset();
    while (!in.atEnd())
    {
    	// split the line into values
    	line = in.readLine();
    	entr = line.split(splitter);

        // skip incomplete data sets
        if (entr.size() < 3)
         continue;

    	x = entr.at(0).toDouble(&bx);
        y = entr.at(1).toDouble(&by);
        z = entr.at(2).toDouble(&bz);

        if (!bx || !by || !bz)
         continue;

        if (z < lowPt[2])
        {
        	lowPt[0] = x;
        	lowPt[1] = y;
        	lowPt[2] = z;
        }

        if (z > highPt[2])
        {
        	highPt[0] = x;
        	highPt[1] = y;
        	highPt[2] = z;
        }

        id = pts->InsertNextPoint(x, y, z);
        verts->InsertNextCell(1, &id);

        num++;
    }
    file.close();

	// setting up the PolyData
    pointCloud->SetPoints(pts);
    pointCloud->SetVerts(verts);

//    this->displayPolyData(pointCloud, lowPt, highPt);

    QFileInfo fi(file);

	vtkRenderWindow* renWin = this->ui->qvtkWidget->GetRenderWindow();
	NMVectorLayer* layer = new NMVectorLayer(renWin);
	layer->setObjectName(fi.baseName());
	layer->setDataSet(pointCloud);
	layer->setVisible(true);
    this->mLayerList->addLayer(layer);

}

/*
void OtbModellerWin::displayPolyData(vtkSmartPointer<vtkPolyData> polydata, double* lowPt, double* highPt)
{
	NMDebugCtx(ctx, << "...");

	// give some feedback about what you're going to display
	vtkIndent ind(0);
    polydata->PrintSelf(std::cout, ind);

    // get the boundary of the polydata
    double vp[6];
    polydata->GetBounds(vp);

    vtkSmartPointer<vtkOGRLayerMapper> mapper = vtkSmartPointer<vtkOGRLayerMapper>::New();
	mapper->SetInput(polydata);

	vtkSmartPointer<vtkLookupTable> clrtab = vtkSmartPointer<vtkLookupTable>::New();
	clrtab->SetNumberOfTableValues(polydata->GetPolys()->GetNumberOfCells());
	clrtab->SetHueRange(0, 1);
	clrtab->SetSaturationRange(0.5, 1);
	clrtab->SetValueRange(0.5, 1);
	clrtab->Build();

	mapper->SetScalarRange(0, clrtab->GetNumberOfTableValues()-1);
	mapper->SetLookupTable(clrtab);

	vtkSmartPointer<vtkActor> polyActor = vtkSmartPointer<vtkActor>::New();
	polyActor->SetMapper(mapper);
	polyActor->SetPickable(1);

	// create a polydata layer for the outline of polys
	vtkSmartPointer<vtkPolyData> outline = vtkSmartPointer<vtkPolyData>::New();
	outline->SetPoints(polydata->GetPoints());
	outline->SetLines(polydata->GetPolys());

	vtkSmartPointer<vtkIntArray> outlineclr = vtkSmartPointer<vtkIntArray>::New();
	outlineclr->SetNumberOfValues(outline->GetNumberOfCells());
	for (int i=0; i < outline->GetNumberOfCells(); i++)
		outlineclr->SetValue(i, 0);
	outline->GetCellData()->SetScalars(outlineclr);

	vtkSmartPointer<vtkLookupTable> olclrtab = vtkSmartPointer<vtkLookupTable>::New();
	olclrtab->SetNumberOfTableValues(1);
	olclrtab->SetTableValue(0, 0, 0, 0, 1);


	vtkSmartPointer<vtkOGRLayerMapper> outlinemapper = vtkSmartPointer<vtkOGRLayerMapper>::New();
	outlinemapper->SetInput(outline);
	outlinemapper->SetLookupTable(olclrtab);

	vtkSmartPointer<vtkActor> outlineactor = vtkSmartPointer<vtkActor>::New();
	outlineactor->SetMapper(outlinemapper);
	outlineactor->SetPickable(1);

	this->m_renderer->AddActor(polyActor);
	this->m_renderer->AddActor(outlineactor);

    this->m_renderer->ResetCamera(vp);
    this->m_renderer->Render();
    this->ui->qvtkWidget->update();

    NMDebugAI( << "bounds: " << vp[0] << ", " << vp[1] << ", " << vp[2] << ", " << vp[3] << ", " <<
    		vp[4] << ", " << vp[5] << endl);

    NMDebugCtx(ctx, << "done!");

// ----------------------------------------------------------
// * 	some snipptes that might be useful later
// * ----------------------------------------------------------


//    vtkSmartPointer<vtkDataSetMapper> surfmapper = vtkDataSetMapper::New();

//	vtkSmartPointer<vtkElevationFilter> filter = vtkElevationFilter::New();
//    // apply the elevation filter
//    if (lowPt != 0 && highPt != 0)
//    {
//    	filter->SetInput(polydata);
//    	filter->SetLowPoint(lowPt);
//    	filter->SetHighPoint(highPt);
//    	mapper->SetInput(filter->GetPolyDataOutput());
//    }
//    else
//    	mapper->SetInput(polydata);

//    vtkSmartPointer<vtkDelaunay2D> delaunay = vtkDelaunay2D::New();
//    delaunay->SetInput(polydata);

//    surfmapper->SetInput(delaunay->GetOutput());


//	display also the points
//    vtkSmartPointer<vtkPolyDataMapper> ptsmapper = vtkPolyDataMapper::New();
//    ptsmapper->SetInput(polydata);
//
//    vtkSmartPointer<vtkActor> ptsactor = vtkActor::New();
//    ptsactor->SetMapper(ptsmapper);
//
//    vtkSmartPointer<vtkProperty> prop = vtkProperty::New();
//    prop->SetColor(1,0,0);
//    prop->SetPointSize(2);
//    ptsactor->SetProperty(prop);
//    this->m_renderer->AddViewProp(ptsactor);
}
*/


vtkSmartPointer<vtkPolyData> OtbModellerWin::wkbPointToPolyData(OGRLayer& l)
{
	 /* OGC SimpleFeature Spec
	  *
	  * Point {double x; double y}
	  *
      *	WKBPoint {char byteOrder; static unsigned int wkbType = 1;
	  *		Point				point
	  * }
	  *
	  * WKBMultiPoint {char byteOrder; static unsigned int wkbType = 4;
	  * 	unsigned int 		numPoints;
	  * 	Point				points[numPoints]
	  * }
	  */

	return 0;
}


vtkSmartPointer<vtkPolyData> OtbModellerWin::wkbLineStringToPolyData(OGRLayer& l)
{
	/*  OGC SimpleFeature Spec
	 *
	 * 	Point {double x; double y}
	 *
	 *  WKBLineString {char byteOrder; static unsigned int wkbType = 2;
	 *  	unsigned int		numPoints;
	 *  	Point 				points[numPoints]
	 *  }
	 *
	 *  WKBMultiLineString {char byteOrder; static unsigned int wkbType = 5;
	 *  	unsigned int 		numLineStrings;
	 *  	WKBLineString		lineStrings[numLineStrings]
	 *  }
	 *
	 */

	return 0;
}


vtkSmartPointer<vtkPolyData> OtbModellerWin::wkbPolygonToPolyData(OGRLayer& l)
{
	/*	OGC SimpleFeature Spec
	 *
	 * 	Point {double x; double y}
	 * 	LinearRing {unsigned int numPoints; Point points[numPoints]}
	 *
	 *  WKBPolygon {char byteOrder; static unsigned int wkbType = 3;
	 *  	unsigned int		numRings;
	 *		LinearRing			rings[numRings]
	 *	}
	 *
	 *	WKBMultiPolygon {char byteOrder; static unsigned int wkbType = 6;
	 *		unsigned int		numPolygons;
	 *		WKBPolygon			polygons[numPolygons]
	 *	}
	 *
	 *	To allow for proper polygon display (i.e. holes / donuts, non-convex polygons),
	 *	the herewith created vtkPolyData should be used in conjunction with the
	 *	vtkOGRLayerMapper class. vtkOGRLayerMapper uses GLU tesselation to display
	 *	the above mentioned features. For this to work, the following vertex order
	 *	is required to identify features
	 *		- exterior ring: counter-clockwise
	 *		- interior rings: clockwise
	 *
	 *	Unfortunately, the OGR exportToWKB function uses the opposite ordering of rings. So here
	 *	all exterior rings are traversed from the last point to the first one to adhere
	 *	with vtk requirements
	 */

	NMDebugCtx(ctxOtbModellerWin, << "...");

	// create vtk data objects
	vtkSmartPointer<vtkPolyData> vtkVect = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkMergePoints> mpts = vtkSmartPointer<vtkMergePoints>::New();

	// set divisions --> no of division in x-y-z-direction (i.e. range)
	// set bounds
	OGREnvelope ogrext;
	l.GetExtent(&ogrext);
	double fbnds[6] = {ogrext.MinX-10, ogrext.MaxX+10,
					   ogrext.MinY-10, ogrext.MaxY+10, 0, 0};
	mpts->SetDivisions(10, 10, 1);
	mpts->SetTolerance(1e-2);
	mpts->InitPointInsertion(points, fbnds);

	vtkIdType pointId, cellId;
	double dMax = std::numeric_limits<double>::max();
	double dMin = std::numeric_limits<double>::max() * -1;
	double bnd[6] = {dMax, dMin, dMax, dMin, 0, 0};
	int featcount = l.GetFeatureCount(1);
	NMDebugAI(<< "number of features: " << featcount << endl);

	vtkVect->Allocate(featcount, 100);

	// OGC SimpleFeature WellKnownBinary (WKB) byte order enum:
	// wkbXDR = 0 (big endian) | wkbNDR = 1 (little endian)
	OGRwkbByteOrder bo = QSysInfo::ByteOrder == QSysInfo::BigEndian ? wkbXDR : wkbNDR;
	OGRFeature *pFeat;

	// ----------------------------------------------------------------------------------
	// create the attributes arrays for this layer
	l.ResetReading();
	pFeat = l.GetNextFeature();
	int nfields = pFeat->GetFieldCount();

	std::vector<vtkSmartPointer<vtkAbstractArray> > attr;
    // keeps track of the features to copy
    // we filter all "nm_*" by default
    std::vector<int> fieldIdx;

	vtkSmartPointer<vtkLongArray> nm_id = vtkSmartPointer<vtkLongArray>::New();
	nm_id->SetName("nm_id");
	nm_id->Allocate(l.GetFeatureCount(1), 100);

	vtkSmartPointer<vtkUnsignedCharArray> nm_hole = vtkSmartPointer<vtkUnsignedCharArray>::New();
	nm_hole->SetName("nm_hole");
	nm_hole->Allocate(l.GetFeatureCount(1), 100);

	vtkSmartPointer<vtkUnsignedCharArray> nm_sel = vtkSmartPointer<vtkUnsignedCharArray>::New();
	nm_sel->SetName("nm_sel");
	nm_sel->Allocate(l.GetFeatureCount(1), 100);


	vtkSmartPointer<vtkIntArray> iarr;
	vtkSmartPointer<vtkDoubleArray> darr;
	vtkSmartPointer<vtkStringArray> sarr;

	for (int f=0; f < nfields; ++f)
	{
		OGRFieldDefn* fdef = pFeat->GetFieldDefnRef(f);
        //		NMDebugAI( << fdef->GetNameRef() << ": " << fdef->GetFieldTypeName(fdef->GetType()) << endl);
        if (::strcmp(fdef->GetNameRef(), "nm_id") == 0  ||
            ::strcmp(fdef->GetNameRef(), "nm_hole") == 0 ||
            ::strcmp(fdef->GetNameRef(), "nm_sel") == 0)
        {
            continue;
        }
        fieldIdx.push_back(f);

		switch(fdef->GetType())
		{
			case OFTInteger:
				iarr = vtkSmartPointer<vtkIntArray>::New();
				iarr->SetName(fdef->GetNameRef());
				iarr->Allocate(featcount, 100);
				attr.push_back(iarr);
				break;
			case OFTReal:
				darr = vtkSmartPointer<vtkDoubleArray>::New();
				darr->SetName(fdef->GetNameRef());
				darr->Allocate(featcount, 100);
				attr.push_back(darr);
				break;
			default: // case OFTString:
				sarr = vtkSmartPointer<vtkStringArray>::New();
				sarr->SetName(fdef->GetNameRef());
				sarr->Allocate(featcount, 100);
				attr.push_back(sarr);
				break;
		}
	}

	// ------------------------------------------------------------------------------------------------
	// process feature by feature
	l.ResetReading();
	//NMDebugAI(<< "importing features ... " << std::endl);
	unsigned int featCounter = 1;
	while ((pFeat = l.GetNextFeature()) != NULL)
	{
		OGRGeometry *geom = pFeat->GetGeometryRef();
		if (geom == 0)
		{
			NMWarn(ctxOtbModellerWin, << "Oops - got NULL geometry for this feature! - Abort.");
			continue;
		}
		int fid = pFeat->GetFieldAsInteger(0);

		int wkbSize = geom->WkbSize();
//		NMDebug(<< endl << endl << "feature #" << featCounter << ":" << fid << " (" << geom->getGeometryName() <<
//				", " << wkbSize << " bytes)" << endl);
		unsigned char* wkb = new unsigned char[wkbSize];
		if (wkb == 0)
		{
			NMErr(ctxOtbModellerWin, << "not enough memory to allocate feature wkb buffer!");
			return 0;
		}
		geom->exportToWkb(bo, wkb);

		// multi or not ?
		unsigned int pos = sizeof(char); // we start with the type, 'cause we know the byte order already
		unsigned int type;
		memcpy(&type, (wkb+pos), sizeof(unsigned int));

		// jump over type (= 4 byte)
		pos += sizeof(unsigned int);

		//NMDebugAI( << "feature #" << featCounter << " is of geometry type " << type << endl);

		unsigned int nPolys;
		unsigned int nRings;

		if (type == 6)
		{
			memcpy(&nPolys, (wkb+pos), sizeof(unsigned int));
			// jump over the nPolys (= 4 byte),
			// skip byte order for 1st polygon (= 1 byte),
			// skip geometry type for 1st polygon (= 4 byte);
			pos += sizeof(unsigned int) + sizeof(char) + sizeof(unsigned int);
		}
		else
			nPolys = 1;

//		NMDebug(<< endl << "number of polygons: " << nPolys << endl);

		unsigned int nPoints;
		double x, y;

		// pos should point to the number of rings of the 1st polygon
		for (unsigned int p=0; p < nPolys; ++p) // -----------------------------------------
		{
			// get the number of rings for this polygon
			memcpy(&nRings, (wkb+pos), sizeof(unsigned int));

//			NMDebug(<< "polygon #" << p+1 << " - " << nRings << " rings ... " << endl);

			// jump over nRings (= 4 byte)
			pos += sizeof(unsigned int);

			// process rings
			for (unsigned int r=0; r < nRings; ++r) // ......................................
			{
				// get the number of points for this ring
				memcpy(&nPoints, (wkb+pos), sizeof(unsigned int));

//				NMDebug(<< "ring #" << r+1 << " - " << nPoints << " points ..." << endl);

				// jump over nPoints (= 4 byte)
				pos += sizeof(unsigned int);

				// insert next cell and assign cellId as preliminary attribute
				cellId = polys->InsertNextCell(nPoints);

				// assign feature id and cell values
				nm_sel->InsertNextValue(0);
				if (r == 0)
				{
					nm_id->InsertNextValue(featCounter);
					nm_hole->InsertNextValue(0);
				}
				else
				{
					nm_id->InsertNextValue(-1);
					nm_hole->InsertNextValue(1);
				}

				vtkIntArray* iar;
				vtkDoubleArray* dar;
				vtkStringArray* sar;
                for (int fidx=0; fidx < fieldIdx.size(); ++fidx)
				{
                    switch(pFeat->GetFieldDefnRef(fieldIdx[fidx])->GetType())
					{
					case OFTInteger:
                        iar = vtkIntArray::SafeDownCast(attr[fidx]);
                        iar->InsertNextValue(pFeat->GetFieldAsInteger(fieldIdx[fidx]));
						break;
					case OFTReal:
                        dar = vtkDoubleArray::SafeDownCast(attr[fidx]);
                        dar->InsertNextValue(pFeat->GetFieldAsDouble(fieldIdx[fidx]));
						break;
					default:
                        sar = vtkStringArray::SafeDownCast(attr[fidx]);
                        sar->InsertNextValue(pFeat->GetFieldAsString(fieldIdx[fidx]));
						break;
					}
				}

				/* We traverse all points backwards, since the vertex order given by
				 * exportToWKB is opposite to the VTK required one (and the
				 * OGC simple feature spec).
				 *
				  0   1   2   3   4 ... nPoints-1
				 >x y x y x y x y x y   x y
				 --> advancing the buffer pointer (pos) nPoints-1 times by
					 16 bytes moves it right in front of the x coordinate
					 of the last point, adding another 8 bytes, in front
					 of the y coordinate of the last point; then we are
					 going backwards through the buffer and read
					 the coordinates;
				*/

				unsigned int rpos = pos + ((nPoints-1) * 2 * sizeof(double)) + sizeof(double);
				pos = rpos + sizeof(double);

				for (unsigned int pnt=0; pnt < nPoints; ++pnt) // ............................
				{
					// get the coordinates and jump over it
					// i.e. forward 8 byte each time (double!)
					memcpy(&y, (wkb+rpos), sizeof(double)); rpos -= sizeof(double);
					memcpy(&x, (wkb+rpos), sizeof(double)); rpos -= sizeof(double);

					double pt[3];
					vtkIdType tmpId;

//					NMDebug(<< pnt << "(" << x << "," << y << ")" << endl);
					pt[0] = x;
					pt[1] = y;
					pt[2] = 0.0;

					mpts->InsertUniquePoint(pt, tmpId);
					polys->InsertCellPoint(tmpId);
				}
//				NMDebug(<< endl);
			}
			//skip byte order for next polygon (= 1 byte)
			//skip geom type for next polygon (= 4 byte)
			pos += sizeof(char) + sizeof(unsigned int);
		}

		// increase feature counter
		//NMDebug(<< featCounter << " ");
		++featCounter;

		// release memory for the wkb buffer
		delete[] wkb;
	}
	//NMDebug(<< std::endl);

	// add geometry (i.e. points and cells)
	vtkVect->SetPoints(mpts->GetPoints());
	vtkVect->SetPolys(polys);

	// add attributes
	vtkVect->GetCellData()->SetScalars(nm_id);
	vtkVect->GetCellData()->AddArray(nm_hole);
	vtkVect->GetCellData()->AddArray(nm_sel);
    for (int f=0; f < fieldIdx.size(); ++f)
		vtkVect->GetCellData()->AddArray(attr[f]);

	vtkVect->BuildCells();
	vtkVect->BuildLinks();

	NMDebugAI(<< featCounter << " features imported" << endl);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
	return vtkVect;
}


void
OtbModellerWin::vtkPolygonPolydataToOGR(OGRDataSource *ds, NMVectorLayer* vectorLayer)
{
    NMDebugCtx(ctxOtbModellerWin, << "...");

    // create the output layer
    NMDebugAI(<< "Create output layer ..." << std::endl);
    OGRLayer* ogrLayer = ds->CreateLayer(vectorLayer->objectName().toStdString().c_str(),
                                         0, wkbPolygon, 0);
    if (ogrLayer == 0)
    {
        NMDebugAI(<< "Failed creating the OGR polygon layer!" << std::endl);
        NMDebugCtx(ctxOtbModellerWin, << "done!");
        return;
    }

    // make a list of available attributes
    vtkDataSet* vtkDS = const_cast<vtkDataSet*>(vectorLayer->getDataSet());
    vtkDataSetAttributes* dsAttr = vtkDS->GetAttributes(vtkDataSet::CELL);

    // check, whether we've got some info about donut polygons ...
    vtkDataArray* hole = dsAttr->GetArray("nm_hole");
    if (hole == 0)
    {
        NMDebugAI(<< "Failed creating the OGR polygon layer!" << std::endl);
        NMErr(ctxOtbModellerWin, << "Lacking info on donut polygons - bailing out!");
        NMDebugCtx(ctxOtbModellerWin, << "done!");
        return;
    }

    //=================================================================================
    // copying attribute table structure
    //=================================================================================

    QStringList origFieldNames;
    QStringList writeFieldNames;

    NMDebugAI(<< "Add attributes to output layer ..." << std::endl);
    int numFields = dsAttr->GetNumberOfArrays();
    for (int f=0; f < numFields; ++f)
    {
        vtkAbstractArray* aa = dsAttr->GetAbstractArray(f);
        QString fieldName = aa->GetName();
        origFieldNames << fieldName;
        if (fieldName.length() > 10)
        {
            bool bok;
            QString newName = QInputDialog::getText(this, "Confirm Field Name",
                                                    "Please enter field name with max. 10 characters.",
                                                    QLineEdit::Normal, fieldName, &bok);
            if (!bok)
            {
                NMDebugAI(<< "No valid new field name entered! Field name gets mangled!" << std::endl);
                writeFieldNames << fieldName.left(10);
            }
            else
            {
                if (newName.length() <= 10)
                {
                    writeFieldNames << newName;
                }
                else
                {
                    NMDebugAI(<< "No valid new field name entered! Field name gets mangled!" << std::endl);
                    writeFieldNames << fieldName.left(10);
                }
            }
        }
        else
        {
            writeFieldNames << fieldName;
        }


        if (aa->IsNumeric())
        {
            if (    aa->GetDataType() == VTK_FLOAT
                 || aa->GetDataType() == VTK_DOUBLE
               )
            {
                OGRFieldDefn field(writeFieldNames.at(f).toStdString().c_str(), OFTReal);
                if (ogrLayer->CreateField(&field) != OGRERR_NONE)
                {
                    NMDebugAI(<< "Failed crating double field '" << aa->GetName() << "'!"
                              << std::endl);
                }

            }
            else
            {
                OGRFieldDefn field(writeFieldNames.at(f).toStdString().c_str(), OFTInteger);
                if (ogrLayer->CreateField(&field) != OGRERR_NONE)
                {
                    NMDebugAI(<< "Failed crating integer field '" << aa->GetName() << "'!"
                              << std::endl);
                }
            }
        }
        else
        {
            OGRFieldDefn field(writeFieldNames.at(f).toStdString().c_str(), OFTString);
            if (ogrLayer->CreateField(&field) != OGRERR_NONE)
            {
                NMDebugAI(<< "Failed crating string field '" << aa->GetName() << "'!"
                          << std::endl);
                continue;
            }
        }
    }

    //=================================================================================
    // copying the geometries and fill the attribute table
    //=================================================================================

    NMDebugAI(<< "Copy data feature by feature ... " << std::endl);
    // now we're looping over the polygons, define OGRFeatures, and
    // write them out
    vtkPolyData* pd = vtkPolyData::SafeDownCast(vtkDS);
    vtkIdType ncells = pd->GetNumberOfCells();
    vtkPoints* pts = pd->GetPoints();
    vtkCellArray* ca = pd->GetPolys();
    ca->InitTraversal();

    vtkIdType npts;
    vtkIdType* cpts;
    double* xyz;
    int holecell = -1;
    for (int cell=0; cell < ncells; ++cell)
    {
        OGRFeature* feat = OGRFeature::CreateFeature(ogrLayer->GetLayerDefn());

        // ===============================================================
        // create the geometry
        if (ca->GetNextCell(npts, cpts) == 0)
        {
            break;
        }

        OGRPolygon poly;
        OGRLinearRing outerRing;
        //for (int n=0; n < npts; ++n)
        for (int n=npts-1; n >= 0; --n)
        {
            xyz = pts->GetPoint(cpts[n]);
            outerRing.addPoint(xyz[0], xyz[1], xyz[2]);
        }

        poly.addRing(&outerRing);

        holecell = cell;
        while (holecell < ncells-1)
        {
            if (hole->GetTuple1(holecell+1))
            {
                if (ca->GetNextCell(npts, cpts) == 0)
                {
                    break;
                }
                OGRLinearRing innerRing;
                //for (int n=0; n < npts; ++n)
                for (int n=npts-1; n >= 0; --n)
                {
                    xyz = pts->GetPoint(cpts[n]);
                    innerRing.addPoint(xyz[0], xyz[1], xyz[2]);
                }
                poly.addRing(&innerRing);

                ++holecell;
            }
            else
            {
                break;
            }
        }

        if (feat->SetGeometry(&poly) != OGRERR_NONE)
        {
            NMDebugAI(<< "Failed adding polygon #" << cell << "!" << std::endl);
            break;
        }

        // ================================================================
        // add the attributes to the layer
        for (int f=0; f < numFields; ++f)
        {
            vtkAbstractArray* aa = dsAttr->GetAbstractArray(f);
            QString fieldName = writeFieldNames.at(f);
            if (aa->IsNumeric())
            {
                vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
                if (    aa->GetDataType() == VTK_FLOAT
                     || aa->GetDataType() == VTK_DOUBLE
                   )
                {
                    feat->SetField(fieldName.toStdString().c_str(), static_cast<double>(da->GetTuple1(cell)));
                }
                else
                {
                    feat->SetField(fieldName.toStdString().c_str(), static_cast<int>(da->GetTuple1(cell)));
                }
            }
            else
            {
                vtkStringArray* sa = vtkStringArray::SafeDownCast(aa);
                feat->SetField(fieldName.toStdString().c_str(), sa->GetValue(cell).c_str());
            }
        }
        // re-align cell with holecell
        cell = holecell;

        // ================================================================
        // finally, we add the local feature to the data set feature
        if (ogrLayer->CreateFeature(feat) != OGRERR_NONE)
        {
            NMDebugAI(<< "Failed crating feature #" << cell << "!" << std::endl);
        }

        OGRFeature::DestroyFeature(feat);

        if (cell % 500 == 0)
        {
            NMDebug( << ".");
        }
    }
    NMDebug(<< std::endl);

    NMDebugCtx(ctxOtbModellerWin, << "done!");
}

vtkSmartPointer<vtkPolyData> OtbModellerWin::OgrToVtkPolyData(OGRDataSource& ds)
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	// working just on the first layer right now
	OGRLayer *pLayer = ds.GetLayer(0);
		NMDebugAI( << "the layer contains " << pLayer->GetFeatureCount(1)
			<< " features" << endl);
		NMDebugAI( << "geometry type: "
			<< pLayer->GetLayerDefn()->GetGeomType() << endl);

	// get an idea of the type of geometry we are dealing with
	// and call the appropriate internal conversion function
	OGRwkbGeometryType geomType = pLayer->GetLayerDefn()->GetGeomType();

	vtkSmartPointer<vtkPolyData> vtkVect;
	switch (geomType)
	{
	case wkbPoint:
	case wkbMultiPoint:
		vtkVect = this->wkbPointToPolyData(*pLayer);
		break;
	case wkbLineString:
	case wkbMultiLineString:
		vtkVect = this->wkbLineStringToPolyData(*pLayer);
		break;
	case wkbPolygon:
	case wkbMultiPolygon:
		vtkVect = this->wkbPolygonToPolyData(*pLayer);
		break;
	default:
		NMErr(ctxOtbModellerWin, << "Geometry type '" << geomType << "' is currently not supported!");
		vtkVect = NULL;
	}

	NMDebugCtx(ctxOtbModellerWin, << "done!");

	return vtkVect;
}

void OtbModellerWin::loadVectorLayer()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Import OGR Vector File"), "~", tr("OGR supported files (*.*)"));
	if (fileName.isNull()) return;

	NMDebugAI( << "opening '" << fileName.toStdString() << "' ..." << endl);

	OGRRegisterAll();
	OGRDataSource *pDS = OGRSFDriverRegistrar::Open(fileName.toStdString().c_str(),
			FALSE, NULL);
	if (pDS == NULL)
	{
		NMErr(ctxOtbModellerWin, << "failed to open '" << fileName.toStdString() << "'!");
		return;
	}

	vtkSmartPointer<vtkPolyData> vtkVec = this->OgrToVtkPolyData(*pDS);
	OGRDataSource::DestroyDataSource(pDS);

	QFileInfo finfo(fileName);
	QString layerName = finfo.baseName();

	vtkRenderWindow* renWin = this->ui->qvtkWidget->GetRenderWindow();
	NMVectorLayer* layer = new NMVectorLayer(renWin);
	layer->setObjectName(layerName);
	layer->setDataSet(vtkVec);
	layer->setVisible(true);
    this->mLayerList->addLayer(layer);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void
OtbModellerWin::connectImageLayerProcSignals(NMLayer* layer)
{
	connect(layer, SIGNAL(layerProcessingStart()), this, SLOT(showBusyStart()));
	connect(layer, SIGNAL(layerProcessingEnd()), this, SLOT(showBusyEnd()));
	connect(layer, SIGNAL(layerLoaded()), this, SLOT(addLayerToCompList()));
}

#ifdef BUILD_RASSUPPORT
void
OtbModellerWin::loadRasdamanLayer()
{
	// if we haven't got a connection, we shouldn't
	// try browsing the metadata
	if (this->getRasdamanConnector() == 0)
		return;

	this->updateRasMetaView();
    if (this->mpPetaView != 0)
		this->mpPetaView->show();
	else
	{
		NMBoxInfo("Failed initialising rasdaman metadata browser!",
				 "Check whether Postgres is up and running and access is "
				"is configured properly!");
		return;
	}
}

void
OtbModellerWin::fetchRasLayer(const QString& imagespec,
		const QString& covname)
{
//	try
//	{
		NMDebugAI( << "opening " << imagespec.toStdString() << " ..." << std::endl);

		vtkRenderWindow* renWin = this->ui->qvtkWidget->GetRenderWindow();
		NMImageLayer* layer = new NMImageLayer(renWin);

		RasdamanConnector* rasconn = this->getRasdamanConnector();
		if (rasconn == 0)
		{
			NMErr(ctxOtbModellerWin, << "Connection with rasdaman failed!");
			return;
		}

		layer->setRasdamanConnector(rasconn);
		layer->setObjectName(covname);
		this->connectImageLayerProcSignals(layer);

		QtConcurrent::run(layer, &NMImageLayer::setFileName, imagespec);

		//if (layer->setFileName(imagespec))
		//{
		//	layer->setVisible(true);
        //	this->mLayerList->addLayer(layer);
		//}
		//else
		//	delete layer;
//	}
//	catch(r_Error& re)
//	{
//		//this->mpRasconn->disconnect();
//		//this->mpRasconn->connect();
//		NMErr(ctxOtbModellerWin, << re.what());
//		NMDebugCtx(ctxOtbModellerWin, << "done!");
//	}
}

void
OtbModellerWin::eraseRasLayer(const QString& imagespec)
{
	QString msg = QString(tr("You're about to delete image '%1'!\nDo you really want to do this?")).
			arg(imagespec);

	int ret = QMessageBox::warning(this, tr("Delete Rasdaman Image"),
			msg, QMessageBox::Yes | QMessageBox::No);

	if (ret == QMessageBox::No)
	{
		return;
	}

	NMDebugAI(<< "Deleting image ..." << imagespec.toStdString() << std::endl);

	int pos = imagespec.indexOf(':');
	QString coll = imagespec.left(pos);

	bool bok;
	double oid = imagespec.right(imagespec.size()-1-pos).toDouble(&bok);
	if (!bok)
	{
		NMErr(ctxOtbModellerWin, << "Couldn't extract OID from rasdaman image spec!");
		return;
	}

	RasdamanConnector* rasconn = this->getRasdamanConnector();
	if (rasconn == 0)
	{
		return;
	}

	RasdamanHelper2 helper(rasconn);
	helper.deletePSMetadata(coll.toStdString(), oid);
	helper.dropImage(coll.toStdString(), oid);

	std::vector<double> oids = helper.getImageOIDs(coll.toStdString());
	if (oids.size() == 0)
		helper.dropCollection(coll.toStdString());

    if (this->mpPetaView != 0)
		this->updateRasMetaView();
}

void
OtbModellerWin::updateRasMetaView()
{
	// query the flat metadata table showing all coverage/image metadata
	RasdamanConnector* rasconn = this->getRasdamanConnector();
	if (rasconn == 0)
	{
		return;
	}
	const PGconn* conn = rasconn->getPetaConnection();

	std::stringstream query;
	query << "select create_metatable(); "
		  << "select * from tmp_flatmetadata as t1 "
		  << "right join geometadata as t2 "
		  << "on t1.oid = t2.oid;";

	// copy the table into a vtkTable to be fed into a TableView
	PGresult* res = const_cast<PGresult*>(
			PQexec(const_cast<PGconn*>(conn), query.str().c_str()));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		NMBoxErr("Rasdaman Image Metadata",
				PQresultErrorMessage(res));
		return;
	}

	int nrows = PQntuples(res);
	int ncols = PQnfields(res);
	if (nrows < 1)
	{
		NMBoxInfo("Rasdaman Image Metadata",
				"No registered rasdaman images found in database!");
		NMDebugCtx(ctxOtbModellerWin, << "done!");
		return;
	}

	// because of the left join above, we've got two oid fields,
	// but we only want one!
	std::vector<std::string> colnames;
	std::vector<int> colindices;
	int oidcnt = 0;
	for (int c=0; c < ncols; ++c)
	{
		if (   QString(PQfname(res,c)).compare("oid", Qt::CaseInsensitive) == 0
		    && oidcnt == 0)
		{
			colnames.push_back(PQfname(res,c));
			colindices.push_back(c);
			++oidcnt;
		}
		else
		{
			colnames.push_back(PQfname(res,c));
			colindices.push_back(c);
		}
	}

	// copy the table structure
	// all columns, except oid, text columns, so we mostly just use vtk string arrays
	vtkSmartPointer<vtkTable> metatab = vtkSmartPointer<vtkTable>::New();
	QMap<QString, int> idxmap;
	for (int i=0; i < colindices.size(); ++i)
	{
		QString colname(PQfname(res, colindices[i]));
		//NMDebug(<< colname.toStdString() << " ");
		if (colname.compare("oid", Qt::CaseInsensitive) == 0)
		{
			vtkSmartPointer<vtkLongArray> lar = vtkSmartPointer<vtkLongArray>::New();
			lar->SetNumberOfComponents(1);
			lar->SetNumberOfTuples(nrows);
			lar->FillComponent(0,0);
			lar->SetName(PQfname(res, colindices[i]));
			metatab->AddColumn(lar);
			idxmap.insert(colname, colindices[i]);
		}
		else
		{
			vtkSmartPointer<vtkStringArray> sar = vtkSmartPointer<vtkStringArray>::New();
			sar->SetNumberOfComponents(1);
			sar->SetNumberOfTuples(nrows);
			sar->SetName(PQfname(res, colindices[i]));
			metatab->AddColumn(sar);
			idxmap.insert(colname, colindices[i]);
		}
	}
	//NMDebug(<< std::endl);

	//vtkSmartPointer<vtkUnsignedCharArray> car = vtkSmartPointer<vtkUnsignedCharArray>::New();
	//car->SetNumberOfComponents(1);
	//car->SetNumberOfTuples(nrows);
	//car->SetName("nm_sel");

	//vtkSmartPointer<vtkLongArray> idar = vtkSmartPointer<vtkLongArray>::New();
	//idar->SetNumberOfComponents(1);
	//idar->SetNumberOfTuples(nrows);
	//idar->FillComponent(0,0);
	//idar->SetName("nm_id");


	// copy the table body
	int ntabcols = metatab->GetNumberOfColumns();
	for (int r=0; r < nrows; ++r)
	{
		for (int i=0; i < ntabcols; ++i)
		{
			vtkAbstractArray* aar = metatab->GetColumn(i);
			QString colname(aar->GetName());
			if (colname.compare("oid", Qt::CaseInsensitive) == 0)
			{
				vtkLongArray* lar = vtkLongArray::SafeDownCast(aar);
				lar->SetValue(r, ::atol(PQgetvalue(res, r, idxmap.value(colname))));
			}
			else
			{
				vtkStringArray* sar = vtkStringArray::SafeDownCast(aar);
				sar->SetValue(r, PQgetvalue(res, r, idxmap.value(colname)));
			}
		}
		//car->SetTuple1(r, 0);
		//idar->SetTuple1(r, r);
	}
	//NMDebug( << std::endl);
	//metatab->AddColumn(car);
	//metatab->AddColumn(idar);

	PQclear(res);

	vtkQtTableModelAdapter* model;
	if (this->mpPetaView == 0)
	{
		model = new vtkQtTableModelAdapter(metatab, this);
	}
	else
	{
		model = qobject_cast<vtkQtTableModelAdapter*>(this->mPetaMetaModel);
		model->setTable(metatab);

		delete this->mpPetaView;
		this->mpPetaView = 0;
	}
	model->SetKeyColumn(0);
	this->mPetaMetaModel = model;

	this->mpPetaView = new NMTableView(this->mPetaMetaModel, 0);
	this->mpPetaView->setViewMode(NMTableView::NMTABVIEW_RASMETADATA);
	this->mpPetaView->setSelectionModel(
			new NMFastTrackSelectionModel(this->mPetaMetaModel, mpPetaView));


	connect(this->mpPetaView, SIGNAL(notifyLoadRasLayer(const QString&, const QString&)),
			this, SLOT(fetchRasLayer(const QString&, const QString&)));
	connect(this->mpPetaView, SIGNAL(notifyDeleteRasLayer(const QString&)),
			this, SLOT(eraseRasLayer(const QString&)));


	this->mpPetaView->setTitle("Rasdaman Image Metadata");
	//this->mpPetaView->setRowKeyColumn("nm_id");
	//this->mpPetaView->hideAttribute("nm_sel");
	//this->mpPetaView->hideAttribute("nm_id");

	return;
}
#endif

void OtbModellerWin::loadImageLayer()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Open Image"), "~", tr("All Image Files (*.*)"));
	if (fileName.isNull())
		return;

	NMDebugAI( << "opening " << fileName.toStdString() << " ..." << std::endl);

	QFileInfo finfo(fileName);
	QString layerName = finfo.baseName();

	vtkRenderWindow* renWin = this->ui->qvtkWidget->GetRenderWindow();
	NMImageLayer* layer = new NMImageLayer(renWin, 0, this);

//    connect(layer, SIGNAL(layerProcessingEnd(const QString &)),
//            loader, SLOT());

    this->connectImageLayerProcSignals(layer);
    //QtConcurrent::run(layer, &NMImageLayer::setFileName, fileName);
    layer->setFileName(fileName);

//	if (layer->setFileName(fileName))
//	{
//		layer->setVisible(true);
//		this->mLayerList->addLayer(layer);
//	}
//	else
//		delete layer;

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

void
OtbModellerWin::addLayerToCompList()
{
	NMLayer* layer = qobject_cast<NMLayer*>(this->sender());
	if (layer == 0)
		return;

	layer->setVisible(true);
    this->mLayerList->addLayer(layer);
}

void OtbModellerWin::toggle3DSimpleMode()
{
	if (m_b3D)
	{
		NMDebugAI( << "switching 3D off ..." << endl);

		// check for stero mode
		if (this->ui->actionToggle3DStereoMode->isChecked())
		{
			this->toggle3DStereoMode();
			this->ui->actionToggle3DStereoMode->setChecked(false);
		}

		// set image interaction
		this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
            vtkInteractorStyleImage::New());

		// reset the camera for the background renderer
		vtkRenderer* ren0 = this->mBkgRenderer;
		vtkCamera* cam0 = ren0->GetActiveCamera();

		double *fp = new double[3];
		double *p = new double[3];
		double dist;

		ren0->ResetCamera();
		cam0->GetFocalPoint(fp);
		cam0->GetPosition(p);
		dist = ::sqrt( ::pow((p[0]-fp[0]),2) + ::pow((p[1]-fp[1]),2) + ::pow((p[2]-fp[2]),2) );
		cam0->SetPosition(fp[0], fp[1], fp[2]+dist);
		cam0->SetViewUp(0.0, 1.0, 0.0);

		delete[] fp;
		delete[] p;

		// set new camera for all other renderers as well
		vtkRendererCollection* recoll = this->ui->qvtkWidget->GetRenderWindow()->GetRenderers();
		vtkRenderer *ren = recoll->GetNextItem();
		while (ren != NULL)
		{
			ren->SetActiveCamera(cam0);
			ren = recoll->GetNextItem();
		}

		this->m_orientwidget->SetEnabled(0);
		this->ui->actionToggle3DStereoMode->setEnabled(false);
		m_b3D = false;
	}
	else
	{
		NMDebugAI( << "switching 3D on!! ..." << endl);

		this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
			vtkInteractorStyleTrackballCamera::New());

		this->m_orientwidget->SetEnabled(1);
		this->ui->actionToggle3DStereoMode->setEnabled(true);
		m_b3D = true;
	}

    emit signalIsIn3DMode(m_b3D);
	this->ui->qvtkWidget->update();
}

void OtbModellerWin::toggle3DStereoMode()
{
        this->ui->qvtkWidget->GetRenderWindow()->SetStereoRender(
	    		!this->ui->qvtkWidget->GetRenderWindow()->GetStereoRender());
}


