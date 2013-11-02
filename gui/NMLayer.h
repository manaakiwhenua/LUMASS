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
 * NMLayer.h
 *
 * Abstract superclass for NMVectorLayer and NMImageLayer
 *
 *
 *  Created on: 10/03/2011
 *      Author: alex
 */

#ifndef NMLAYER_H_
#define NMLAYER_H_

#define ctxNMLayer "NMLayer"
#include "nmlog.h"
#include "NMModelComponent.h"
#include "NMTableView.h"

#include "QVTKWin32Header.h"
#include "vtkConfigure.h"

#include <QItemSelectionModel>
#include <QObject>
#include <QHash>
#include <QVector>
#include <QIcon>

#include "vtkSmartPointer.h"
#include "vtkDataSet.h"
#include "vtkAbstractMapper.h"
#include "vtkProp3D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTable.h"

//class NMTableView;

class QVTK_EXPORT NMLayer : public QObject//public NMModelComponent //public QObject
{
	Q_OBJECT
	Q_ENUMS(NMLayerType NMLayerSelectionType)

public:
	NMLayer(vtkRenderWindow* renWin,
			vtkRenderer* renderer=0,
			QObject* parent=0);
	virtual ~NMLayer();

	enum NMLayerType {NM_VECTOR_LAYER, NM_IMAGE_LAYER, NM_UNKNOWN_LAYER};
	enum NMLayerSelectionType {NM_SEL_NEW,
		NM_SEL_ADD,
		NM_SEL_REMOVE,
		NM_SEL_CLEAR};

	virtual void setDataSet(vtkDataSet* dataset);
	virtual bool setFileName(QString filename)
		{this->mFileName = filename; return true;};
	QString getFileName() {return this->mFileName;};
	virtual const vtkDataSet* getDataSet(void)=0;

	const vtkAbstractMapper* getMapper(void);
	const vtkProp3D* getActor(void);
	const vtkRenderer* getRenderer(void);

	virtual void showAttributeTable(void);
	virtual const vtkTable* getTable(void);

	virtual void setBBox(double bbox[6]);
	const double* getBBox(void);
	virtual void getBBox(double bbox[6]);

	// gets sum of covered feature area for
	// polygon cells; and bounding box
	// area for polyline cells
	// and image layers which equals the
	// covered pixel area
	virtual double getArea(void);

	int setLayerPos(int pos);
	int getLayerPos(void);

	NMLayer::NMLayerType getLayerType(void);
	void setLayerType(NMLayerType type);
	bool isVectorLayer(void);
	bool isImageLayer(void);

	QIcon getLayerIcon(void)
		{return this->mLayerIcon;};

	bool isVisible(void);
	virtual void setVisible(bool visible);
	bool isSelectable(void);
	virtual void setSelectable(bool selectable);

	//--LEGEND INFO---------------------------------
	// get the colour of the legend item in row
	// legendRow; requires provision of a double[4]
	bool getLegendColour(int legendRow, double* rgba);
	QIcon getLegendIcon(int legendRow);
	QString getLegendName(int legendRow);
	int getLegendItemCount(void);
	double getLegendItemUpperValue(int legendRow);
	double getLegendItemLowerValue(int legendRow);
	bool getLegendItemRange(int legendRow, double* range);

	virtual int mapUniqueValues(QString fieldName)=0;

public slots:
	// call this function whenever you've changed the
	// layer's data set and wish other objects get
	// to know about it
	virtual void writeDataSet(void);
	void emitDataSetChanged();
	void emitAttributeTableChanged(
			QStringList& slAlteredColumns,
			QStringList& slDeletedColumns);
	bool hasChanged(void) {return this->mHasChanged;};
	virtual void updateLayerSelection(QList<long> lstCellId,
			QList<long> lstNMId, NMLayerSelectionType seltype);

signals:
	void visibilityChanged(const NMLayer* layer);
	void selectabilityChanged(const NMLayer* layer);
	void legendChanged(const NMLayer* layer);
	void dataSetChanged(const NMLayer* layer);
	void attributeTableChanged(vtkTable* table);
	void layerSelectionChanged(const NMLayer* layer);

protected:
	vtkSmartPointer<vtkRenderWindow> mRenderWindow;
	vtkSmartPointer<vtkRenderer> mRenderer;
	vtkSmartPointer<vtkDataSet> mDataSet;
	vtkSmartPointer<vtkAbstractMapper> mMapper;
	vtkSmartPointer<vtkProp3D> mActor;
	NMLayerType mLayerType;

	NMTableView* mTableView;
	QItemSelectionModel* mSelectionModel;
	QAbstractItemModel* mTableModel;

	// ToDo:: deprecated: move into VectorLayer; use
	//        solely the tabel model here
	vtkSmartPointer<vtkTable> mAttributeTable;


	// hash map linking category values with
	// table info index and lookup table info
	// of the associated mapper
	// key = category name (equals the value for unique value classifications)
	// value = a vector of n indices; whereas the value[0] denotes the index
	//		   of the table info table and all subsequent indices value[1] ...
	//		   value[n-1] are denoting the lookup-table indices associated
	// 			with this legend category
	QHash<QString, QVector<int> > mHashValueIndices;

	/* Legend category information, is supposed
	 * to be used in conjunction with the
	 * lookup table of the layer
	 *
	 * the array holds three vtkArrays
	 *
	 * "rgba": vtkDoubleArray
	 * 		   double [4]: 0: red; 1: green; 2: blue; 3: alpha
	 *
	 * "range": vtkDoubleArray
	 * 			double [2]: 0: lower value; 1: upper value
	 *
	 * "name": vtkStringArray
	 * 			vtkString: name of legend category
	 *
	 */
	vtkSmartPointer<vtkTable> mLegendInfo;


	QIcon mLayerIcon;
	QString mFileName;
	bool mIsVisible;
	bool mIsSelectable;
	int mLayerPos;
	bool mHasChanged;

	double mBBox[6];
	double mTotalArea;

	virtual void createTableView(void);
	virtual void resetLegendInfo(void);
	virtual void removeFromMap(void);

protected slots:
	virtual int updateAttributeTable(void);
	virtual void updateDataSet(QStringList& slAlteredColumns,
			QStringList& slDeletedColumns);
	virtual void updateSelectionData();



private:


};

#endif /* NMLAYER_H_ */
