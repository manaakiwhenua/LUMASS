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
#include <limits>

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif
#include "NMGlobalHelper.h"

#include "modelcomponentlist.h"
#include "lumassmainwin.h"
#include "NMImageLayer.h"
#include "NMVectorLayer.h"
#include "NMComponentListItemDelegate.h"
#include "NMProcessComponentItem.h"
#include "otbAttributeTable.h"
#include "NMModelController.h"
#include "NMModelComponent.h"
#include "NMItkDataObjectWrapper.h"
#include "NMChartView.h"

#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMainWindow>
#include <QListWidgetItem>
#include <QSharedPointer>
#include <QAbstractItemView>
#include <QTreeWidgetItem>
#include <QTreeView>
#include <QMenu>
#include <QAction>
#include <QItemSelection>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QInputDialog>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QImage>
#include <QHeaderView>
#include <QMessageBox>
#include <QColorDialog>
#include <QGraphicsItem>
#include <QFileInfo>
#include <QtConcurrent>
#include <QFileDialog>
#include <QSqlQuery>

#include "vtkSmartPointer.h"
#include "vtkObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkCamera.h"
#include "vtkProperty.h"
#include "QVTKOpenGLWidget.h"
#include "vtkCell.h"
#include "vtkPolygon.h"
#include "vtkPolyDataReader.h"
#include "vtkChartHistogram2D.h"
#include "vtkImageData.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkIdTypeArray.h"
#include "vtkChartBox.h"
#include "vtkPlot.h"
#include "vtkPlotBox.h"
#include "vtkChartXY.h"
#include "vtkPlotBar.h"
#include "vtkNew.h"
#include "vtkAxis.h"
#include "vtkFloatArray.h"
#include "vtkRenderWindow.h"

#include "vtkAbstractMapper.h"
#include "vtkMapper.h"

// for testing
#include "NMSqlTableView.h"

#include "nmqsql_sqlite_p.h"
#include "nmqsqlcachedresult_p.h"

// end

const std::string ModelComponentList::ctx = "ModelComponentList";

ModelComponentList::ModelComponentList(QWidget *parent)
       :QTreeView(parent), mLastLayer(nullptr)
{
	this->mLayerModel = new NMLayerModel(this);
	this->setModel(this->mLayerModel);

    this->mVTKConn = vtkSmartPointer<vtkEventQtSlotConnect>::New();

    mbWholeImgStats = false;

	// set the general column count for this control
	this->setHeaderHidden(true);
	this->setRootIsDecorated(false);
	this->setExpandsOnDoubleClick(true);
	this->setUniformRowHeights(false);

	this->setSelectionBehavior(QAbstractItemView::SelectRows);

	this->viewport()->setAcceptDrops(true);
	this->viewport()->installEventFilter(this);

	this->reset();

    // do some stuff we can't do here
    mDelegate = new NMComponentListItemDelegate(this);
    this->setItemDelegate(mDelegate);

    // set up the logger
    mLogger = new NMLogger(this);
    mLogger->setHtmlMode(true);

    connect(mLogger, SIGNAL(sendLogMsg(QString)), NMGlobalHelper::getLogWidget(),
            SLOT(insertHtml(QString)));

    /* =============================================================
                        GENERAL LAYER CONTEXT MENU
    ================================================================ */
	this->mMenu = new QMenu(this);

	QAction* actZoom = new QAction(this->mMenu);
	actZoom->setText(tr("Zoom To Layer"));

    mActZoomSel = new QAction(this->mMenu);
    mActZoomSel->setText(tr("Zoom To Selection"));

    QAction* actTable = new QAction(this->mMenu);
	actTable->setText(tr("Open Attribute Table"));
	QAction* actSaveChanges = new QAction(this->mMenu);
	actSaveChanges->setText(tr("Save Changes"));
	QAction* actRemove = new QAction(this->mMenu);
	actRemove->setText(tr("Remove Layer"));

    mActValueStats = new QAction(this->mMenu);
    mActValueStats->setText(tr("Value Field Statistics"));
    mActImageStats = new QAction(this->mMenu);
    mActImageStats->setText(tr("Whole Image Pixel Statistics"));
    mActImageInfo = new QAction(this->mMenu);
    mActImageInfo->setText(tr("Show Image Information"));
    mActImageHistogram = new QAction(this->mMenu);
    mActImageHistogram->setText(tr("Visible Pixel Histogram"));


    mActUniqueValues = new QAction(this->mMenu);
    mActUniqueValues->setText(tr("Map Unique Values ..."));
    mActSingleSymbol = new QAction(this->mMenu);
    mActSingleSymbol->setText(tr("Map Single Symbol"));
    mActClrTab = new QAction(this->mMenu);
    mActClrTab->setText(tr("Map Colour Table"));
    mActClrRamp = new QAction(this->mMenu);
    mActClrRamp->setText(tr("Map Value Range"));
    mActRGBImg = new QAction(this->mMenu);
    mActRGBImg->setText(tr("Map RGB Image"));

    mActStretchClrRamp = new QAction(this->mMenu);
    mActStretchClrRamp->setText(tr("Stretch Colour Ramp"));

    mActOpacity = new QAction(this->mMenu);
    mActOpacity->setText(tr("Layer Opacity ..."));

    QAction* actSelColor = new QAction(this->mMenu);
    actSelColor->setText(tr("Selection Colour ..."));

    QAction* actLoadLegend = new QAction(this->mMenu);
    actLoadLegend->setText(tr("Load Legend ..."));

    QAction* actSaveLegend = new QAction(this->mMenu);
    actSaveLegend->setText(tr("Save Legend ..."));

    this->mActExportColourRamp = new QAction(this->mMenu);
    mActExportColourRamp->setText(tr("Save Colour Ramp ..."));

	this->mMenu->addAction(actTable);
	this->mMenu->addSeparator();

    this->mMenu->addAction(actZoom);
    this->mMenu->addAction(mActZoomSel);
	this->mMenu->addSeparator();

    this->mMenu->addAction(mActImageInfo);
    this->mMenu->addAction(mActValueStats);
    this->mMenu->addAction(mActImageHistogram);
    this->mMenu->addAction(mActImageStats);
	this->mMenu->addSeparator();

    this->mMenu->addAction(mActSingleSymbol);
    this->mMenu->addAction(mActUniqueValues);
    this->mMenu->addAction(mActClrTab);
    this->mMenu->addAction(mActClrRamp);
    this->mMenu->addAction(mActRGBImg);
    this->mMenu->addAction(mActStretchClrRamp);
    this->mMenu->addSeparator();

    this->mMenu->addAction(mActOpacity);
    this->mMenu->addAction(actSelColor);
	this->mMenu->addSeparator();

    this->mMenu->addAction(actLoadLegend);
    this->mMenu->addAction(actSaveLegend);
    this->mMenu->addAction(mActExportColourRamp);
    this->mMenu->addSeparator();
    this->mMenu->addAction(actSaveChanges);
	this->mMenu->addAction(actRemove);

    this->connect(actLoadLegend, SIGNAL(triggered()), this, SLOT(loadLegend()));
    this->connect(actSaveLegend, SIGNAL(triggered()), this, SLOT(saveLegend()));
	this->connect(actZoom, SIGNAL(triggered()), this, SLOT(zoomToLayer()));
    this->connect(mActZoomSel, SIGNAL(triggered()), this, SLOT(zoomToSelection()));
	this->connect(actTable, SIGNAL(triggered()), this, SLOT(openAttributeTable()));
	this->connect(actRemove, SIGNAL(triggered()), this, SLOT(removeCurrentLayer()));
    this->connect(mActSingleSymbol, SIGNAL(triggered()), this, SLOT(mapSingleSymbol()));
    this->connect(mActUniqueValues, SIGNAL(triggered()), this, SLOT(mapUniqueValues()));
    this->connect(mActClrTab, SIGNAL(triggered()), this, SLOT(mapColourTable()));
    this->connect(mActClrRamp, SIGNAL(triggered()), this, SLOT(mapColourRamp()));
    this->connect(mActRGBImg, SIGNAL(triggered()), this, SLOT(mapRGBImage()));
	this->connect(actSaveChanges, SIGNAL(triggered()), this, SLOT(saveLayerChanges()));
    this->connect(mActValueStats, SIGNAL(triggered()), this, SLOT(showValueStats()));
    this->connect(mActImageStats, SIGNAL(triggered()), this, SLOT(wholeImgStats()));
    this->connect(mActImageHistogram, SIGNAL(triggered()), this, SLOT(showImageHistogram()));
    this->connect(mActOpacity, SIGNAL(triggered()), this, SLOT(editLayerOpacity()));
    this->connect(actSelColor, SIGNAL(triggered()), this, SLOT(editSelectionColour()));
    this->connect(mActImageInfo, SIGNAL(triggered()), this, SLOT(showImageInfo()));
    this->connect(mActExportColourRamp, SIGNAL(triggered()), this, SLOT(exportColourRamp()));
    this->connect(mActStretchClrRamp, SIGNAL(triggered()), this, SLOT(stretchColourRampToVisMinMax()));


#ifdef LUMASS_DEBUG

	QAction* testing = new QAction(this->mMenu);
	testing->setText(tr("Test ..."));
	this->mMenu->addSeparator();
	this->mMenu->addAction(testing);

	this->connect(testing, SIGNAL(triggered()), this, SLOT(test()));

#endif


    /* =============================================================
                        VECTOR CONTOUR MENU
    ================================================================ */

    this->mContourMenu = new QMenu(this);

    QAction* actContourColour = new QAction(this->mContourMenu);
    actContourColour->setText(tr("Contour Colour ..."));
    QAction* actContourWidth = new QAction(this->mContourMenu);
    actContourWidth->setText(tr("Contour Width ..."));
    QAction* actContourStyle = new QAction(this->mContourMenu);
    actContourStyle->setText(tr("Contour Style ..."));
    mActVecContourOnly = new QAction(this->mContourMenu);
    mActVecContourOnly->setText(tr("Map Contours Only"));
    mActVecContourOnly->setCheckable(true);
    mActVecContourOnly->setChecked(false);

    this->mContourMenu->addAction(actContourColour);
    this->mContourMenu->addAction(actContourWidth);
    this->mContourMenu->addAction(actContourStyle);
    this->mContourMenu->addAction(mActVecContourOnly);


    this->connect(mActVecContourOnly, SIGNAL(triggered()), this, SLOT(mapVectorContoursOnly()));
    this->connect(actContourColour, SIGNAL(triggered()), this, SLOT(editContourColour()));
    this->connect(actContourWidth, SIGNAL(triggered()), this, SLOT(editContourWidth()));
    this->connect(actContourStyle, SIGNAL(triggered()), this, SLOT(editContourStyle()));

}

