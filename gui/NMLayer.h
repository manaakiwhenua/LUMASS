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

#include "NMModelComponent.h"
#include "NMTableView.h"
#include "NMSqlTableView.h"

#include <sqlite3.h>
#include <spatialite.h>

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
//#include "vtkColorTransferFunctionSpecialNodes.h"
#include "vtkLookupTable.h"
//#include "NMVtkLookupTable.h"

//#include "vtkScalarsToColors.h"

class NMLogger;

#define ctxNMLayer "NMLayer"
#define NM_LEGEND_RAMP_ROW 2

/*!
 * \brief The CompNumStrings struct is usded as comparison
 *        object for the mMapValueIndices look up map, and
 *        enables alphabetically and numerically correct
 *        sorted legend entries
 */
struct CompNumStrings
{
public:
    bool operator()(const QString& lhs, const QString& rhs)
    {
        bool blhv = false, brhv = false;
        const long long lhv = lhs.toLongLong(&blhv);
        const long long rhv = rhs.toLongLong(&brhv);

        if (blhv && brhv)
        {
            if (lhv < rhv)
            {
                return true;
            }
        }
        else
        {
            return lhs.localeAwareCompare(rhs) < 0 ? true : false;
        }

        return false;
    }
};


//class QVTK_EXPORT NMLayer : public QObject//public NMModelComponent //public QObject
class NMLayer : public QObject
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
    Q_PROPERTY(bool IsSelected READ getIsSelected WRITE setIsSelected)


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
            NM_LEGEND_CLRTAB,
            NM_LEGEND_RGB
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
			NM_RAMP_MANUAL
		};

	NMPropertyGetSet(LegendType, NMLayer::NMLegendType);
	NMPropertyGetSet(LegendClassType, NMLayer::NMLegendClassType);
	NMPropertyGetSet(ColourRamp, NMLayer::NMColourRamp);
	//NMPropertyGetSet(LegendValueField, QString);
	NMPropertyGetSet(LegendDescrField, QString);
	//NMPropertyGetSet(Nodata, double);
	//NMPropertyGetSet(Lower, double);
	//NMPropertyGetSet(Upper, double);
	QString getLegendValueField(void) {return mLegendValueField;}
	double getNodata(void) {return mNodata;}
	double getUpper(void) {return mUpper;}
	double getLower(void) {return mLower;}
	virtual void setLegendValueField(QString field);
	virtual void setNodata(double val);
	virtual void setUpper(double val);
	virtual void setLower(double val);

    void setIsSelected(bool sel);
    bool getIsSelected(void) {return this->mIsSelected;}



signals:
    void nmChanged();
	void LegendTypeChanged();
	void LegendClassTypeChanged();
	void LegendDescrFieldChanged();
	void LegendValueFieldChanged();
	void ColourRampChanged();
	void LowerChanged();
	void UpperChanged();
	void NodataChanged();
    void IsSelectedChanged(bool);


