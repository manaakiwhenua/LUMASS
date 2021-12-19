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
#ifndef MODELCOMPONENTLIST_H
#define MODELCOMPONENTLIST_H

//#define ctxModelComponentList "ModelComponentList"

#include <vector>

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeView>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QChildEvent>
#include <QActionEvent>
#include <QPoint>
#include <QMap>
#include <QSharedPointer>
#include <QDrag>
#include <QPaintEvent>
#include <QFutureWatcher>
#include <QFuture>

#include "NMLayerModel.h"
#include "NMComponentListItemDelegate.h"

#include "vtkSmartPointer.h"
#include "vtkEventQtSlotConnect.h"

class LUMASSMainWin;
class NMLayer;
class NMLogger;

class ModelComponentList : public QTreeView
{
    Q_OBJECT

friend class LUMASSMainWin;

public:
    ModelComponentList(QWidget *parent);
    ~ModelComponentList();

    void addLayer(NMLayer* layer);
    bool removeLayer(QString layerName);

    int getLayerCount(void) {
        return this->mLayerModel->getItemLayerCount();}

    NMLayer* getLayer(QString layerName) {
        return this->mLayerModel->getItemLayer(layerName);}

    NMLayer* getLayer(int idx) {
        return this->mLayerModel->getItemLayer(idx);}

    // gets the topmost selected layer
    NMLayer* getSelectedLayer(void);

    // MapBBox is the union of all layer's bounding boxes
    // -> map extent (in coordinate units)
    const double* getMapBBox(void);

    // moves layer from to oldpos to newpos position
    // in the layer stack
    // returns the layers old position
    int changeLayerPos(int oldpos, int newpos);

public: signals:
    void selectedLayerChanged(const NMLayer* layer);

public slots:
    void zoomToLayer();
    void zoomToSelection();
    void updateMapWin(const NMLayer* layer);
    void updateLegend(const NMLayer* layer);
    void mapUniqueValues();
    void mapSingleSymbol();
    void saveLayerChanges();
    void mapColourTable();
    void mapColourRamp();
    void mapRGBImage();
    void showValueStats();
    void mapVectorContoursOnly();
    void editContourColour();
    void editContourWidth();
    void editContourStyle();
    void editLayerOpacity();
    void editSelectionColour();
    void saveLegend();
    void loadLegend();
    void showImageInfo();
    void exportColourRamp();
    void toggleClrRampInversion(bool tog);
    void toggleClrRampScale(bool tog);

    void test();

private slots:
    void openAttributeTable();
    void removeCurrentLayer();

protected:
    void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const {}

protected slots:
    void showWholeImgStats();
    void wholeImgStats();
    void showImageHistogram();
    /*!
     *  mode=0 : zoom to layer
     *  mode=1 : zoom to selection
     */
    void zoom(int mode);

    void stretchColourRampToVisMinMax();
    void importMultiDataSet(const QString& fn);

private:

    double mFullMapExt[6];

    NMLogger* mLogger;

    NMLayer* mLastLayer;
    NMLayerModel* mLayerModel;
    NMComponentListItemDelegate* mDelegate;
    QPoint dragStartPosition;
    QMenu* mMenu;
    QMenu* mContourMenu;
    QMenu* mRampMenu;
    QModelIndex mIndicatorIdx;
    static const std::string ctx;


    QAction* mActUniqueValues;
    QAction* mActSingleSymbol;
    QAction* mActClrTab;
    QAction* mActClrRamp;
    QAction* mActRGBImg;
    QAction* mActVecContourOnly;
    QAction* mActOpacity;

    QAction* mActStretchClrRamp;

    QAction* mActImageHistogram;
    QAction* mActValueStats;
    QAction* mActImageStats;
    QAction* mActImageInfo;
    QAction* mActExportColourRamp;
    QAction* mActZoomSel;
    bool mbWholeImgStats;

    QAction* mActRampInvert;
    QAction* mActRampLogScale;


    // keeps track of future watchers being created for stats creation survaillance
    // and keeps them thereby in scope
    QMap<QFutureWatcher<std::vector<double> >*, QString> mStatsWatcherMap;

    vtkSmartPointer<vtkEventQtSlotConnect> mVTKConn;

    void initView(void);
    void processSelection(bool toggle);
    void removeLayer(NMLayer* layer);
    void unionMapBBox(const double* box);
    void recalcMapBBox();
    NMLayer* getCurrentLayer(void);

    QString formatStats(const std::vector<double>& stats);

    // event handlers
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void paintEvent(QPaintEvent* event);
};

#endif // MODELCOMPONENTLIST_H