ModelComponentList::~ModelComponentList()
{
	delete this->mMenu;
	delete this->mLayerModel;
}

void
ModelComponentList::initView(void)
{
	// we do some init work here we can't do in the constructor
	this->setIconSize(QSize(35,16));
}

void ModelComponentList::openAttributeTable()
{
//	NMDebugCtx(ctx, << "...");

	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	if (l != 0)
		l->showAttributeTable();

//	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::exportColourRamp()
{
    NMLayer* l = this->getSelectedLayer();

    bool bok;
    QString theDir = getenv("HOME");
    QString rampFileName = QFileDialog::getSaveFileName(this,
                           tr("Export Colour Ramp"),
                           theDir,
                           tr("Portable Network Graphic (*.png);;JPEG Image (*.jpg *.jpeg)"));

    if (rampFileName.isEmpty())
    {
        return;
    }


    QString sizeStr = QInputDialog::getText(this, tr("Export Colour Ramp"),
                                           tr("Colour ramp image size: "),
                                            QLineEdit::Normal,
                                            QString("100, 600"), &bok);
    int width = 100;
    int height = 600;
    bool userSize = bok ? true : false;
    if (bok)
    {
        QStringList sizes = sizeStr.split(QString(","));
        if (sizes.size() >= 2)
        {
            int tw = sizes.at(0).toInt(&bok);
            if (bok)
            {
                width = tw;
            }
            else
            {
                userSize = false;
            }

            tw = sizes.at(1).toInt(&bok);
            if (bok && userSize)
            {
                height = tw;
            }
            else
            {
                userSize = false;
            }
        }
    }

    if (!userSize)
    {
        NMBoxErr("Export Colour Ramp", "Invalid image size specified!" );
        return;
    }


    QPixmap pix = l->getColourRampPix(width,height);

    QFileInfo nameInfo(rampFileName);
    QString format = nameInfo.suffix().toUpper();

    QFile rampFile(rampFileName);
    rampFile.open(QIODevice::WriteOnly);
    pix.save(&rampFile, format.toStdString().c_str());
    rampFile.close();
}

void ModelComponentList::saveLayerChanges()
{
	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	this->setCurrentIndex(QModelIndex());
	if (l != 0)
	{
		NMDebugAI(<< "going to save changes to the data set..." << endl);
		l->writeDataSet();
	}
}

void
ModelComponentList::showImageHistogram()
{
    NMLayer* l = this->getSelectedLayer();
    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (il == 0)
    {
        return;
    }

    il->showHistogram();
}

void
ModelComponentList::showImageInfo()
{
    NMLayer* l = this->getSelectedLayer();
    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (il == 0)
    {
        return;
    }

    const double* bbox = il->getBBox();
    const double* spac = il->getSignedSpacing();
    const double* orig = il->getOrigin();

    std::stringstream bandorder;
    std::vector<int> bands = il->getBandMap();
    if (bands.size())
    {
        for (int i=0; i < bands.size(); ++i)
        {
            bandorder << bands.at(i);
            if (i < bands.size()-1)
            {
                bandorder << ", ";
            }
        }
    }
    else
    {
        bandorder << "1";
    }

    std::stringstream ovv;
    std::vector<std::vector<int> > oss = il->getOverviewSizes();
    if (oss.size())
    {
        for (int s=0; s < oss.size(); ++s)
        {
            ovv << oss.at(s).at(0) << "x" << oss.at(s).at(1);
            if (s < oss.size()-1)
            {
                ovv << " ";
            }
        }
    }
    else
    {
        ovv << "n/a";
    }

    std::stringstream ovidx;
    if (il->getOverviewIndex() >= 0)
    {
        ovidx << il->getOverviewIndex()+1;
    }
    else
    {
        ovidx << "n/a";
    }

    std::stringstream sizestr;
    sizestr << "Size: ";
    std::stringstream pixsizestr;
    pixsizestr << "Pixel Size: ";

    for (int d=0; d < il->getNumDimensions(); ++d)
    {
        sizestr << (bbox[d*2+1] - bbox[d*2]) / ::abs(spac[d]);
        pixsizestr << ::abs(spac[d]);

        if (d < il->getNumDimensions()-1)
        {
            sizestr << " x ";
            pixsizestr << " x ";
        }
    }
    sizestr << std::endl;
    pixsizestr << std::endl;

    std::string typeStr = il->getImage()->getComponentTypeString(il->getImage()->getNMComponentType()).toStdString();

    std::stringstream ii;
    ii << setprecision(0) << fixed;

    ii << sizestr.str()
       << setprecision(2)
       << pixsizestr.str()
       << "Pixel Component Type: " << typeStr << std::endl
       << "Origin: " << orig[0] << ", " << orig[1] << std::endl
       << "Top Left Corner: " << bbox[0] << ", " << bbox[3]  << std::endl
       << "Bottom Right Corner: " << bbox[1] << ", " << bbox[2] << std::endl
       << setprecision(0)
       << "Total Number of Bands: " << il->getTotalNumBands() << std::endl
       << "Bands Displayed: " << bandorder.str() << std::endl
       << "Current Overview: " << ovidx.str() << std::endl
       << "Available Overviews: " << ovv.str() << std::endl
       << "File Name: " << il->getFileName().toStdString() << std::endl;


   QMessageBox::information(this, il->objectName(),
                            QString(ii.str().c_str()));

}

void ModelComponentList::removeCurrentLayer()
{
	const int toplevelrow = (this->currentIndex().internalId() / 100) - 1;
	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
	NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
	this->setCurrentIndex(QModelIndex());
	if (l == 0)
		return;

	if (l->hasChanged())
	{
		QMessageBox msgBox;
		QString text = QString(tr("Remove Layer '%1'")).arg(l->objectName());
		msgBox.setText(text);
		msgBox.setInformativeText("Do you want to save your changes before?");
		msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::Yes);
		int ret = msgBox.exec();

		if (ret == QMessageBox::Yes)
		{
			l->writeDataSet();
		}
	}

	this->removeLayer(l);
}

