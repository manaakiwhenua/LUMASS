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
#include <QStringList>

#include "vtkSmartPointer.h"
#include "vtkDataSet.h"
#include "vtkAbstractMapper.h"
#include "vtkProp3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkOGRLayerMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkLookupTable.h"

class QVTK_EXPORT NMLayer : public QObject//public NMModelComponent //public QObject
{
	Q_OBJECT
	Q_ENUMS(NMLegendClassType NMLayerType NMLayerSelectionType NMLegendType)
	Q_PROPERTY(NMLayer::NMLegendType LegendType READ getLegendType WRITE setLegendType NOTIFY LegendTypeChanged)
	Q_PROPERTY(NMLayer::NMLayerType LayerType READ getLayerType)
	Q_PROPERTY(NMLayer::NMLegendClassType LegendClassType READ getLegendClassType WRITE setLegendClassType NOTIFY LegendClassTypeChanged)
	Q_PROPERTY(NMLayer::NMColourRamp ColourRamp READ getColourRamp WRITE setColourRamp NOTIFY ColourRampChanged)
	Q_PROPERTY(QString LegendValueField READ getLegendValueField WRITE setLegendValueField NOTIFY LegendValueFieldChanged)
	Q_PROPERTY(QString LegendDescrField READ getLegendDescrField WRITE setLegendDescrField NOTIFY LegendDescrFieldChanged)
	Q_PROPERTY(double Lower READ getLower WRITE setLower NOTIFY LowerChanged)
	Q_PROPERTY(double Upper READ getUpper WRITE setUpper NOTIFY UpperChanged)
	Q_PROPERTY(double Nodata READ getNodata WRITE setNodata NOTIFY NodataChanged)


public:
	enum NMLayerType
		{
			NM_VECTOR_LAYER,
			NM_IMAGE_LAYER,
			NM_UNKNOWN_LAYER
		};
	enum NMLayerSelectionType
		{
			NM_SEL_NEW,
			NM_SEL_ADD,
			NM_SEL_REMOVE,
			NM_SEL_CLEAR
		};
	enum NMLegendType
		{
			NM_LEGEND_SINGLESYMBOL,
			NM_LEGEND_RAMP,
			NM_LEGEND_INDEXED,
			NM_LEGEND_CLRTAB
		};
	enum NMLegendClassType
		{
			NM_CLASS_UNIQUE,
			NM_CLASS_EQINT,
			NM_CLASS_JENKS,
			NM_CLASS_MANUAL,
			NM_CLASS_SDEV
		};
	enum NMColourRamp
		{
			NM_RAMP_RAINBOW,
			NM_RAMP_GREY,
			NM_RAMP_RED,
			NM_RAMP_BLUE,
			NM_RAMP_GREEN,
			NM_RAMP_RED2BLUE,
			NM_RAMP_BLUE2RED,
			NM_RAMP_ALTITUDE,
			NM_RAMP_BLUE2RED_DIV,
			NM_RAMP_GREEN2RED_DIV,
			NM_RAMP_GREEN2YELLOW2RED,
			NM_RAMP_MANUAL
		};

	NMPropertyGetSet(LegendType, NMLayer::NMLegendType);
	NMPropertyGetSet(LegendClassType, NMLayer::NMLegendClassType);
	NMPropertyGetSet(ColourRamp, NMLayer::NMColourRamp);
	NMPropertyGetSet(LegendValueField, QString);
	NMPropertyGetSet(LegendDescrField, QString);
	NMPropertyGetSet(Nodata, double);
	NMPropertyGetSet(Lower, double);
	NMPropertyGetSet(Upper, double);

signals:
	void LegendTypeChanged();
	void LegendClassTypeChanged();
	void LegendDescrFieldChanged();
	void LegendValueFieldChanged();
	void ColourRampChanged();
	void LowerChanged();
	void UpperChanged();
	void NodataChanged();


public:
	NMLayer(vtkRenderWindow* renWin,
			vtkRenderer* renderer=0,
			QObject* parent=0);
	virtual ~NMLayer();


	virtual void setDataSet(vtkDataSet* dataset);
	virtual bool setFileName(QString filename)
		{this->mFileName = filename; return true;};
	QString getFileName() {return this->mFileName;};
	virtual const vtkDataSet* getDataSet(void)=0;

	const vtkAbstractMapper* getMapper(void);
	const vtkProp3D* getActor(void);
	const vtkRenderer* getRenderer(void);

	virtual void showAttributeTable(void);
	//virtual const vtkTable* getTable(void);
	virtual const QAbstractItemModel* getTable(void);

	const QItemSelection getSelection(void);


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

	QIcon getLayerIcon(void);
	QImage getLayerIconAsImage(void);


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

	QString getLegendTypeStr(NMLayer::NMLegendType type)
		{return mLegendTypeStr.at((int)type);}
	QString getLegendClassTypeStr(NMLayer::NMLegendClassType type)
		{return mLegendClassTypeStr.at((int)type);}
	QString getColourRampStr(NMLayer::NMColourRamp ramptype)
		{return mColourRampStr.at((int)ramptype);}



