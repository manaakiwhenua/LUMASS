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
 * NMVectorLayer.h
 *
 *  Created on: 11/03/2011
 *      Author: alex
 */

#ifndef NMVECTORLAYER_H_
#define NMVECTORLAYER_H_
#define ctxNMVectorLayer "NMVectorLayer"

#include "NMLayer.h"

#include "vtkPolyData.h"
#include "vtkActor.h"

class NMVectorLayer: public NMLayer
{
	Q_OBJECT
	Q_ENUMS(NMFeatureType)

public:
	NMVectorLayer(vtkRenderWindow* renWin,
			vtkRenderer* renderer=0,
			QObject* parent=0);
	virtual ~NMVectorLayer();

	enum NMFeatureType {NM_UNKNOWN_FEATURE, NM_POINT_FEAT, NM_POLYLINE_FEAT,
		NM_POLYGON_FEAT};

	virtual void setDataSet(vtkDataSet* dataset);
	virtual void setVisible(bool visible);

	const vtkDataSet *getDataSet(void)
        {return this->mDataSet;}

	const vtkPolyData* getContour(void);
#ifdef VTK_OPENGL2
    const vtkPolyDataMapper* getContourMapper(void);
#else
    const vtkOGRLayerMapper* getContourMapper(void);
#endif
	const vtkActor* getContourActor(void);

    void setContoursVisible(bool vis);
    void setFeaturesVisible(bool vis);
    bool getIsContoursOnlyOn(void)
           {return mContourOnly;}

	NMFeatureType getFeatureType(void);

	QColor getContourColour(void) {return mContourColour;}
    void setContourColour(QColor clr);

	//	double getArea();
	long getNumberOfFeatures(void);

public slots:
	virtual void selectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);
	virtual void writeDataSet(void);

protected:
#ifdef VTK_OPENGL2
    vtkSmartPointer<vtkPolyDataMapper> mContourMapper;
#else
    vtkSmartPointer<vtkOGRLayerMapper> mContourMapper;
#endif
	vtkSmartPointer<vtkActor> mContourActor;
	vtkSmartPointer<vtkPolyData> mContour;

	NMFeatureType mFeatureType;
	vtkSmartPointer<vtkTable> mAttributeTable;
	QColor mContourColour;

    bool mContourOnly;


	void createTableView(void);
	void setContour(vtkPolyData* contour);
	virtual void removeFromMap(void);

protected slots:
	int updateAttributeTable(void);
    void updateSelectionColor();

private:

};

#endif /* NMVECTORLAYER_H_ */