void ModelComponentList::loadLegend()
{
    NMLayer* l = this->getCurrentLayer();
    if (l == 0)
        return;

    QString fileName = QFileDialog::getOpenFileName(this,
         tr("Select Legend Colour Table"), "~", tr("Delimited Text File (*.csv)"));
    QFileInfo info(fileName);

    if (!info.isReadable())
    {
        return;
    }

    l->loadLegend(fileName);

}

void ModelComponentList::saveLegend()
{
    NMLayer* l = this->getCurrentLayer();
    if (l == 0)
        return;

    QString fileName = QFileDialog::getSaveFileName(this,
         tr("Save Legend File"), "~", tr("Delimited Text File (*.csv)"));

    if (fileName.isEmpty())
    {
        NMBoxErr("Save Legend File",
                  "Invalid File Name!");
        return;
    }

    l->saveLegend(fileName);

}

void ModelComponentList::updateLegend(const NMLayer* layer)
{
	//NMDebugAI( << "going to update the legend for " << layer->objectName().toStdString()
		//	<< endl);

	QModelIndex idx = this->mLayerModel->getItemLayerModelIndex(layer->objectName());
	if (this->isExpanded(idx))
	{
		this->collapse(idx);
		this->expand(idx);
	}



	this->update(idx);
}

//void
//ModelComponentList::zoomChanged(vtkObject* obj)
//{
//	this->topLevelWidget()->findChild<QVTKOpenGLWidget*>(tr("qvtkWidget"))->update();
//	NMDebugAI(<< "zoom" << endl);
//}

NMLayer*
ModelComponentList::getSelectedLayer()
{
	NMLayer* l = 0;

	QModelIndexList il = this->selectedIndexes();
	if (il.size() == 0)
		return l;

	const QModelIndex idx = il.at(0);
	const int stackpos = this->mLayerModel->toLayerStackIndex(idx.row());
	l = this->mLayerModel->getItemLayer(stackpos);

	return l;
}

int ModelComponentList::changeLayerPos(int oldpos, int newpos)
{
	// save the collapsed state of all layers
	QList<bool> collapsed;
	for (unsigned int r=0; r < this->mLayerModel->rowCount(QModelIndex()); ++r)
	{
		if (this->isExpanded(this->mLayerModel->index(r, 0, QModelIndex())))
			collapsed.push_back(false);
		else
			collapsed.push_back(true);
	}

	// swap places in collapsed list
	int oldtree = this->mLayerModel->toTreeModelRow(oldpos);
	int newtree = this->mLayerModel->toTreeModelRow(newpos);
	bool tc = collapsed.takeAt(oldtree);
	collapsed.insert(newtree, tc);

	// now swap layer places
	this->mLayerModel->changeItemLayerPos(oldpos, newpos);

	// reinstate layer's expansion setting
	for (unsigned int r=0; r < this->mLayerModel->rowCount(QModelIndex()); ++r)
	{
		QModelIndex id = this->mLayerModel->index(r, 0, QModelIndex());
		if (collapsed.at(r))
			this->collapse(id);
		else
			this->expand(id);
	}

	// update the map display window
    //this->topLevelWidget()->findChild<QVTKOpenGLWidget*>(tr("qvtkWidget"))->update();
    NMGlobalHelper::getRenderWindow()->Render();
	return oldpos;
}


bool ModelComponentList::removeLayer(QString layerName)
{
	NMLayer* l = this->mLayerModel->getItemLayer(layerName);
	if (l == 0)
		return false;

	this->removeLayer(l);
	return  true;
}

void ModelComponentList::removeLayer(NMLayer* layer)
{
	NMDebugCtx(ctx, << "...");

	disconnect(layer, SIGNAL(visibilityChanged(const NMLayer*)),
				this, SLOT(updateMapWin(const NMLayer*)));
	disconnect(layer, SIGNAL(legendChanged(const NMLayer*)),
			this, SLOT(updateLegend(const NMLayer*)));
	disconnect(this, SIGNAL(selectedLayerChanged(const NMLayer *)),
			layer, SLOT(selectedLayerChanged(const NMLayer *)));


    // double check whether we need to clear any currently displayed
    // layer info ...
    NMGlobalHelper helper;
    LUMASSMainWin* mwin = helper.getMainWindow();
    mwin->checkRemoveLayerInfo(layer);

    // remove layer from the model (which updates as well the layer position
    // held by each layer in the layer stack)
	this->mLayerModel->removeLayer(layer);
	this->reset();

    // update the map display window
    //this->topLevelWidget()->findChild<QVTKOpenGLWidget*>(tr("qvtkWidget"))->update();
    NMGlobalHelper::getRenderWindow()->Render();

    mwin->getRenderWindow()->SetNumberOfLayers(mLayerModel->getItemLayerCount()+2);
    vtkRenderer* scaleRenderer = const_cast<vtkRenderer*>(mwin->getScaleRenderer());
    scaleRenderer->SetLayer(this->mLayerModel->getItemLayerCount()+1);

	NMDebugCtx(ctx, << "done!");
}


void ModelComponentList::updateMapWin(const NMLayer* layer)
{
	// just do the whole window for now

	// TODO: later we have to get the layer's bbox, transform into the window
	// coordinates and then just update this very region of the renderwidget window

	//NMDebugAI( << "updating map window for " << layer->objectName().toStdString() << endl);

    NMGlobalHelper::getRenderWindow()->Render();
}