	//virtual int mapUniqueValues(QString fieldName)=0;
	bool hasChanged(void) {return this->mHasChanged;};

public slots:
	// call this function whenever you've changed the
	// layer's data set and wish other objects get
	// to know about it
	virtual void selectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel);
	virtual void tableDataChanged(const QModelIndex& tl, const QModelIndex& br);
	virtual void tableColumnsInserted(const QModelIndex& parent,
			int startsection, int endsection);
	virtual void tableColumnsRemoved(const QModelIndex& parent,
			int startsection, int endsection);
	virtual void selectedLayerChanged(const NMLayer* layer);
	virtual void writeDataSet(void);
	virtual void selectCell(int cellID, NMLayerSelectionType seltype);
	//void emitDataSetChanged();
	//void emitAttributeTableChanged(
	//		QStringList& slAlteredColumns,
	//		QStringList& slDeletedColumns);
	void forwardLastClickedRowSignal(long cellID);
	virtual void updateLayerSelection(QList<long> lstCellId,
		QList<long> lstNMId, NMLayerSelectionType seltype);
	/*! Create a legend according to user choices */
	virtual void updateLegend(void);

signals:
	void visibilityChanged(const NMLayer* layer);
	void selectabilityChanged(bool bselectable);
	void legendChanged(const NMLayer* layer);
	void dataSetChanged(const NMLayer* layer);
	void attributeTableChanged(vtkTable* table);
	void layerSelectionChanged(const NMLayer* layer);
	void layerProcessingStart();
	void layerProcessingEnd();
	void layerLoaded();
	void notifyLastClickedRow(NMLayer* l, double cellID);

protected:
	vtkSmartPointer<vtkRenderWindow> mRenderWindow;
	vtkSmartPointer<vtkRenderer> mRenderer;
	vtkSmartPointer<vtkDataSet> mDataSet;
	vtkSmartPointer<vtkAbstractMapper> mMapper;
	vtkSmartPointer<vtkProp3D> mActor;

	vtkSmartPointer<vtkPolyData> mCellSelection;
	vtkSmartPointer<vtkOGRLayerMapper> mSelectionMapper;
	vtkSmartPointer<vtkProp3D>	mSelectionActor;

	NMTableView* mTableView;
	NMFastTrackSelectionModel* mSelectionModel;
	QAbstractItemModel* mTableModel;

	vtkSmartPointer<vtkEventQtSlotConnect> mVtkConn;

	// ToDo:: deprecated: move into VectorLayer; use
	//        solely the tabel model here
	//vtkSmartPointer<vtkTable> mAttributeTable;


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

	bool mHasClrTab;

	// Field indices containing RGBA components
	int mClrTabIdx[4];
	long mNumClasses;
	long mNumLegendRows;

	// legend stats
	// 0: min
	// 1: max
	// 2: mean
	// 3: median
	// 4: sdev
	double mStats[5];
	double mLower;
	double mUpper;
	double mNodata;

	QString mLegendValueField;
	QString mLegendDescrField;

	NMLayer::NMLayerType mLayerType;
	NMLayer::NMLegendType mLegendType;
	NMLayer::NMLegendClassType mLegendClassType;
	NMLayer::NMColourRamp mColourRamp;

	vtkSmartPointer<vtkColorTransferFunction> mClrFunc;
	vtkSmartPointer<vtkLookupTable> mLookupTable;

	QIcon mLayerIcon;
	QString mFileName;

	QStringList mLegendTypeStr;
	QStringList mLegendClassTypeStr;
	QStringList mColourRampStr;

	bool mIsVisible;
	bool mIsSelectable;
	int mLayerPos;
	bool mHasChanged;
	double mBBox[6];
	double mTotalArea;

	virtual void createTableView(void);
	/*! gather information to build a legend
	 *  without asking the user for anything */
	virtual void initiateLegend(void);
	virtual void resetLegendInfo(void);
	virtual void removeFromMap(void);
	virtual void connectTableSel(void);
	virtual void disconnectTableSel(void);
	void printSelRanges(const QItemSelection& sel, const QString& msg);
	int getColumnIndex(const QString& fieldname);
	QVariant::Type getColumnType(int colidx);

	virtual void mapSingleSymbol(void);
	virtual void mapUniqueValues(void);
	virtual void mapColourTable(void);
	virtual void mapValueClasses(void);
	virtual void mapValueRamp(void);

protected slots:
	virtual int updateAttributeTable(void);
	//virtual void updateDataSet(QStringList& slAlteredColumns,
	//		QStringList& slDeletedColumns);
	virtual void updateSelectionData();



private:


};

Q_DECLARE_METATYPE(NMLayer::NMLayerType)
Q_DECLARE_METATYPE(NMLayer::NMLegendType)
Q_DECLARE_METATYPE(NMLayer::NMLegendClassType)
Q_DECLARE_METATYPE(NMLayer::NMColourRamp)

#endif /* NMLAYER_H_ */
