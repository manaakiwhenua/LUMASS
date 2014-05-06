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



// VTK
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
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObject.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPoints.h"
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


//#include "valgrind/callgrind.h"


OtbModellerWin::OtbModellerWin(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::OtbModellerWin)
{
	// ++++++++++++++++++ META TYPES AND OTHER STUFF +++++++++++++++++++++++++++++

#ifdef BUILD_RASSUPPORT
	this->mpRasconn = 0;
	this->mpPetaView = 0;
	this->mbNoRasdaman = true;
#endif
	this->mbNoRasdaman = false;

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
	qRegisterMetaType<NMOtbAttributeTableWrapper>("NMOtbAttributeTableWrapper");
	//qRegisterMetaType<NMModelComponent>("NMModelComponent");
	//qRegisterMetaType<NMModelComponent*>("NMModelComponent*");

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
    tabWidget->setVerticalHeader(0);
    tabWidget->setColumnCount(2);
    tabWidget->setRowCount(1);
    tabWidget->clear();
    ui->infoWidgetList->addWidgetItem(tabWidget, QString::fromUtf8("Layer Info"));

    mTreeCompEditor = new NMComponentEditor(ui->infoWidgetList);
    ui->infoWidgetList->addWidgetItem(mTreeCompEditor, QString::fromUtf8("Model Component Info"));

    ui->componentInfoDock->setVisible(false);

    // ================================================
    // MODEL COMPONENT DOCK (Source)
    // ================================================

    mLayerList = new ModelComponentList(ui->compWidgetList);
    mLayerList->setObjectName(QString::fromUtf8("modelCompList"));
    ui->compWidgetList->addWidgetItem(mLayerList, QString::fromUtf8("Map Layers"));

    NMProcCompList* procList = new NMProcCompList(ui->compWidgetList);
    procList->setObjectName(QString::fromUtf8("processComponents"));
    ui->compWidgetList->addWidgetItem(procList, QString::fromUtf8("Model Components"));

    // ================================================
    // BAR(s) SETUP - MENU - PROGRESS - STATUS
    // ================================================


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
    connect(ui->actionModel_View, SIGNAL(triggered()), this, SLOT(showModelView()));
    connect(ui->actionRemoveAllObjects, SIGNAL(triggered()), this, SLOT(removeAllObjects()));
    connect(ui->actionFullExtent, SIGNAL(triggered()), this, SLOT(zoomFullExtent()));
    connect(ui->actionSaveAsVTKPolyData, SIGNAL(triggered()), this, SLOT(saveAsVtkPolyData()));
    connect(ui->actionTest, SIGNAL(triggered()), this, SLOT(test()));
    connect(ui->actionSaveAsVectorLayerOGR, SIGNAL(triggered()), this, SLOT(saveAsVectorLayerOGR()));
    connect(ui->actionImportODBC, SIGNAL(triggered()), this, SLOT(importODBC()));
    connect(ui->actionLUMASS, SIGNAL(triggered()), this, SLOT(aboutLUMASS()));

    connect(ui->actionShow_Map_View, SIGNAL(toggled(bool)), this, SLOT(showMapView(bool)));
    connect(ui->actionShow_Model_View, SIGNAL(toggled(bool)), this, SLOT(showModelView(bool)));
    connect(ui->actionMap_View_Mode, SIGNAL(triggered()), this, SLOT(mapViewMode()));
    connect(ui->actionModel_View_Mode, SIGNAL(triggered()), this, SLOT(modelViewMode()));

    connect(ui->componentsWidget, SIGNAL(visibilityChanged(bool)),
            ui->actionComponents_View, SLOT(setChecked(bool)));
    connect(ui->componentInfoDock, SIGNAL(visibilityChanged(bool)),
            ui->actionShow_Components_Info, SLOT(setChecked(bool)));

    // **********************************************************************
	// *                    MODEL BUILDER WINDOW                            *
	// **********************************************************************

    mModelBuilderWindow = new QMainWindow(this);
    mModelBuilderWindow->setWindowFlags(Qt::Widget);
    mModelBuilderWindow->addToolBar(this->ui->mainToolBar);
    mModelBuilderWindow->setCentralWidget(ui->modelViewWidget);

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
    QAction* moveAction = new QAction(moveIcon, "Move Scene or Component",
    		this->ui->mainToolBar);
    moveAction->setCheckable(true);
    moveAction->setChecked(true);

    QIcon linkIcon(":link-icon.png");
    QAction* linkAction = new QAction(linkIcon, "Link Components", this->ui->mainToolBar);
    linkAction->setCheckable(true);

    QIcon selIcon(":select-icon.png");
    QAction* selAction = new QAction(selIcon, "Select Components", this->ui->mainToolBar);
    selAction->setCheckable(true);

    QActionGroup* modelToolGroup = new QActionGroup(this->ui->mainToolBar);
    modelToolGroup->addAction(moveAction);
    modelToolGroup->addAction(linkAction);
    modelToolGroup->addAction(selAction);

    // ..........................
    // model execution actions
    QIcon execIcon(":model-execute-icon.png");
    QAction* execAction = new QAction(execIcon, "Execute Model", this->ui->mainToolBar);

    QIcon stopIcon(":model-stop-icon.png");
    QAction* stopAction = new QAction(stopIcon, "Stop Model Execution", this->ui->mainToolBar);

    QIcon resetIcon(":model-reset-icon.png");
    QAction* resetAction = new QAction(resetIcon, "Reset Model",  this->ui->mainToolBar);


    this->ui->mainToolBar->addActions(modelToolGroup->actions());
    this->ui->mainToolBar->addSeparator();

    this->ui->mainToolBar->addAction(zoomInAction);
    this->ui->mainToolBar->addAction(zoomOutAction);
    this->ui->mainToolBar->addAction(zoomToContent);

    this->ui->mainToolBar->addSeparator();
    this->ui->mainToolBar->addAction(resetAction);
    this->ui->mainToolBar->addAction(stopAction);
    this->ui->mainToolBar->addAction(execAction);


    // connect model view widget signals / slots
    connect(linkAction, SIGNAL(toggled(bool)),
    		this->ui->modelViewWidget, SIGNAL(linkToolToggled(bool)));
    connect(selAction, SIGNAL(toggled(bool)),
    		this->ui->modelViewWidget, SIGNAL(selToolToggled(bool)));
    connect(moveAction, SIGNAL(toggled(bool)),
    		this->ui->modelViewWidget, SIGNAL(moveToolToggled(bool)));

    connect(zoomInAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomIn()));
    connect(zoomOutAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomOut()));
    connect(zoomToContent, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(zoomToContent()));

    connect(execAction, SIGNAL(triggered()), this->ui->modelViewWidget, SLOT(executeModel()));
    connect(stopAction, SIGNAL(triggered()), this->ui->modelViewWidget, SIGNAL(requestModelAbortion()));
    connect(resetAction, SIGNAL(triggered()), this->ui->modelViewWidget, SIGNAL(resetModel()));


    // **********************************************************************
	// *                    VTK DISPLAY WIDGET                              *
	// **********************************************************************

    // create the render window
    vtkSmartPointer<vtkRenderWindow> renwin = vtkSmartPointer<vtkRenderWindow>::New();

    // set the number of allowed layers in the window
    renwin->SetAlphaBitPlanes(1);
    renwin->SetMultiSamples(0);
    renwin->SetNumberOfLayers(2);

	// set-up the background renderer
	this->mBkgRenderer = vtkSmartPointer<vtkRenderer>::New();
	this->mBkgRenderer->SetLayer(0);
	this->mBkgRenderer->SetBackground(0.7,0.7,0.7);
	//this->mBkgRenderer->SetUseDepthPeeling(1);
	//this->mBkgRenderer->SetMaximumNumberOfPeels(100);
	//this->mBkgRenderer->SetOcclusionRatio(0.1);


	//	this->mBkgRenderer->SetBackground(0,0,0);
	renwin->AddRenderer(this->mBkgRenderer);

    // set the render window
		// for supporting 3D mice (3DConnexion Devices)
		//this->ui->qvtkWidget->SetUseTDx(true);
    this->ui->qvtkWidget->SetRenderWindow(renwin);

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

	m_orientwidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
	m_orientwidget->SetOrientationMarker(axes);
	m_orientwidget->SetInteractor(static_cast<vtkRenderWindowInteractor*>(this->ui->qvtkWidget->GetInteractor()));
	m_orientwidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	m_orientwidget->InteractiveOff();
	m_orientwidget->SetEnabled(0);

    this->ui->qvtkWidget->GetInteractor()->SetInteractorStyle(
    		vtkInteractorStyleImage::New());

    // QVTKWidget Events---------------------------------------------------------
    this->m_vtkConns = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::MouseMoveEvent,
    		this, SLOT(updateCoords(vtkObject*)));
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::LeftButtonPressEvent,
    		this, SLOT(pickObject(vtkObject*)));
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::MouseWheelForwardEvent,
    		this, SLOT(zoomChanged(vtkObject*)));
    this->m_vtkConns->Connect(ui->qvtkWidget->GetRenderWindow()->GetInteractor(),
    		vtkCommand::MouseWheelBackwardEvent,
    		this, SLOT(zoomChanged(vtkObject*)));

    // **********************************************************************
    // *                    CENTRAL WIDGET                                  *
    // **********************************************************************

    QVBoxLayout* boxL = new QVBoxLayout();
    if (ui->centralWidget->layout())
        delete ui->centralWidget->layout();

    ui->centralWidget->setLayout(boxL);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->setChildrenCollapsible(true);
    splitter->addWidget(ui->qvtkWidget);
    splitter->addWidget(mModelBuilderWindow);
    boxL->addWidget(splitter);

    // ================================================
    // INITIAL WIDGET's VISIBILITY
    // ================================================