void ModelComponentList::addLayer(NMLayer* layer)
{
	NMDebugCtx(ctx, << "...");

	// TODO: check this
	if (layer == 0 || layer->getRenderer() == 0)
	{
		NMDebugAI(<< "invalid layer!" << endl);
		NMDebugCtx(ctx, << "done!");
		return;
	}

	// connect layer with updateMap routine
	// (i.e. whenever the visibility of the layer has changed it emits
	// the signal "visibilityChanged(const NMLayer*)" which is then being processed
	// by the slot "updateMapWin(const NMLayer*)"
	connect(layer, SIGNAL(visibilityChanged(const NMLayer*)),
				this, SLOT(updateMapWin(const NMLayer*)));
	connect(layer, SIGNAL(legendChanged(const NMLayer*)),
			this, SLOT(updateLegend(const NMLayer*)));
	connect(this, SIGNAL(selectedLayerChanged(const NMLayer *)),
			layer, SLOT(selectedLayerChanged(const NMLayer *)));

	LUMASSMainWin* mwin = qobject_cast<LUMASSMainWin*>(this->topLevelWidget());
    connect(layer, SIGNAL(notifyLastClickedRow(NMLayer *, long long)),
            mwin, SLOT(updateLayerInfo(NMLayer *, long long)));
    connect(mwin, SIGNAL(signalIsIn3DMode(bool)),
            layer, SLOT(setIsIn3DMode(bool)));

	// add the layer to the NMLayerModel
	this->mLayerModel->pileItemLayer(layer);
	int nlayers = this->mLayerModel->getItemLayerCount();
	this->recalcMapBBox();

	// get the camera of the background renderer
    //LUMASSMainWin* win = qobject_cast<LUMASSMainWin*>(this->topLevelWidget());
    vtkRenderer* bkgRen = const_cast<vtkRenderer*>(mwin->getBkgRenderer());

	// adjust the camera to the map bounding box if it is the first layer we're adding
	if (nlayers == 1)
		bkgRen->ResetCamera(this->mFullMapExt);

	// use the same camera for the newly added layer as for the initial layer
	vtkCamera* cam0 = bkgRen->GetActiveCamera();
	vtkRenderer* ren = const_cast<vtkRenderer*>(layer->getRenderer());
	ren->SetActiveCamera(cam0);

	// adjust the number of layers per this render window and add the renderer
	// to the render window
	// add the renderer to the render window and update the widget
    QVTKOpenGLWidget* qvtk = this->topLevelWidget()->findChild<QVTKOpenGLWidget*>(tr("qvtkWidget"));

	// we keep one layer more than required (and keep in mind that we've got the
	// the background layer as well
    qvtk->GetRenderWindow()->SetNumberOfLayers(nlayers+2);

    //layer->setLayerPos(nlayers-1);
    vtkRenderer* scaleRen = const_cast<vtkRenderer*>(mwin->getScaleRenderer());
    scaleRen->SetLayer(nlayers+1);

    qvtk->GetRenderWindow()->AddRenderer(ren);
    qvtk->GetRenderWindow()->Render();

	this->reset();
	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::recalcMapBBox(void)
{
	NMDebugCtx(ctx, << "...");

	int i;
	double* mb = this->mFullMapExt;

	// reset the actual full map bounding box
	for (i=0; i < 4; i +=2)
		mb[i] = std::numeric_limits<double>::max();

	for (i=1; i < 4; i +=2)
		mb[i] = std::numeric_limits<double>::max() * -1;

	mb[4] = 0; mb[5] = 0;

	int nlayers = this->mLayerModel->getItemLayerCount();
	NMLayer* l;
	for (i=0; i < nlayers; i++)
	{
		l = this->mLayerModel->getItemLayer(i);
		const double* box = l->getBBox();
		this->unionMapBBox(box);
		NMDebugAI( << "processing " << l->objectName().toStdString() <<
						"'s box ..." << endl);
	}

	NMDebugCtx(ctx, << "done!");
}


void ModelComponentList::unionMapBBox(const double* box)
{
	double* mb = this->mFullMapExt;
	int i;
	for (i=0; i < 6; i += 2)
	{
		if (box[i] < mb[i])
			mb[i] = box[i];
	}

	for (i=1; i < 6; i += 2)
	{
		if (box[i] > mb[i])
			mb[i] = box[i];
	}
}

const double* ModelComponentList::getMapBBox(void)
{
	return this->mFullMapExt;
}

void ModelComponentList::mouseDoubleClickEvent(QMouseEvent* event)
{
	//NMDebugCtx(ctx, << "...")

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
		//NMDebugCtx(ctx, << "done!")
		return;
	}

	// ---------------------------------------------------------------
	// COLLAPSE AND EXPAND CHILD NODES

	if (this->mLayerModel->rowCount(idx) > 0)
	{
		if (this->isExpanded(idx))
			this->collapse(idx);
		else
			this->expand(idx);
	}
	// ----------------------------------------------------------------
	// EDIT CHILD NODES
	else
	{
		const int toplevelrow = (idx.internalId() / 100) - 1;
		const int level = idx.internalId() % 100;
		const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
		NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

		if (level == 1 && idx.row() > 0)
		{
            if (      !(	l->getLegendType() == NMLayer::NM_LEGEND_RAMP
                        &&  idx.row() == NM_LEGEND_RAMP_ROW
                       )
                 &&   l->getLegendType() != NMLayer::NM_LEGEND_RGB
			   )
			{
				double rgba[4];
				l->getLegendColour(idx.row(), rgba);

				QString title = QString("Colour for '%1' of %2")
						.arg(l->getLegendName(idx.row()))
						.arg(l->objectName());

				QColor curclr;
				curclr.setRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);
                QColor clr = curclr;

                // ToDo: improve! alpha should work with all layers and
                //       legned options!
                if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
				{
                    clr = QColorDialog::getColor(curclr, this, title,
                            QColorDialog::ShowAlphaChannel);
				}
				else
				{
					clr = QColorDialog::getColor(curclr, this, title,
							QColorDialog::ShowAlphaChannel);
				}

                if (clr.isValid())
                {
                    rgba[0] = clr.redF();
                    rgba[1] = clr.greenF();
                    rgba[2] = clr.blueF();
                    rgba[3] = clr.alphaF();
                    l->setLegendColour(idx.row(), rgba);
                }
			}
			else
			{
				mDelegate->setLastMousePos(event->pos());
                this->setState(QAbstractItemView::NoState);
				this->edit(idx);
			}
		}
		else if (level == 2)
		{
			NMLayer* l = static_cast<NMLayer*>(idx.data(Qt::UserRole+100).value<void*>());
			QString itemstr = idx.data(Qt::DisplayRole).toString();
			NMDebugAI(<< "trying to edit '" << itemstr.toStdString() << "' of "
					<< l->objectName().toStdString() << std::endl);
            this->setState(QAbstractItemView::NoState);
			this->edit(idx);
		}
	}

	//NMDebugCtx(ctx, << "done!")
}

//bool ModelComponentList::eventFilter(QObject* obj, QEvent* event)
//{
//
//
//	return false;
//}

