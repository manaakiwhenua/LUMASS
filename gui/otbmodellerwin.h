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
#ifndef OTBMODELLERWIN_H
#define OTBMODELLERWIN_H

#define ctxOtbModellerWin "OtbModellerWin"

#include <QtGui/QMainWindow>
#include <QMouseEvent>
#include <QActionEvent>
#include <QChildEvent>
#include <QMetaType>
#include <QMap>
#include <QLabel>
#include <QSharedPointer>

// OGR
#include "ogrsf_frmts.h"

//#include "otbGDALRATImageFileReader.h"
//#include "otbImage.h"
#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkSmartPointer.h"
#include "vtkCellPicker.h"

#include "LUMASSConfig.h"
#include "NMLayer.h"


// rasdaman
#ifdef BUILD_RASSUPPORT
  #include "RasdamanConnector.h"
  #include "otbRasdamanImageReader.h"
  #include "otbRasdamanImageIO.h"
  #include "otbRasdamanImageIOFactory.h"
#endif


class vtkRenderer;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;

namespace Ui
{
    class OtbModellerWin;
}

class OtbModellerWin : public QMainWindow
{
    Q_OBJECT


public:
    OtbModellerWin(QWidget *parent = 0);
    ~OtbModellerWin();

    const vtkRenderer* getBkgRenderer(void);
    void displayChart(vtkTable* srcTab);
    void updateCoordLabel(const QString& newCoords);
#ifdef BUILD_RASSUPPORT    
    RasdamanConnector* getRasdamanConnector(void);
#endif
public slots:
	void loadImageLayer();
	void import3DPointSet();			// imports char (" " | "," | ";" | "\t") seperated text (x,y,z)
	void toggle3DStereoMode();
	void toggle3DSimpleMode();
	void loadVTKPolyData();			// loads VTK *vtp PolyData
	void loadVectorLayer();
	void doMOSO();
	void showComponentsView();
	void showModelView();
	void updateCoords(vtkObject* obj);
	void removeAllObjects();
	void pickObject(vtkObject* obj);
	void zoomFullExtent();
	void saveAsVtkPolyData();
	void test();
	void saveAsVectorLayerOGR();
	void updateLayerInfo(NMLayer* l, long cellId);
	void importODBC();
	void aboutLUMASS();

	virtual bool notify(QObject* receiver, QEvent* event);

protected:
//	void displayPolyData(vtkSmartPointer<vtkPolyData> polydata, double* lowPt, double* highPt);
	vtkSmartPointer<vtkPolyData> OgrToVtkPolyData(OGRDataSource& ds);

	// those conversion functions are dealing as well with the particular "Multi-"
	// case (i.e. MultiPoint, etc.)
	vtkSmartPointer<vtkPolyData> wkbPointToPolyData(OGRLayer& l);
	vtkSmartPointer<vtkPolyData> wkbLineStringToPolyData(OGRLayer& l);
	vtkSmartPointer<vtkPolyData> wkbPolygonToPolyData(OGRLayer& l);

private:

	// holds the current 3D state (i.e. is interaction
	// in all 3 dims allowed or restricted to 2D)
    bool m_b3D;

    // the GUI containing all controls of the main window
	Ui::OtbModellerWin *ui;
	// for showing the mouse position in real world coordinates
    QLabel* m_coordLabel;
    // for showing pixel values
    QLabel* mPixelValLabel;

    // for showing random messages in the status bar
    QLabel* m_StateMsg;

    double mLastPick[3];

    // the background renderer (layer 0)
    vtkSmartPointer<vtkRenderer> mBkgRenderer;
    // connection between vtk and qt event mechanism
    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkConns;
    // orientation marker
    vtkSmartPointer<vtkOrientationMarkerWidget> m_orientwidget;

    /* testing whether pt lies in the cell (2d case)
     * uses ray-casting odd-even rule: i.e. when pt is
     * in the cell, a ray going  through this point
     * intersects with the polygon an odd number of times;
     * and when the point lies outside the polygon, it
     * intersects an even number of times
     * */
    bool ptInPoly2D(double pt[3], vtkCell* cell);
#ifdef BUILD_RASSUPPORT
    RasdamanConnector *mpRasconn;
#endif
};


#endif // OTBMODELLERWIN_H