//    this->ui->compWidgetList->setWidgetVisibile(0, true);
//    this->ui->compWidgetList->setWidgetVisibile(1, true);

    this->ui->componentsWidget->setVisible(true);
    this->ui->actionComponents_View->setChecked(true);

    this->ui->infoWidgetList->setWidgetItemVisible(0, false);
    this->ui->infoWidgetList->setWidgetItemVisible(1, false);

    this->ui->infoWidgetList->setWidgetItemVisible(0, true);
    this->ui->infoWidgetList->setWidgetItemVisible(1, true);

    this->ui->componentInfoDock->setVisible(false);
    this->ui->actionShow_Components_Info->setChecked(false);

    // set menu's check buttons to right state
    this->ui->actionShow_Map_View->setChecked(true);
    this->ui->actionShow_Model_View->setChecked(true);
}

OtbModellerWin::~OtbModellerWin()
{
	NMDebugCtx(ctxOtbModellerWin, << "...");

	// close the table view and delete;
	if (this->mpPetaView != 0)
	{
		this->mpPetaView->close();
		delete this->mpPetaView;
	}

#ifdef BUILD_RASSUPPORT
	if (this->mpRasconn)
		delete this->mpRasconn;
#endif

	NMDebugAI(<< "delete ui ..." << endl);
	delete ui;

	const QObjectList& kids = this->children();
	QObject *child;

	NMDebugAI(<< "going to delete child objects ..." << endl);
	foreach (child, kids)
	{
//		NMDebugAI(<< child->objectName().toStdString() << ": "
//				<< child->metaObject()->className() << endl);
		delete child;
		child = 0;
	}

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

void
OtbModellerWin::mapViewMode()
{
    ui->qvtkWidget->setVisible(true);
    ui->actionShow_Map_View->setChecked(true);

    ui->compWidgetList->setWidgetItemVisible(0, true);
    ui->compWidgetList->setWidgetItemVisible(1, false);

    ui->infoWidgetList->setWidgetItemVisible(0, true);
    ui->infoWidgetList->setWidgetItemVisible(1, false);

    ui->componentsWidget->setVisible(true);
    ui->actionComponents_View->setChecked(true);

    ui->infoDock->setVisible(true);
    ui->actionShow_Components_Info->setChecked(true);

    ui->modelViewWidget->parentWidget()->setVisible(false);
    ui->actionShow_Model_View->setChecked(false);
}

void
OtbModellerWin::modelViewMode()
{
    ui->qvtkWidget->setVisible(false);
    ui->actionShow_Map_View->setChecked(false);

    ui->compWidgetList->setWidgetItemVisible(0, false);
    ui->compWidgetList->setWidgetItemVisible(1, true);

    ui->infoWidgetList->setWidgetItemVisible(0, false);
    ui->infoWidgetList->setWidgetItemVisible(1, true);

    ui->componentsWidget->setVisible(true);
    ui->actionComponents_View->setChecked(true);

    ui->infoDock->setVisible(true);
    ui->actionShow_Components_Info->setChecked(true);

    ui->modelViewWidget->parentWidget()->setVisible(true);
    ui->actionShow_Model_View->setChecked(true);
}

void
OtbModellerWin::showMapView(bool vis)
{
    ui->qvtkWidget->setVisible(vis);
}

void
OtbModellerWin::showModelView(bool vis)
{
    ui->modelViewWidget->parentWidget()->setVisible(vis);
}

void
OtbModellerWin::zoomChanged(vtkObject* obj)
{
	//NMDebugAI(<< "zoomed" << endl);
	//ui->qvtkWidget->update();

    //for (int i=0; i < mLayerList->getLayerCount(); ++i)
	//{
    //	mLayerList->getLayer(i);
	//}


	//vtkRendererCollection* rencoll = ui->qvtkWidget->GetRenderWindow()->GetRenderers();
	//vtkRenderer* ren = rencoll->GetFirstRenderer();
	//while (ren != 0)
	//{
	//	//ren->Render();
    //	//ren->ResetCamera(mLayerList->getMapBBox());
	//	ren->Render();
	//	ren = rencoll->GetNextItem();
	//}
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
		return 0;

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
	QString vinfo = QString("Version %1.%2.%3 - %4\nlast updated %5").arg(_lumass_version_major)
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
			<< "User group: https://groups.google.com/forum/#!forum/lumass-users" << endl;

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

	// create the output layer
//	OGRLayer* outLayer = ds->CreateLayer(l->objectName().toStdString().c_str(), 0,
//			)

	// make a list of available attributes
	vtkDataSet* vtkDS = const_cast<vtkDataSet*>(vL->getDataSet());
	vtkDataSetAttributes* dsAttr = vtkDS->GetAttributes(vtkDataSet::CELL);

	int numFields = dsAttr->GetNumberOfArrays();

	// clean up OGR stuff
	// destroy the data source object
	OGRDataSource::DestroyDataSource(ds);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
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
		NMDebugCtx(ctxOtbModellerWin, << "done!");
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

	bool b = true;
	bool a = false;

	QString astr = QString("true=%1 | false=%2").arg(b).arg(a);
	NMDebugAI(<< astr.toStdString() << std::endl);

	QString strue = "true";
	QString sfalse = "false";

	bool bok;
	bool c = strue.toInt(&bok);
	bool d = sfalse.toInt(&bok);

	NMDebugAI(<< "strue=" << c << " | sfalse=" << d << std::endl);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
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

	double wPt[3];

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
		NMDebugAI(<< "analysing cells ..." << endl);
		for (long c = 0; c < ncells; c++)
		{
			if (hole->GetTuple1(c) == 0)
			{
				//		NMDebugAI( << "cell (nmid) " << nmids->GetTuple1(c) << ":" << endl);
				cell = pd->GetCell(c);
				in = this->ptInPoly2D(wPt, cell);
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
			//l->updateLayerSelection(lstCellId,lstNMId, NMLayer::NM_SEL_CLEAR);
			l->selectCell(0, NMLayer::NM_SEL_CLEAR);
			this->updateLayerInfo(l, -1);
			return;
		}

		// select cell
		NMLayer::NMLayerSelectionType seltype;
		if (iren->GetControlKey())
			seltype = NMLayer::NM_SEL_ADD;
		else
			seltype = NMLayer::NM_SEL_NEW;

		cellId = vIds.last();
		long nmid = nmids->GetTuple1(cellId);
		lstCellId.push_back(cellId);
		lstNMId.push_back(nmid);
		this->updateLayerInfo(l, cellId);
		l->selectCell(cellId, seltype);
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

		if (img == 0)
			return;

		int did[3] = {-1,-1,-1};
		il->world2pixel(wPt, did);
		for (unsigned int d=0; d < 3; ++d)
		{
			if (did[d] < 0)
			{
				this->updateLayerInfo(l, -1);
				l->selectCell(cellId, NMLayer::NM_SEL_CLEAR);
				return;
			}
		}

		cellId = img->GetScalarComponentAsDouble(did[0], did[1], did[2], 0);

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

bool OtbModellerWin::ptInPoly2D(double pt[3], vtkCell* cell)
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

//	NMDebugAI(<< "test point: " << pt[0] << " " << pt[1] << " "
//	          << pt[3] <<  endl);

	// we assume here, that the points are ordered either anti-clockwise or clockwise
	// iterate over polygon points, create line segments
	double s1[2];
	double s2[2];
	double bs;
	double as;
	double segbnd[4];
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
		/// TODO: account for 0 slope of ray
		as = (s2[1] - s1[1]) / (s2[0] - s1[0]);
		bs = s1[1] - (as * s1[0]);

		// fill load vector and parameter matrix
		x[0] = pt[1];								// b of test ray
		x[1] = bs;      							// b of polygon segment
		a[0][0] = 0; 		 a[0][1] = 1;			// coefficients for x and y of test ray
		a[1][0] = as * (-1); a[1][1] = 1;			// coefficients for x and y of polygon segment

		// DEBUG
//		NMDebugAI(<< "S-" << i+1 << ": " << "S1(" << s1[0] << ", " << s1[1] << ") "
//				  << "S2(" << s2[0] << ", " << s2[1] << "): y = " << as << "x + " << bs);

//		NMDebugAI(<< "Linear System: ..." << endl);
//		NMDebugAI(<< x[0] << " = " << a[0][0] << " " << a[0][1] << endl);
//		NMDebugAI(<< x[1] << " = " << a[1][0] << " " << a[1][1] << endl);

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
//		NMDebug( << "\t\thit: " << onseg << endl);

	}
	//NMDebugAI(<< "total hits: " << retcnt << endl << endl);

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

	double wPt[3];
	vtkInteractorObserver::ComputeDisplayToWorld(this->mBkgRenderer,
			event_pos[0], event_pos[1], 0, wPt);
	wPt[2] = 0;

	// update label
	QString s = QString("Map Location: X: %1 Y: %2"). // Z: %3").
	arg(wPt[0], 0, 'f', 5).arg(wPt[1], 0, 'f', 5); //.arg(wPt[3],0,'f',2);

	this->m_coordLabel->setText(s);

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

	NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
	QString pixval = "";
	int did[3] = {-1,-1,-1};
	il->world2pixel(wPt, did);
	for (unsigned int d=0; d < 3; ++d)
	{
		if (did[d] < 0)
		{
			this->mPixelValLabel->setText(pixval);
			return;
		}
	}

	stringstream cvs;
	for (unsigned int d=0; d < img->GetNumberOfScalarComponents(); ++d)
	{
		cvs << img->GetScalarComponentAsDouble(did[0], did[1], did[2], d)
				<< " ";
	}

	// get the color window and level settings
	//double pixelvalue = img->GetScalarComponentAsDouble(did[0], did[1], did[2], 0);
    //
	////vtkIdType ptId = img->ComputePointId(did);
	////std::string ptval = img->GetPointData()->GetScalars()->GetVariantValue(ptId).ToString();
    //
    //
	//vtkImageSlice* slice = vtkImageSlice::SafeDownCast(const_cast<vtkProp3D*>(l->getActor()));
	//vtkImageProperty* iprop = slice->GetProperty();
	//double cwin = iprop->GetColorWindow();
	//double clevel = iprop->GetColorLevel();
    //
	//double lower = clevel - (cwin * 0.5);
	//double upper = clevel + (cwin * 0.5);

	pixval = QString(" Pixel (%1, %2, %3) = %4").//  | Window, Level, Range = %5, %6, %7-%8").
				arg(did[0]).arg(did[1]).arg(did[2]).
				arg(cvs.str().c_str());//.
				//arg(cwin, 0, 'f', 2).arg(clevel, 0, 'f', 2).
				//arg(lower, 0, 'f', 2).arg(upper, 0, 'f', 2);


	this->mPixelValLabel->setText(pixval);

}

void
OtbModellerWin::showBusyStart()
{
	NMDebugAI(<< this->sender()->objectName().toStdString() << " - turned busy!" << std::endl);
	QString msg = QString("processing %1").arg(this->sender()->objectName());
	this->m_StateMsg->setText(msg);
	this->mProgressBar->reset();
	this->mProgressBar->setVisible(true);
	this->mProgressBar->setMinimum(0);
	this->mProgressBar->setMaximum(0);
}

void
OtbModellerWin::showBusyEnd()
{
	NMDebugAI(<< this->sender()->objectName().toStdString() << " - stopped processing!" << std::endl);
	this->m_StateMsg->setText("");
	this->mProgressBar->setVisible(false);
}

void OtbModellerWin::showComponentsView(bool vis)
{
    this->ui->componentsWidget->setVisible(vis);
}

void OtbModellerWin::showComponentsInfoView(bool vis)
{
    this->ui->componentInfoDock->setVisible(vis);
}


void OtbModellerWin::doMOSObatch()
{
	return;

	NMDebugCtx(ctxOtbModellerWin, << "...");

	QString fileName = "/home/alex/projects/HBRC_EnviroLink/sensitivity/scenario_files/r1_minNleach_ConstAgrProd.los";
	QString dsName = "/home/alex/projects/HBRC_EnviroLink/sensitivity/data/r1sens.vtk";
	//QString fileName = QFileDialog::getOpenFileName(this,
	//     tr("Open Optimisation Settings"), "~", tr("LUMASS Optimisation Settings (*.los)"));
    //
	//if (fileName.isNull())
	//{
	//	NMDebugAI( << "Please provide a filename!" << endl);
	//	return;
	//}

	QFileInfo fileinfo(fileName);
	QFileInfo dsInfo(dsName);

	QString path = fileinfo.path();
	QString baseName = fileinfo.baseName();
	if (!fileinfo.isReadable())
	{
		NMErr(ctxNMMosra, << "Could not read file '" << fileName.toStdString() << "'!");
		return;
	}

	// create a new optimisation object
	NMMosra* mosra = new NMMosra(this);

	for (int runs=5; runs < 7; ++runs)
	{
		NMDebugAI(<< "******** PERTURBATION #" << runs+1 << " *************" << endl);
		// load the file with optimisation settings
		mosra->loadSettings(fileName);

		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(dsName.toStdString().c_str());
		reader->Update();
		vtkPolyData* pd = reader->GetOutput();
		mosra->setDataSet(pd);
		mosra->perturbCriterion("Nleach", 5);
		vtkSmartPointer<vtkTable> tab = mosra->getDataSetAsTable();

		mosra->setTimeOut(180);
		if (!mosra->solveLp())
			continue;

		if (!mosra->mapLp())
			continue;

		vtkSmartPointer<vtkTable> sumres = mosra->sumResults();

		// get rid of admin fields
		tab->RemoveColumnByName("nm_id");
		tab->RemoveColumnByName("nm_hole");
		tab->RemoveColumnByName("nm_sel");

		// now write the input and the result table
		QString perturbName = QString("%1/%2_p%3.csv").arg(dsInfo.path())
				.arg(dsInfo.baseName()).arg(runs+1);

		QString resName = QString("%1/res_%2_p%3.csv").arg(dsInfo.path())
						.arg(dsInfo.baseName()).arg(runs+1);

		vtkDelimitedTextWriter* writer = vtkDelimitedTextWriter::New();
		writer->SetFieldDelimiter(",");

        writer->SetInputData(tab);
		writer->SetFileName(perturbName.toStdString().c_str());
		writer->Update();

        writer->SetInputData(sumres);
		writer->SetFileName(resName.toStdString().c_str());
		writer->Update();

		writer->Delete();
	}

	NMDebugCtx(ctxOtbModellerWin, << "done!");
}

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

	QString path = fileinfo.path();
	QString baseName = fileinfo.baseName();
	if (!fileinfo.isReadable())
	{
		NMErr(ctxNMMosra, << "Could not read file '" << fileName.toStdString() << "'!");
		return;
	}

	NMDebugAI( << "reading settings from " << fileName.toStdString() << endl);

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

//	NMDebugAI(<< "split off solving to seperate thread ... " << endl);
//	QFuture<int> future = QtConcurrent::run(mosra, &NMMosra::solveLp);
//
//	NMDebugAI(<< "waiting 10 secs ..." << endl);
//	::sleep(10);
//
//	NMDebugAI(<< "cancel solving ..." << endl);
//	mosra->cancelSolving();


	// asking the user for the lp timeout
	bool timeok;
	int timeout = QInputDialog::getInt(this, tr("lp_solve timeout"), tr("timeout in secs:"), 60, 5, 86400, 30, &timeok);
	if (!timeok)
		return;
	mosra->setTimeOut(timeout);

	//	if (!future.result())
	if (!mosra->solveLp())
	{
		NMDebugAI(<< "encountered trouble setting/solving the problem!" << std::endl);
		delete mosra;
		return;
	}

	// ============================================================================


	int solved = mosra->mapLp();
	//layer->emitDataSetChanged();
	layer->tableDataChanged(QModelIndex(), QModelIndex());


	if (solved)
	{
		NMDebugAI( << "visualising optimisation results ..." << endl);
		vtkSmartPointer<vtkTable> resTab = mosra->sumResults();
		// show table if we got one
		if (resTab.GetPointer() != 0)
		{
			vtkQtTableModelAdapter* tabModel = new vtkQtTableModelAdapter(this);
			NMFastTrackSelectionModel* ftsm = new NMFastTrackSelectionModel(tabModel, this);
			tabModel->setTable(resTab);
			NMTableView* tv = new NMTableView(tabModel, this);
			tv->setSelectionModel(ftsm);
			tv->setTitle(tr("Optimisation results!"));
			tv->show();
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

	QString sRepName = path + QString(tr("/report_%1.%2")).arg(baseName).arg(tr("txt"));
	NMDebugAI( << "write report to '" << sRepName.toStdString() << "'" << endl);
	mosra->writeReport(sRepName);

	QString lpName = path + QString(tr("/lp_%1.lp")).arg(baseName);
	mosra->getLp()->WriteLp(lpName.toStdString());
	delete mosra;

	if (solved)
	{
		NMDebugAI( << "mapping unique values ..." << endl);
		//NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(layer);
		//vl->mapUniqueValues(tr("OPT_STR"));
		layer->setLegendType(NMLayer::NM_LEGEND_INDEXED);
		layer->setLegendClassType(NMLayer::NM_CLASS_UNIQUE);
		layer->setLegendValueField("OPT_STR");
		layer->setLegendDescrField("OPT_STR");
		layer->updateLegend();
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
	dlg.setWindowTitle(tr("Save As VTK PolyData File (binary/XML)"));
	dlg.setDirectory("~/");
    dlg.setNameFilter("XML PolyData (*.vtp);;Binary PolyData (*.vtk)");

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

	if (selectedFilter == "XML PolyData (*.vtp)")
	{
		NMDebugAI(<< "writing XML-file " << fileName.toStdString() << " ..." << endl);
		vtkSmartPointer<vtkXMLPolyDataWriter> xw = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
		//if (xw.GetPointer() == 0)
		//{
		//	NMDebugAI(<< "XML PolyDataWriter is NULL!");
		//	return;
		//}
        xw->SetInputData(const_cast<vtkDataSet*>(l->getDataSet()));
		xw->SetFileName(fileName.toStdString().c_str());
		//xw->SetDataModeToAscii();
		xw->Write();
		//xw->Update();
	}
	else
	{
		NMDebugAI(<< "saving binary *.vtk file ..." << endl);
		vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
		writer->SetFileName(fileName.toStdString().c_str());
        writer->SetInputData(const_cast<vtkDataSet*>(l->getDataSet()));
		writer->SetFileTypeToBinary();
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

//        // skip incomplete data sets
//        if (entr.size() < 3)
//         continue;

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
				for (int f=0; f < nfields; ++f)
				{
					switch(pFeat->GetFieldDefnRef(f)->GetType())
					{
					case OFTInteger:
						iar = vtkIntArray::SafeDownCast(attr[f]);
						iar->InsertNextValue(pFeat->GetFieldAsInteger(f));
						break;
					case OFTReal:
						dar = vtkDoubleArray::SafeDownCast(attr[f]);
						dar->InsertNextValue(pFeat->GetFieldAsDouble(f));
						break;
					default:
						sar = vtkStringArray::SafeDownCast(attr[f]);
						sar->InsertNextValue(pFeat->GetFieldAsString(f));
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
	for (int f=0; f < nfields; ++f)
		vtkVect->GetCellData()->AddArray(attr[f]);

	vtkVect->BuildCells();
	vtkVect->BuildLinks();

	NMDebugAI(<< featCounter << " features imported" << endl);

	NMDebugCtx(ctxOtbModellerWin, << "done!");
	return vtkVect;
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

//	QThread* loader = new QThread;
//	layer->moveToThread(loader);
//	connect(layer, SIGNAL(layerProcessingEnd(const QString &)),
//			loader, SLOT())

	layer->setObjectName(layerName);
	this->connectImageLayerProcSignals(layer);
	QtConcurrent::run(layer, &NMImageLayer::setFileName, fileName);

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
	NMDebugCtx(ctxOtbModellerWin, << "...");

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
//			vtkCamera* c = vtkCamera::New();
//			c->DeepCopy(cam0);
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

	this->ui->qvtkWidget->update();


}

void OtbModellerWin::toggle3DStereoMode()
{
		this->ui->qvtkWidget->GetRenderWindow()->SetStereoRender(
	    		!this->ui->qvtkWidget->GetRenderWindow()->GetStereoRender());
}