void ModelComponentList::mousePressEvent(QMouseEvent *event)
{
    //NMDebugCtx(ctx, << "...");

	int x = event->pos().x();
	int y = event->pos().y();

	QModelIndex idx = this->indexAt(event->pos());
	if (!idx.isValid())
	{
        //NMDebugCtx(ctx, << "done!")
		return;
	}
    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
    mLastLayer = l;

    const int level = idx.internalId() % 100;
    if (!idx.parent().isValid())
	{
        this->setCurrentIndex(idx);
		if (event->button() == Qt::LeftButton)
		{
			QRect vrect = visualRect(idx);
			int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
			QRect rect1 = QRect(
					header()->sectionViewportPosition(0) + itemIndentation,
					vrect.y(), 16, vrect.height());
			QRect rect2 = QRect(
					header()->sectionViewportPosition(0) + itemIndentation + 17,
					vrect.y(), 16, vrect.height());
			if (rect1.contains(event->pos()))
			{
				this->mLayerModel->setData(idx, QVariant("VIS"), Qt::CheckStateRole);
				this->processSelection(false);
			}
			else if  (rect2.contains(event->pos()))
			{
				this->mLayerModel->setData(idx, QVariant("SEL"), Qt::CheckStateRole);
                this->processSelection(false);
			}
			else
			{
				this->processSelection(true);
			}
			this->dragStartPosition = event->pos();

		}
		else if (event->button() == Qt::RightButton)
		{
            this->processSelection(false);
            if (l == 0)
            {
                NMMsg(<< "Please Select a Layer!");
                return;
            }
            // update the menu if we've got an image layer
            if (l != 0 && l->getLayerType() == NMLayer::NM_IMAGE_LAYER)
            {
                    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
                    if (il && il->getNumBands() == 1)
                    {
                        if (    mActValueStats->text().endsWith(QString("Value Field Statistics"))
                            ||  mActValueStats->text().endsWith(QString("Visible Pixel Statistics"))
                           )
                        {
                            if (l->getLegendValueField().compare(QString("Pixel Values")) == 0)
                            {
                                mActValueStats->setText(QString("Visible Pixel Statistics"));
                            }
                            else
                            {
                                mActValueStats->setText(QString("Value Field Statistics"));
                            }
                        }
                        mActImageStats->setEnabled(true);
                        mActImageStats->setVisible(true);

                        mActUniqueValues->setVisible(true);
                        mActSingleSymbol->setVisible(true);
                        mActClrTab->setVisible(true);
                        mActRGBImg->setVisible(false);

                        mActClrRamp->setText("Map Value Range");

                    }
                    else
                    {
                        mActImageStats->setEnabled(false);
                        mActImageStats->setVisible(false);

                        mActUniqueValues->setVisible(false);
                        mActSingleSymbol->setVisible(false);
                        mActClrTab->setVisible(false);
                        mActRGBImg->setVisible(true);

                        mActClrRamp->setText("Map Band Value Range");
                    }
                    mActImageInfo->setVisible(true);
                    mActStretchClrRamp->setEnabled(true);
                    mActStretchClrRamp->setVisible(true);

                    if (    il->hasSelBox()
                        &&  il->getSelection().count() > 0
                       )
                    {
                        mActZoomSel->setEnabled(true);
                    }
                    else
                    {
                        mActZoomSel->setEnabled(false);
                    }

                    // TODO: switch on when reliable
                    mActImageHistogram->setVisible(true);

                    mActVecContourOnly->setVisible(false);
                    mActOpacity->setVisible(true);
            }
            else
            {
                NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
                if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
                {
                    this->mActVecContourOnly->setVisible(true);
                }
                else
                {
                    this->mActVecContourOnly->setVisible(false);
                }

                mActRGBImg->setVisible(false);
                mActOpacity->setVisible(false);

                mActZoomSel->setVisible(false);
                mActZoomSel->setEnabled(false);

                mActStretchClrRamp->setEnabled(false);
                mActStretchClrRamp->setVisible(false);

                mActUniqueValues->setVisible(true);
                mActSingleSymbol->setVisible(true);
                mActClrTab->setVisible(true);
                mActClrRamp->setText("Map Value Range");

                mActImageInfo->setVisible(false);
                mActImageStats->setEnabled(false);
                mActImageStats->setVisible(false);
                mActImageHistogram->setVisible(false);
                mActValueStats->setText("Value Field Statistics");
            }

            if (l->getLegendType() == NMLayer::NM_LEGEND_RAMP)
            {
                mActExportColourRamp->setVisible(true);
            }
            else
            {
                mActExportColourRamp->setVisible(false);
            }

			this->mMenu->move(event->globalPos());
			this->mMenu->exec();
		}

    }
    else if (   event->button() == Qt::RightButton
             && level == 1
             && idx.row() > 0
             //&& this->getCurrentLayer() != 0
            )
    {
        if(l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
        {
            NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
            if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
            {
                this->mActVecContourOnly->setChecked(vl->getIsContoursOnlyOn());
                this->mContourMenu->move(event->globalPos());
                this->mContourMenu->exec();
            }
        }
    }

    //NMDebugCtx(ctx, << "done!");
}

void
ModelComponentList::editLayerOpacity()
{
    NMLayer* l = this->getCurrentLayer();
    if (l == 0)
        return;

    double curOpacity = l->getLayerOpacity();
    curOpacity *= 100;

    bool bok;
    double newOpacity = (double)QInputDialog::getInt(this,
                        tr("Set Layer Opacity"),
                        tr("Opacity [%]"), (int)curOpacity, 0, 100, 5, &bok);

    newOpacity /= 100.0;
    if (bok)
    {
        l->setLayerOpacity(newOpacity);
        NMGlobalHelper h;
        h.getVTKWidget()->update();
    }
}

void
ModelComponentList::editContourColour()
{
    NMLayer* l = mLastLayer;
    if (l == nullptr)
        return;

    NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
    if (   vl == 0
        || vl->getFeatureType() != NMVectorLayer::NM_POLYGON_FEAT
       )
        return;

    QString title = QString("Contour Colour for %1")
            .arg(l->objectName());

    QColor curclr = vl->getContourColour();
    QColor clr = QColorDialog::getColor(curclr, this, title);

    if (clr.isValid())
    {
        vl->setContourColour(clr);
    }

    NMGlobalHelper h;
    h.getVTKWidget()->update();
}

void
ModelComponentList::editSelectionColour()
{
    NMLayer* l = this->getCurrentLayer();
    if (l == nullptr)
    {
        return;
    }

    QString title = QString("Selection Colour for %1")
            .arg(l->objectName());

    QColor curclr = l->getSelectionColor();
    QColor clr = QColorDialog::getColor(curclr, this, title,
                                        QColorDialog::ShowAlphaChannel);

    if (clr.isValid())
    {
        l->setSelectionColor(clr);
    }

    NMGlobalHelper h;
    h.getVTKWidget()->update();

}

void
ModelComponentList::editContourWidth()
{
    NMLayer* l = mLastLayer;
    if (l == nullptr)
        return;

    NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
    if (   vl == 0
        || vl->getFeatureType() != NMVectorLayer::NM_POLYGON_FEAT
       )
        return;

    vtkActor* a = const_cast<vtkActor*>(vl->getContourActor());
    if (a == 0)
        return;

    double curWidth = a->GetProperty()->GetLineWidth();

    bool bok;
    double newWidth = QInputDialog::getDouble(this,
                        tr("Set Contour Width"),
                        "Width in Pixel", curWidth, 1, 8, 1, &bok);
    if (bok)
    {
        a->GetProperty()->SetLineWidth(static_cast<float>(newWidth));

        NMGlobalHelper h;
        h.getVTKWidget()->update();
    }

}

void
ModelComponentList::editContourStyle()
{
    NMLayer* l = mLastLayer;
    if (l == nullptr)
        return;

    NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
    if (   vl == 0
        || vl->getFeatureType() != NMVectorLayer::NM_POLYGON_FEAT
       )
        return;

    vtkActor* a = const_cast<vtkActor*>(vl->getContourActor());
    if (a == 0)
        return;

    QStringList options;
    options << "Solid" << "Dashed" << "Dotted";

    int curPattern = a->GetProperty()->GetLineStipplePattern();
    int curIdx = 0;
    if (curPattern == 0xffff) curIdx = 0;
    else if (curPattern == 0xf0f0) curIdx = 1;
    else curIdx = 2;

    bool bok;
    QString style = QInputDialog::getItem(this,
                   tr("Select Contour Style"), "",
                   options, curIdx, false, &bok);

    if (bok)
    {
        if (style == "Solid")
        {
            a->GetProperty()->SetLineStipplePattern(0xffff);
        }
        else if (style == "Dashed")
        {
            a->GetProperty()->SetLineStipplePattern(0xf0f0);
        }
        else if (style == "Dotted")
        {
            a->GetProperty()->SetLineStipplePattern(0xf00);
        }

        NMGlobalHelper h;
        h.getVTKWidget()->update();
    }
}


void
ModelComponentList::mapVectorContoursOnly()
{
    NMLayer* l = mLastLayer;
    if (l == nullptr)
        return;

    NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(l);
    if (vl == 0)
        return;

    vl->setFeaturesVisible(!this->mActVecContourOnly->isChecked());
    vtkAbstractMapper* mapper = const_cast<vtkAbstractMapper*>(vl->getMapper());
    mapper->Update();

    NMGlobalHelper h;
    h.getVTKWidget()->update();
}

void ModelComponentList::mapRGBImage()
{
    NMLayer* l = this->getCurrentLayer();
    if (l == 0)
        return;

    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    il->setLegendValueField(QString("RGB"));
    il->setLegendDescrField(QString("Band Number"));
    il->setLegendType(NMLayer::NM_LEGEND_RGB);

    il->updateMapping();
}

void ModelComponentList::processSelection(bool toggle)
{
	QModelIndex idx = this->currentIndex();

    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

    bool bselect = toggle ? !l->getIsSelected() : l->getIsSelected();
    if (bselect)
	{
		this->selectionModel()->clearSelection();
		this->selectionModel()->select(idx, QItemSelectionModel::Select |
			QItemSelectionModel::Rows);

        l->setIsSelected(true);
		emit selectedLayerChanged(l);
	}
    else
	{
        l->setIsSelected(false);
		this->selectionModel()->select(idx, QItemSelectionModel::Deselect |
			QItemSelectionModel::Rows);
	}
	this->update();
    NMGlobalHelper::getMainWindow()->checkInteractiveLayer();
}

void ModelComponentList::mouseMoveEvent(QMouseEvent *event)
{
	//NMDebugCtx(ctx, << "...");

    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - dragStartPosition).manhattanLength()
         < QApplication::startDragDistance())
        return;

    QModelIndex idx = this->indexAt(dragStartPosition);
    if (!idx.isValid())
    	return;

    const int tlr = (idx.internalId() / 100) - 1;
    const int sp = this->mLayerModel->toLayerStackIndex(tlr);
    NMLayer* l = this->mLayerModel->getItemLayer(sp);
    if (l == 0)
    	return;

    QString layerName = l->objectName();//idx.data(Qt::DisplayRole).toString();
    //NMLayer* l = (NMLayer*)idx.internalPointer();
    //NMLayer* l = this->getLayer(layerName);

    QIcon layerIcon = l->getLayerIcon();//idx.data(Qt::DecorationRole).value<QIcon>();

    QSize dragImageSize(32,32);
    QImage dragImage(dragImageSize, QImage::Format_ARGB32_Premultiplied);

    QPainter dragPainter(&dragImage);
    dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
    dragPainter.fillRect(dragImage.rect(), Qt::transparent);
    dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    dragPainter.drawPixmap(0,0,32,32, layerIcon.pixmap(dragImageSize));
    dragPainter.end();

    QDrag* drag = new QDrag(this);
    drag->setPixmap(QPixmap::fromImage(dragImage));
    drag->setDragCursor(QPixmap(":move-icon.png"), Qt::CopyAction);


	QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString::fromLatin1("_ModelComponentList_:%1").arg(layerName).toStdString().c_str());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction, Qt::CopyAction);

    //NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::dropEvent(QDropEvent* event)
{
    NMDebugCtx(ctx, << "...");

    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_NMModelScene_")) == 0)
    {
        NMDebugAI(<< "adding layer: " << dropLayer.toStdString() << std::endl);
        NMDataComponent* comp = qobject_cast<NMDataComponent*>(
                    NMGlobalHelper::getModelController()->getComponent(dropLayer));

        if (comp == 0
            || comp->getOutput(0).isNull())
        {
            NMLogInfo(<< ctx << ": Cannot display NULL object!")
            NMDebugCtx(ctx, << "done!");
            return;
        }

        // =========================================================
        //              DROPPED LAYER (w || w/o RAT)
        // =========================================================
        NMGlobalHelper h;
        if (comp->getOutput(0)->getDataObject() != 0)
        {
            QString layerName = dropLayer;
            if (!comp->getDescription().isEmpty())
            {
                layerName = comp->getDescription();
            }


            vtkRenderWindow* renWin = h.getRenderWindow();
            NMImageLayer* iLayer = new NMImageLayer(renWin, 0, this);
            iLayer->setObjectName(layerName);
            iLayer->setLogger(h.getMainWindow()->getLogger());
            h.getMainWindow()->connectImageLayerProcSignals(iLayer);

            iLayer->setImage(comp->getOutput(0));

            connect(comp, SIGNAL(NMDataComponentChanged()), iLayer, SLOT(updateSourceBuffer()));
        }
        // =========================================================
        //              DROPPED STAND ALONE TABLE
        // =========================================================
        else if (comp->getOutput(0)->getOTBTab().IsNotNull())
        {
            otb::SQLiteTable::Pointer sqltab = static_cast<otb::SQLiteTable*>(comp->getOutput(0)->getOTBTab().GetPointer());

            if (sqltab && !sqltab->GetDbFileName().empty())
            {
                h.getMainWindow()->createTableView(sqltab);
            }
        }
    }
    else if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        QModelIndex destidx = this->indexAt(event->pos());
        if (!destidx.isValid() || destidx.parent().isValid())
        {
            this->mIndicatorIdx = QModelIndex();

            NMDebugAI(<< "no valid drop pos!" << endl);
            NMDebugCtx(ctx, << "done!");
            return;
        }

        const int stackpos = this->mLayerModel->toLayerStackIndex(destidx.row());
        //NMLayer* dl = (NMLayer*)destidx.internalPointer();
        NMLayer* dl = this->mLayerModel->getItemLayer(stackpos);
        if (dl == 0)
            return;
        int destpos = dl->getLayerPos();
        //NMDebugAI(<< "dest pos: " << destpos << endl);

        QModelIndex srcidx = this->indexAt(this->dragStartPosition);
        const int srcstackpos = this->mLayerModel->toLayerStackIndex(srcidx.row());
        //NMLayer* sl = (NMLayer*)srcidx.internalPointer();
        NMLayer* sl = this->mLayerModel->getItemLayer(srcstackpos);
        if (sl == 0)
            return;
        int srcpos = sl->getLayerPos();
        //NMDebugAI(<< "src pos: " << srcpos << endl);

        this->changeLayerPos(srcpos, destpos);

        // stop drawing the indicator
        this->mIndicatorIdx = QModelIndex();

        //event->acceptProposedAction();
    }
    else if (event->mimeData()->hasUrls())
    {
        QString fileName;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                fileName = url.toLocalFile();
                break;
            }
        }

        if (!fileName.isEmpty())
        {
            QFileInfo finfo(fileName);

            QStringList tabFormats;
            tabFormats << "dbf" << "db" << "sqlite" << "ldb" << "csv" << "txt" << "xls" << "shp" << "shx";
            QStringList imgFormats;
            imgFormats << "kea" << "img" << "tiff" << "jpg" << "jpeg" << "tif"
                       << "png" << "gif" << "adf" << "hdr" << "sdat" << "vrt";
            QString ext = finfo.suffix().toLower();
            if (tabFormats.contains(ext))// || fileName.compare(QString::fromLatin1("file::memory:")) == 0)
            {
                NMGlobalHelper hlp;
                LUMASSMainWin* mainWin = hlp.getMainWindow();

                QStringList sqliteformats;
                sqliteformats << "db" << "sqlite" << "ldb";

                QString tableName;
                if (sqliteformats.contains(ext))// || fileName.compare(QString::fromLatin1("file::memory:")) == 0)
                {
                    tableName = mainWin->selectSqliteTable(fileName);
                    if (!tableName.isEmpty())
                    {
                        mainWin->importTable(fileName, LUMASSMainWin::NM_TABVIEW_STANDALONE, true, tableName);
                    }
                }
                else
                {
                    mainWin->importTable(fileName, LUMASSMainWin::NM_TABVIEW_STANDALONE, true);
                }
            }
            else if (imgFormats.contains(ext))
            {
                NMGlobalHelper h;
                vtkRenderWindow* renWin = h.getRenderWindow();
                NMImageLayer* fLayer = new NMImageLayer(renWin, 0, this);
                fLayer->setObjectName(finfo.baseName());
                fLayer->setLogger(h.getMainWindow()->getLogger());
                h.getMainWindow()->connectImageLayerProcSignals(fLayer);
                fLayer->setFileName(fileName);
            }
            else if (ext.compare(QString("vtk")) == 0)
            {
                vtkSmartPointer<vtkPolyData> pd;
                vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
                reader->SetFileName(fileName.toStdString().c_str());
                reader->Update();
                pd = reader->GetOutput();

                NMGlobalHelper h;
                vtkRenderWindow* renWin = h.getRenderWindow();
                NMVectorLayer* vl = new NMVectorLayer(renWin);
                vl->setLogger(h.getMainWindow()->getLogger());
                vl->setFileName(fileName);
                vl->setObjectName(finfo.baseName());
                vl->setDataSet(pd);
                vl->setVisible(true);

                h.getMainWindow()->addLayerToCompList(vl);

            }
            //QtConcurrent::run(fLayer, &NMImageLayer::setFileName, fileName);
        }
    }
    //event->setAccepted(true);
    NMDebugCtx(ctx, << "done!");

}