public:
	NMLayer(vtkRenderWindow* renWin,
			vtkRenderer* renderer=0,
			QObject* parent=0);
	virtual ~NMLayer();

    void setLogger(NMLogger* logger){mLogger = logger;}

	virtual void setDataSet(vtkDataSet* dataset);
	virtual bool setFileName(QString filename)
        {this->mFileName = filename; return true;}
    QString getFileName() {return this->mFileName;}
	virtual const vtkDataSet* getDataSet(void)=0;

	const vtkAbstractMapper* getMapper(void);
    const vtkProp3D* getActor(void);
	const vtkRenderer* getRenderer(void);

	virtual void showAttributeTable(void);
	//virtual const vtkTable* getTable(void);
	virtual const QAbstractItemModel* getTable(void);
    QString getSqlTableConnectionName(void){return mQSqlConnectionName;}
	int getColumnIndex(const QString& fieldname);
	QVariant::Type getColumnType(int colidx);
	QString getColumnName(const int idx);
	QStringList getNumericColumns(bool onlyints);
	QStringList getStringColumns(void);

    virtual long long getNumTableRecords();

	const QItemSelection getSelection(void);
    void setSelection(const QItemSelection& sel);


    void loadLegend(const QString& filename);
    void saveLegend(const QString& filename);

	virtual void setBBox(double bbox[6]);
	const double* getBBox(void);
	virtual void getBBox(double bbox[6]);

	// gets sum of covered feature area for
	// polygon cells; and bounding box
	// area for polyline cells
	// and image layers which equals the
	// covered pixel area
	virtual double getArea(void);

    /*! get/set the whole layer's opacity [0,1] */
    double getLayerOpacity();
    void setLayerOpacity(const double& opacity);


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
    bool isInteractive(void);


	//-----------------------------------------------
	//--GET LEGEND INFO---------------------------------

	// get the colour of the legend item in row
	// legendRow; requires provision of a double[4]
	bool getLegendColour(const int legendRow, double* rgba);
	QIcon getLegendIcon(const int legendRow);
	QIcon getColourRampIcon(void);
    QPixmap getColourRampPix(int width, int height);
	QString getLegendName(const int legendRow);
	int getLegendItemCount(void);
	double getLegendItemUpperValue(const int legendRow);
	double getLegendItemLowerValue(const int legendRow);
	bool getLegendItemRange(const int legendRow, double* range);

	bool hasColourTable(void);
	/* get the statistics of the current legend value field;
	 * in case of an image layer without attribute table,
	 * the pixel value statistics are returned for the current
	 * band.
	 *
	 * 0:	min
	 * 1:	max
	 * 2:
	 */
	std::vector<double> getValueFieldStatistics(void);

	QString getLegendTypeStr(NMLayer::NMLegendType type)
		{return mLegendTypeStr.at((int)type);}
	QString getLegendClassTypeStr(NMLayer::NMLegendClassType type)
		{return mLegendClassTypeStr.at((int)type);}
	QString getColourRampStr(NMLayer::NMColourRamp ramptype)
		{return mColourRampStr.at((int)ramptype);}

	const QStringList getLegendTypeStrings(void)
		{return mLegendTypeStr;}
	const QStringList getLegendClassTypeStrings(void)
		{return mLegendClassTypeStr;}
	const QStringList getColourRampStrings(void)
		{return mColourRampStr;}

	NMLayer::NMColourRamp getColourRampFromStr(const QString rampStr);




	// -------------------------------------------------------
	// --SET LEGEND INFO ------------------------------------
	void setLegendColour(const int legendRow, double* rgba);


	//virtual int mapUniqueValues(QString fieldName)=0;
    bool hasChanged(void) {return this->mHasChanged;}

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
    virtual void selectCell(long long cellID, NMLayerSelectionType seltype);
	//void emitDataSetChanged();
	//void emitAttributeTableChanged(
	//		QStringList& slAlteredColumns,
	//		QStringList& slDeletedColumns);
    void forwardLastClickedRowSignal(long long cellID);
    virtual void updateLayerSelection(QList<long long> lstCellId,
        QList<long long> lstNMId, NMLayerSelectionType seltype);

    /*! Create a legend according to user choices */
	virtual void updateLegend(void);
	virtual void updateMapping(void);

    virtual void mapExtentChanged(void);

    void setIsIn3DMode(bool in3d) {this->mIsIn3DMode = in3d;}

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
    void notifyLastClickedRow(NMLayer* l, long long cellID);