void ModelComponentList::dragEnterEvent(QDragEnterEvent* event)
{
    NMDebugCtx(ctx, << "...");

    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        if (this->getLayer(dropLayer) != 0)
        {
            event->acceptProposedAction();
        }
    }
    else if (dropSource.startsWith(QString::fromLatin1("_NMModelScene_")))
    {
        NMModelComponent* comp = NMGlobalHelper::getModelController()->getComponent(dropLayer);
        if (comp != 0)
        {
            QSharedPointer<NMItkDataObjectWrapper> dw = comp->getOutput(0);
            if (dw.isNull() || dw->getDataObject() == 0)
            {
                NMDebugAI(<< "empty data object!" << std::endl);
            }
            else
            {
                NMDebugAI(<< "got data on the hook!" << std::endl);
            }
            event->acceptProposedAction();
        }
    }
    else if (event->mimeData()->hasUrls())
    {
        QString fileName;
        foreach(const QUrl& url, event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                fileName = url.toLocalFile();
                break;
            }
        }

        if (!fileName.isEmpty())
        {
            event->acceptProposedAction();
        }
    }

    NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::dragMoveEvent(QDragMoveEvent* event)
{
    QString dropSource;
    QString dropLayer;
    if (event->mimeData()->hasFormat("text/plain"))
    {
        QString ts = event->mimeData()->text();
        QStringList tl = ts.split(':', QString::SkipEmptyParts);
        if (tl.count() == 2)
        {
            dropSource = tl.at(0);
            dropLayer = tl.at(1);
        }
    }

    if (dropSource.compare(QString::fromLatin1("_ModelComponentList_")) == 0)
    {
        QModelIndex id = this->indexAt(event->pos());
        if (!id.isValid() || id.parent().isValid())
        {
            this->mIndicatorIdx = QModelIndex();
            return;
        }
        this->mIndicatorIdx = id;
        this->viewport()->update();
        event->acceptProposedAction();
    }
    else
    {
        if (    dropSource.startsWith(QString::fromLatin1("_NMModelScene_"))
            ||  event->mimeData()->hasUrls()
            )
        {
            event->acceptProposedAction();
        }
    }
}

void ModelComponentList::mouseReleaseEvent(QMouseEvent* event)
{

}

void ModelComponentList::paintEvent(QPaintEvent* event)
{
	// do all normal painting first
	QTreeView::paintEvent(event);

	// add an indicator rectangle, if applicable
	if (this->mIndicatorIdx.isValid())
	{
		QPainter painter(this->viewport());

		QString text = this->mLayerModel->data(this->mIndicatorIdx, Qt::DisplayRole).toString();
		QFont font = this->mLayerModel->data(this->mIndicatorIdx, Qt::FontRole).value<QFont>();

		QRect vrect = visualRect(this->mIndicatorIdx);
		int itemIndentation = vrect.x() - visualRect(rootIndex()).x();
		int checkboxwidth = style()->pixelMetric(QStyle::PM_IndicatorWidth);

		painter.setFont(font);
		QFontMetrics fm = painter.fontMetrics();
		int textwidth = fm.width(text);
		int iconwidth = vrect.width() - itemIndentation - textwidth - checkboxwidth;

		QRect rect = QRect(
				header()->sectionViewportPosition(0) + itemIndentation,
				vrect.y(),
				vrect.width(),// - itemIndentation,
				//checkboxwidth + iconwidth + textwidth,
				vrect.height());

		QImage img(rect.size(), QImage::Format_ARGB32_Premultiplied);
		img.fill(0);
	    QPainter dragPainter(&img);

	    QLinearGradient bw(0,0,0,img.rect().height()*0.5);
	    bw.setSpread(QGradient::ReflectSpread);
	    bw.setColorAt(0, QColor(0,0,200));
	    bw.setColorAt(0.8, QColor(255,255,255));
	    bw.setColorAt(1, QColor(255,255,255));
	    dragPainter.fillRect(img.rect(), bw);
	    dragPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);

	    QLinearGradient fade(0,0,0,img.rect().height()*0.5);
	    fade.setSpread(QGradient::ReflectSpread);
	    fade.setColorAt(0, QColor(0,0,0,127));
	    fade.setColorAt(1, QColor(0,0,0,0));
	    dragPainter.fillRect(img.rect(), fade);

		//painter.setPen(Qt::red);
		//painter.drawRect(rect);
		//painter.setCompositionMode(QPainter::CompositionMode_Overlay);
		//painter.fillRect(rect, QBrush(QColor(255,0,0,80)));
		//painter.setOpacity(0.2);

	    painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(QPen(QBrush(QColor(0,0,200,127)), 3));
		painter.drawImage(rect, img);
		painter.drawRoundedRect(rect, 5,5);
	}
}

void ModelComponentList::zoomToLayer()
{
    // mode 0
    this->zoom(0);
}

void ModelComponentList::zoomToSelection()
{
    // mode 1
    this->zoom(1);
}

void ModelComponentList::zoom(int mode)
{
    // get the current layer
    QModelIndex idx = this->currentIndex();

    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->mLayerModel->getItemLayer(stackpos);

    double* box;
    if (mode == 0)
    {
        box = const_cast<double*>(l->getBBox());
    }
    else
    {
        box = const_cast<double*>(l->getSelectionBBox());
    }

    // get the camera of the background renderer
    LUMASSMainWin* win = qobject_cast<LUMASSMainWin*>(this->topLevelWidget());
    vtkRenderer* bkgRen = const_cast<vtkRenderer*>(win->getBkgRenderer());
    bkgRen->ResetCamera(box);
    //win->findChild<QVTKOpenGLWidget*>(tr("qvtkWidget"))->update();
    NMGlobalHelper::getRenderWindow()->Render();
}

void ModelComponentList::mapSingleSymbol()
{
	// get the current layer
    //	QModelIndex idx = this->currentIndex();
    //	const int toplevelrow = (idx.internalId() / 100) - 1;
    //	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->getCurrentLayer();//this->mLayerModel->getItemLayer(stackpos);
    if (l == 0)
        return;

	l->setLegendType(NMLayer::NM_LEGEND_SINGLESYMBOL);
	l->updateMapping();
}

void ModelComponentList::mapColourTable()
{
    //	// get the current layer
    //	QModelIndex idx = this->currentIndex();
    //	const int toplevelrow = (idx.internalId() / 100) - 1;
    //	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->getCurrentLayer();//this->mLayerModel->getItemLayer(stackpos);
    if (l == 0)
        return;

	if (l->getTable() == 0)
	{
		NMDebugAI(<< "cannot map the colour table, since we don't even have an attribute table!" << std::endl);
		return;
	}

	if (!l->hasColourTable())
	{
		NMDebugAI(<< "cannot map the colour table, since we didn't find any colour attributes!" << std::endl);
		return;
	}

	l->setLegendType(NMLayer::NM_LEGEND_CLRTAB);
	l->updateMapping();


}

NMLayer* ModelComponentList::getCurrentLayer()
{
    // get the current layer
    QModelIndex idx = this->currentIndex();
    if (!idx.isValid())
    {
        return 0;
    }
    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    return this->mLayerModel->getItemLayer(stackpos);
}


void ModelComponentList::mapColourRamp()
{
	// get the current layer
    NMLayer* l = this->getCurrentLayer();
    if (l == 0)
    {
        return;
    }

	l->setLegendType(NMLayer::NM_LEGEND_RAMP);

    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (il && il->getNumBands() == 3)
    {
        const int nband = il->getScalarBand();

        QString sband = QString("Band #%1").arg(nband);
        il->setLegendValueField(sband);
        il->setLegendDescrField(sband);

        std::vector<int> bandmap;
        for (int b=0; b < 3; ++b)
        {
            bandmap.push_back(nband);
        }
        il->setBandMap(bandmap);
    }
    l->updateMapping();

}