protected:
	vtkSmartPointer<vtkRenderWindow> mRenderWindow;
	vtkSmartPointer<vtkRenderer> mRenderer;
	vtkSmartPointer<vtkDataSet> mDataSet;
	vtkSmartPointer<vtkAbstractMapper> mMapper;
    vtkSmartPointer<vtkProp3D> mActor;

	vtkSmartPointer<vtkPolyData> mCellSelection;
	vtkSmartPointer<vtkOGRLayerMapper> mSelectionMapper;
	vtkSmartPointer<vtkProp3D>	mSelectionActor;

	void* mSpatialiteCache;
	sqlite3* mSqlViewConn;
	NMTableView* mTableView;
    NMSqlTableView* mSqlTableView;
    QString mQSqlConnectionName;
	NMFastTrackSelectionModel* mSelectionModel;
	QAbstractItemModel* mTableModel;

    NMLogger* mLogger;

	vtkSmartPointer<vtkEventQtSlotConnect> mVtkConn;

	// ToDo:: deprecated: move into VectorLayer; use
	//        solely the tabel model here
	//vtkSmartPointer<vtkTable> mAttributeTable;


	// hash map linking category values with
	// table info index and lookup table index
	// key = category name (equals the value for unique value classifications)
	// value = the index into the LegendInfo or LookupTable or ClrFunc nodes respectively
	//QHash<QString, int> mHashValueIndices;
    std::map<QString, QVector<int>, CompNumStrings > mMapValueIndices;
	QMap<double, QColor> mUserClrNodes;

    /*! DEPRECATED - not in actual use at the moment
     *
     *  Auxiliary legend category information used in conjunction
	 *  with the layer's lookup table for NMLegendType == NM_LEGEND_INDEXED
	 *  and NMLegendClassType != NM_CLASS_UNIQUE. The table holds
	 *  three vtkAbstractArrays:
	 *
	 *  \c <i>DEPRECATED</i>
	 *  \b rgba (vtkDoubleArray) : double [4]: 0: red; 1: green; 2: blue; 3: alpha \n
	 *
	 *  \b range (vtkDoubleArray) : double [2]: 0: lower value; 1: upper value; \n
	 *  			range domain: [\<lower value\>, \<upper value\>[ \n
	 *
	 *  \b name (vtkStringArray) : vtkString: name of legend category \n
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
	// 5: sample size
	// 6: sum
	double mStats[7];
	double mLower;
	double mUpper;
	double mNodata;

    QString mLegendFileName;
    NMLayer::NMLegendType mLegendFileLegendType;
    NMLayer::NMLegendClassType mLegendFileLegendClassType;


    QString mLegendIndexField;
	QString mLegendValueField;
	QString mLegendDescrField;

	NMLayer::NMLayerType mLayerType;
	NMLayer::NMLegendType mLegendType;
	NMLayer::NMLegendClassType mLegendClassType;
	NMLayer::NMColourRamp mColourRamp;

    //vtkSmartPointer<vtkColorTransferFunctionSpecialNodes> mClrFunc;
    vtkSmartPointer<vtkColorTransferFunction> mClrFunc;
    vtkSmartPointer<vtkLookupTable> mLookupTable;
    //vtkSmartPointer<NMVtkLookupTable> mLookupTable;

	QColor mClrNodata;
	QColor mClrLowerMar;
	QColor mClrUpperMar;

	QIcon mLayerIcon;
	QString mFileName;

	QStringList mLegendTypeStr;
	QStringList mLegendClassTypeStr;
	QStringList mColourRampStr;

	bool mIsVisible;
	bool mIsSelectable;
    bool mIsSelected;
    bool mIsIn3DMode;
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
	//int getColumnIndex(const QString& fieldname);
	//QVariant::Type getColumnType(int colidx);

	virtual void mapSingleSymbol(void);
	virtual void mapUniqueValues(void);
	virtual void mapColourTable(void);
	virtual void mapValueClasses(void);
	virtual void mapValueRamp(void);
    virtual void mapRGBImage(void){}

    //	vtkSmartPointer<vtkColorTransferFunctionSpecialNodes> getColorTransferFunc(
    //			const NMLayer::NMColourRamp& ramp,
    //			const QList<double>& userNodes,
    //			const QList<QColor>& userColours,
    //			bool invertRamp=false);

    vtkSmartPointer<vtkColorTransferFunction> getColorTransferFunc(
            const NMLayer::NMColourRamp& ramp,
            const QList<double>& userNodes,
            const QList<QColor>& userColours,
            bool invertRamp=false);

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