void ModelComponentList::mapUniqueValues()
{
	// get the current layer
    //	QModelIndex idx = this->currentIndex();
    //	const int toplevelrow = (idx.internalId() / 100) - 1;
    //	const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->getCurrentLayer();//this->mLayerModel->getItemLayer(stackpos);
    if (l == 0)
        return;

	NMVectorLayer* vL = 0;
	NMImageLayer* iL = 0;
	vtkDataSet* ds = 0;


	QStringList sFields;
	if (l->getLayerType() == NMLayer::NM_VECTOR_LAYER)
	{
		vL = qobject_cast<NMVectorLayer*>(l);
		// ToDo:: this probably works for any feature type -> check!
		if (vL->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
			ds = const_cast<vtkDataSet*>(vL->getDataSet());

		vtkDataSetAttributes* dsAttr = 0;
		if (ds != 0)
			dsAttr = ds->GetAttributes(vtkDataSet::CELL);
		else
			return;

		int numFields = dsAttr->GetNumberOfArrays();

		for (int f=0; f < numFields; f++)
		{
			vtkAbstractArray* aa = dsAttr->GetAbstractArray(f);
			if (strcmp(aa->GetName(), "nm_hole") == 0 ||
				strcmp(aa->GetName(), "nm_sel") == 0)
				continue;

			if (aa->GetDataType() == VTK_DOUBLE ||
				aa->GetDataType() == VTK_FLOAT)
				continue;

			sFields.append(QString(dsAttr->GetArrayName(f)));
		}
	}
	else if (l->getLayerType() == NMLayer::NM_IMAGE_LAYER)
	{
		iL = qobject_cast<NMImageLayer*>(l);

        if (iL->getTable() == 0)
        {
            return;
        }

        /// ToDo: need to test where the performance limit really is to ask

        //        else if (iL->getNumTableRecords() > 25000)
        //        {
        //            QMessageBox::StandardButton resp = QMessageBox::warning(
        //                                 this, "Map Unique Values",
        //                                 "Mapping of more than 25000 unique values may "
        //                                 "impact performance! Do you want to continue?",
        //                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        //            if (resp == QMessageBox::No)
        //            {
        //                return;
        //            }
        //        }

        sFields.append(iL->getNumericColumns(true));
        sFields.append(iL->getStringColumns());
	}
	else
	{
		return;
	}

	bool bOk = false;
	QString theField = QInputDialog::getItem(this,
			tr("Map Unique Values"), tr("Choose an attribute to map"),
			sFields, 0, false, &bOk, 0);
	// if the user pressed cancel
	if (!bOk)
		return;

	l->setLegendType(NMLayer::NM_LEGEND_INDEXED);
	l->setLegendClassType(NMLayer::NM_CLASS_UNIQUE);
	l->setLegendValueField(theField);
}

void ModelComponentList::test()
{
	NMDebugCtx(ctx, << "...");


	NMDebugCtx(ctx, << "done!");
}

void ModelComponentList::stretchColourRampToVisMinMax()
{
    // get the current layer
    QModelIndex idx = this->currentIndex();

    const int toplevelrow = (idx.internalId() / 100) - 1;
    const int stackpos = this->mLayerModel->toLayerStackIndex(toplevelrow);
    NMLayer* l = this->mLayerModel->getItemLayer(stackpos);
    if (l == nullptr)
    {
        return;
    }

    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (il != nullptr)
    {
        std::vector<double> stats = il->getWindowStatistics();
        QString col = il->getLegendValueField();
        NMSqlTableModel* sqlModel = qobject_cast<NMSqlTableModel*>(
                    const_cast<QAbstractItemModel*>(il->getTable()));

        if (sqlModel != nullptr && col.compare("Pixel Values") != 0)
        {
            std::string key = sqlModel->getNMPrimaryKey().toStdString();

            QSqlDriver* drv = sqlModel->database().driver();
            std::string colstr = drv->escapeIdentifier(col, QSqlDriver::FieldName).toStdString();
            std::string tabstr = drv->escapeIdentifier(sqlModel->tableName(), QSqlDriver::TableName).toStdString();
            std::stringstream sql;

            sql << "select "
                << "min(" << colstr << ") as minimum, "
                << "max(" << colstr << ") as maximum "
                << "from " << tabstr
                << " where " <<  key
                << " between " << stats[0] << " and " << stats[1];

            sqlModel->database().transaction();
            QSqlQuery q(sqlModel->database());
            if (q.exec(sql.str().c_str()) && q.next())
            {
                stats[0] = q.value(1).toDouble();
                stats[1] = q.value(2).toDouble();
            }
            q.finish();
            q.clear();
            sqlModel->database().commit();
        }

        il->setLower(stats.at(0));
        il->setUpper(stats.at(1));
        il->updateMapping();
    }
}

void ModelComponentList::wholeImgStats()
{
    mbWholeImgStats = true;
    this->showValueStats();
}

void ModelComponentList::showWholeImgStats()
{
    QFutureWatcher<std::vector<double > >* watcher =
            static_cast<QFutureWatcher<std::vector<double> >* >(this->sender());

    // can this actually happen?
    if (!mStatsWatcherMap.contains(watcher))
    {
        NMLogDebug(<< ctx << ": Got an unobserved future watcher for img stats!");
        delete watcher;
        return;
    }

    //    FutureStatsType past = watcher->future();
    //    StatsType stats = past.result();

    QFuture<std::vector<double> > past = watcher->future();
    std::vector<double> stats = past.result();

    QString layerName = *mStatsWatcherMap.find(watcher);

    QString res = this->formatStats(stats);
    QString title = QString(tr("%1 - %2")).arg(layerName)
            .arg("Whole Image Pixel Statistics");

    QMessageBox::information(this, title, res);

}

void ModelComponentList::showValueStats()
{
    //NMDebugCtx(ctx, << "...");

    NMLayer* l = this->getCurrentLayer();
	if (l == 0)
		return;

    if (mbWholeImgStats)
    {
        mbWholeImgStats = false;
        NMImageLayer* il = qobject_cast<NMImageLayer*>(l);

        QFutureWatcher<std::vector<double> >* watcher =
                new QFutureWatcher<std::vector<double> >();
        mStatsWatcherMap.insert(watcher, il->objectName());
        connect(watcher, SIGNAL(finished()), this, SLOT(showWholeImgStats()));

        QFuture<std::vector<double> > res = QtConcurrent::run(
                    il, &NMImageLayer::getWholeImageStatistics);

        watcher->setFuture(res); //const_cast<QFuture<QList<double> >& >(res));
        return;
    }


    std::vector<double> stats = l->getValueFieldStatistics();
	if (stats.size() == 0)
	{
        NMLogError(<< ctx << ": failed retrieving value field statistics!");
		NMDebugCtx(ctx, << "done!");
		return;
	}

    //	NMDebugAI(<< "what we've got: ..." << std::endl);
    //	for (int r=0; r < stats.size(); ++r)
    //	{
    //		NMDebugAI(<< "#" << r << ": " << stats[r] << std::endl);
    //	}

    QString res = this->formatStats(stats);
    QString title = QString(tr("%1::%2")).arg(l->objectName())
            .arg(l->getLegendValueField());

	QMessageBox::information(this, title, res);

    //NMDebugCtx(ctx, << "done!");
}

QString ModelComponentList::formatStats(const std::vector<double> &stats)
{
    QString res;

    if (stats.size() == 7)
    {
        // min, max, mean, std. dev, sum
        QString smin  = QString("%1").arg(stats[0], 0, 'f', 4); //smin  = smin .rightJustified(15, ' ');
        QString smax  = QString("%1").arg(stats[1], 0, 'f', 4); //smax  = smax .rightJustified(15, ' ');
        QString smean = QString("%1").arg(stats[2], 0, 'f', 4); //smean = smean.rightJustified(15, ' ');
        QString smedi = QString("%1").arg(stats[3], 0, 'f', 4);
        QString ssdev = QString("%1").arg(stats[4], 0, 'f', 4); //ssdev = ssdev.rightJustified(15, ' ');
        QString ssum  = QString("%1").arg(stats[6], 0, 'f', 4); //ssum  = ssum .rightJustified(15, ' ');
        QString ssample = QString("Sample Size: %1").arg(stats[5]);

        QString strmin  ("Minimum: ");  //strmin  = strmin .rightJustified(10, ' ');
        QString strmax  ("Maximum: ");  //strmax  = strmax .rightJustified(10, ' ');
        QString strmean ("Mean: ");     //strmean = strmean.rightJustified(10, ' ');
        QString strmedi ("Median: ");
        QString strsdev ("Std.Dev.: "); //strsdev = strsdev.rightJustified(10, ' ');
        QString strsum  ("Sum: ");      //strsum  = strsum .rightJustified(10, ' ');


        res = QString("%1%2\n%3%4\n%5%6\n%7%8\n%9%10\n%11%12\n%13")
                .arg(strmin).arg(smin)
                .arg(strmax).arg(smax)
                .arg(strmean).arg(smean)
                .arg(strmedi).arg(smedi)
                .arg(strsdev).arg(ssdev)
                .arg(strsum).arg(ssum)
                .arg(ssample);
    }
    else
    {
        res = "Failed Calculating Statistics!";
    }

    return res;
}
