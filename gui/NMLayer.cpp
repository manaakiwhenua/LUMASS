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
 * NMLayer.cpp
 *
 *  Created on: 10/03/2011
 *      Author: alex
 */

#include "NMLayer.h"
#include "NMImageLayer.h"
#include "NMVectorLayer.h"
#include "NMQtOtbAttributeTableModel.h"

#include <QTime>
#include <QColor>
#include <QPainter>

#include "vtkPointData.h"
#include "vtkMapper.h"
#include "vtkProperty.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkShortArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkMath.h"
#include "vtkQtEditableTableModelAdapter.h"
#include "NMQtOtbAttributeTableModel.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "NMVtkLookupTable.h"
#include "NMVtkOpenGLImageSliceMapper.h"
#include "vtkImageSlice.h"
#include "vtkDelimitedTextReader.h"
#include "vtkDelimitedTextWriter.h"

#define VALUE_MARGIN 0.0000001

NMLayer::NMLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
	: QObject(parent), mSelectionModel(0), mTableModel(0), mTableView(0),
      mSqlTableView(0), mDataSet(0), mActor(0), mMapper(0),
      mIsVisible(false), mIsSelectable(true),
      mIsSelected(false), mHasChanged(false),
	  mLayerType(NM_UNKNOWN_LAYER), mClrFunc(0), mLookupTable(0), mLegendInfo(0),
      mLegendClassType(NM_CLASS_JENKS), mColourRamp(NM_RAMP_BLUE2RED_DIV),
      mIsIn3DMode(false)
{
	if (renWin == 0)
	{
		NMErr(ctxNMLayer, << "invalid render window specified!");
		return;
	}
	this->mRenderWindow = renWin;

	if (renderer == 0)
		this->mRenderer = vtkSmartPointer<vtkRenderer>::New();
	else
		this->mRenderer = renderer;

	//this->mRenderer->SetUseDepthPeeling(1);
	//this->mRenderer->SetMaximumNumberOfPeels(100);
	//this->mRenderer->SetOcclusionRatio(0.1);

	// make a legendinfo table for this layer
	//this->resetLegendInfo();

	this->mFileName.clear();
	this->mLayerPos = this->mRenderer->GetLayer();

	// set initial bounding box between 0 and 1
	// for each dimension
	for (int i=0; i < 6; i += 2)
		this->mBBox[i] = 0;

	for (int i=1; i < 6; i += 2)
		this->mBBox[i] = 1;

	this->mTotalArea = -1;

	mUpper = std::numeric_limits<double>::max();
	mLower = -std::numeric_limits<double>::max();
	mNodata = -std::numeric_limits<double>::max();

	// schweinerosa
	mClrNodata = QColor(255, 192, 192);
	// babyblau
	mClrLowerMar = QColor(0,0,0);//QColor(192, 192, 255);
	// mint
	mClrUpperMar = QColor(0, 255, 0);


	// init some enum to string conversion lists
	mLegendTypeStr << "NM_LEGEND_SINGLESYMBOL"
			       << "NM_LEGEND_RAMP"
			       << "NM_LEGEND_INDEXED"
                   << "NM_LEGEND_CLRTAB"
                   << "NM_LEGEND_RGB";

	mLegendClassTypeStr << "NM_CLASS_UNIQUE"
			    << "NM_CLASS_EQINT"
			    << "NM_CLASS_JENKS"
			    << "NM_CLASS_MANUAL"
			    << "NM_CLASS_SDEV";

	mColourRampStr << "NM_RAMP_RAINBOW"
			       << "NM_RAMP_GREY"
			       << "NM_RAMP_RED"
			       << "NM_RAMP_BLUE"
			       << "NM_RAMP_GREEN"
			       << "NM_RAMP_RED2BLUE"
			       << "NM_RAMP_BLUE2RED"
			       << "NM_RAMP_ALTITUDE"
			       << "NM_RAMP_BLUE2RED_DIV"
			       << "NM_RAMP_GREEN2RED_DIV"
			       << "NM_RAMP_MANUAL";
}

NMLayer::~NMLayer()
{
    //NMDebugCtx(ctxNMLayer, << " - " << this->objectName().toStdString());

	if (this->mRenderer != 0)
	{
		this->removeFromMap();
	}

	if (this->mTableView != 0)
	{
		this->mTableView->close();
		delete this->mTableView;
	}

    if (this->mSqlTableView != 0)
    {
        this->mSqlTableView->close();
        delete this->mSqlTableView;
    }

    if (mSelectionModel != 0)
		delete mSelectionModel;

	if (mTableModel != 0)
		delete mTableModel;



    //NMDebugCtx(ctxNMLayer, << "done!");
}

//void NMLayer::emitDataSetChanged()
//{
//	NMDebugCtx(ctxNMLayer, << "...");
//
//	this->updateAttributeTable();
//	this->mHasChanged = true;
//	emit dataSetChanged(this);
//
//	NMDebugCtx(ctxNMLayer, << "done!");
//}

//void NMLayer::emitAttributeTableChanged(
//		QStringList& slAlteredColumns,
//		QStringList& slDeletedColumns)
//{
//	NMDebugCtx(ctxNMLayer, << "...");
//
//	this->updateDataSet(slAlteredColumns, slDeletedColumns);
//	this->updateAttributeTable();
//	this->mHasChanged = true;
////	emit attributeTableChanged(this->mAttributeTable);
//
//	NMDebugCtx(ctxNMLayer, << "done!");
//}

void NMLayer::updateLayerSelection(QList<long> lstCellId,
		QList<long> lstNMId, NMLayerSelectionType seltype)
{

}

QString
NMLayer::getColumnName(int idx)
{
	if (mTableModel == 0)
		return QString();

	return mTableModel->headerData(idx,
			Qt::Horizontal, Qt::DisplayRole).toString();
}

QStringList
NMLayer::getNumericColumns(bool onlyints)
{
	QStringList cols;
	if (mTableModel == 0)
	{
		return cols;
	}

	int ncols = mTableModel->columnCount(QModelIndex());
	for (int c=0; c < ncols; ++c)
	{
		const QVariant::Type type = this->getColumnType(c);
		if (type != QVariant::String)
		{
			if (onlyints)
			{
				if (type != QVariant::Double)
				{
					cols << this->getColumnName(c);
				}
			}
			else
			{
				cols << this->getColumnName(c);
			}
		}
	}

	return cols;
}

QStringList
NMLayer::getStringColumns(void)
{
	QStringList cols;
	if (mTableModel == 0)
	{
		return cols;
	}

	int ncols = mTableModel->columnCount(QModelIndex());
	for (int c=0; c < ncols; ++c)
	{
		if (this->getColumnType(c) == QVariant::String)
		{
			cols << this->getColumnName(c);
		}
	}

	return cols;
}

void NMLayer::resetLegendInfo(void)
{
	// clear the HashMap
	//this->mHashValueIndices.clear();

	// create new one
	this->mLegendInfo = vtkSmartPointer<vtkTable>::New();

	//vtkSmartPointer<vtkDoubleArray> rgba = vtkSmartPointer<vtkDoubleArray>::New();
	//rgba->SetName("rgba");
	//rgba->SetNumberOfComponents(4);
	//mLegendInfo->AddColumn(rgba);

	vtkSmartPointer<vtkDoubleArray> range = vtkSmartPointer<vtkDoubleArray>::New();
	range->SetName("range");
	range->SetNumberOfComponents(2);
	mLegendInfo->AddColumn(range);

	vtkSmartPointer<vtkStringArray> name = vtkSmartPointer<vtkStringArray>::New();
	name->SetName("name");
	name->SetNumberOfComponents(1);
	mLegendInfo->AddColumn(name);

}

int
NMLayer::getColumnIndex(const QString& fieldname)
{
	int colidx = -1;

	if (mTableModel == 0)
		return colidx;


	int ncols = mTableModel->columnCount(QModelIndex());
	QString colname;
	for (int c=0; c < ncols; ++c)
	{
		colname = mTableModel->headerData(c, Qt::Horizontal).toString();
		if (colname.compare(fieldname, Qt::CaseInsensitive) == 0)
		{
			colidx = c;
			break;
		}
	}

	return colidx;
}

QVariant::Type
NMLayer::getColumnType(int colidx)
{
    if (   mTableModel == 0
        || colidx < 0
        || colidx >= mTableModel->columnCount()
       )
    {
		return QVariant::Invalid;
    }

	QModelIndex midx = mTableModel->index(0, colidx, QModelIndex());
	return mTableModel->data(midx, Qt::DisplayRole).type();
}

void
NMLayer::initiateLegend(void)
{
	NMDebugCtx(ctxNMLayer, << "...");

    // bail out of point sets for now
    NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
    if (vl && vl->getFeatureType() == NMVectorLayer::NM_POINT_FEAT)
        return;

	this->updateAttributeTable();

	// ----------------------------------------------------------------------------------
	// GATHER IMAGE LAYER INFORMATION
    bool brawpixels = false;
    bool bRGB = false;
    NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
    if (   (il && mTableModel == 0)
        || (il && il->getNumBands() == 3)
        || (    il
            &&  (   il->getITKComponentType() == otb::ImageIOBase::FLOAT
                 || il->getITKComponentType() == otb::ImageIOBase::DOUBLE
                )
           )
       )
	{
        // we settle for a 'guestimate' of the proper statistics
        std::vector<double> imgStats = il->getWindowStatistics();
        int i=0;
        for (; i < imgStats.size(); ++i)
        {
            mStats[i] = imgStats[i];
        }
        for (; i < 7; ++i)
        {
            mStats[i] = -9999;
        }

        setNodata(il->getDefaultNodata());
        setLower(mStats[0]);
        setUpper(mStats[1]);
        brawpixels = il->getNumBands() == 1 ? true : false;
        bRGB = il->getNumBands() == 3 ? true : false;
	}
	else
	{
		setNodata(-2147483647);
	}


	// do we have a table
    if (mTableModel && !brawpixels && !bRGB)
	{
		NMDebugAI(<< "TableModel-based legend ..." << std::endl);
		// do we have a colour table
		mLegendValueField.clear();
		mLegendDescrField.clear();
		// reset colour table indices
		mHasClrTab = false;
		for (int i=0; i < 4; ++i)
			mClrTabIdx[i] = -1;

		// -------------------------------------------------------------------------------------
		// ANALYSE ATTRIBUTE TABLE

		int ncols = this->mTableModel->columnCount(QModelIndex());

        //ToDo:: let's destrict us to 256 categories
        mNumClasses = this->mTableModel->rowCount(QModelIndex());
		// + 4: description (field); nodata; > upper; < lower
		mNumLegendRows = mNumClasses + 4;

		int colidx = -1;
		for (int i=0; i < ncols; ++i)
		{
			const QModelIndex ci = mTableModel->index(0, i, QModelIndex());
			if (mTableModel->data(ci, Qt::DisplayRole).type() != QVariant::String)
			{
				QString fieldName = mTableModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
				if (	mLegendValueField.isEmpty()
					&&  fieldName != "rowidx"
                    &&  fieldName != "rowid"
					&&  fieldName != "nm_id"
					&&  fieldName != "nm_hole"
					&&  fieldName != "nm_sel"
				   )
				{
					mLegendValueField = fieldName;
					mLegendDescrField = fieldName;
					colidx = i;
				}

				if (fieldName.compare("red", Qt::CaseInsensitive) == 0)
					mClrTabIdx[0] = i;
				else if (fieldName.compare("green", Qt::CaseInsensitive) == 0)
					mClrTabIdx[1] = i;
				else if (fieldName.compare("blue", Qt::CaseInsensitive) == 0)
					mClrTabIdx[2] = i;
				else if (	fieldName.compare("alpha", Qt::CaseInsensitive) == 0
						 || fieldName.compare("opacity", Qt::CaseInsensitive) == 0
						)
					mClrTabIdx[3] = i;
			}
		}

		//NMDebugAI(<< "lower=" << mLower << " upper=" << mUpper << " nodata=" << mNodata << std::endl);

		// --------------------------------------------------------------------------------------
		// DECIDE ON LEGEND TYPE

		// ..........................................................
		// SINGLESYMBOL

		if (mLegendValueField.isEmpty())
		{
			NMDebugAI(<< "... mapping all to one single symbol ..." << std::endl);
			mLegendType = NMLayer::NM_LEGEND_SINGLESYMBOL;
			mNumClasses = 1;
			// +1: description (field)
			mNumLegendRows = mNumClasses + 1;
			if (this->mLayerType == NMLayer::NM_IMAGE_LAYER)
				mLegendDescrField = tr("All Pixel");
			else
				mLegendDescrField = tr("All Features");
		}
		else
		{
			// .........................................................................
			// COLOURTABLE

			if (	mClrTabIdx[0] >=0
				&&  mClrTabIdx[1] >=0
				&&  mClrTabIdx[2] >=0
				&&  mClrTabIdx[3] >=0
			   )
			{
				mHasClrTab = true;
				NMDebugAI(<< "... use colour table for mapping ..." << std::endl);
				mLegendType = NMLayer::NM_LEGEND_CLRTAB;
				mLegendDescrField = mLegendValueField;
				// +2: description (field); nodata
				mNumLegendRows = mNumClasses+2;
			}
			// ..........................................................................
			// RAMP NUMERIC ATTRIBUTE

			else if (	this->getColumnType(colidx) == QVariant::Int
					 || this->getColumnType(colidx) == QVariant::Double
					)
			{
				NMDebugAI(<< "... mapping a numeric attribute ..." << std::endl);

                // updates also internal stats
                this->getValueFieldStatistics();

				mLower = mStats[0];
				mUpper = mStats[1];

				mLegendType = NMLayer::NM_LEGEND_RAMP;
				mColourRamp = NMLayer::NM_RAMP_BLUE2RED_DIV;

			}
			// ..................................................................................
			// CLASS_UNIQUE STRING ATTRIBUTE
			else
			{
                NMDebugAI(<< "... mapping unique values of '" << mLegendValueField.toStdString()
                          << "' ..." << std::endl);
				mLegendType = NMLayer::NM_LEGEND_INDEXED;
				mLegendClassType = NMLayer::NM_CLASS_UNIQUE;
				// +2: description (field); nodata ('other')
				mNumLegendRows = mNumClasses+2;
			}
		}
	}
	// ...........................................................................................
	// RAMP (RAW) PIXEL VALUES
	else
	{
		NMDebugAI(<< "... mapping pixel values ..." << std::endl);

        if (bRGB)
        {
            this->mLegendType = NMLayer::NM_LEGEND_RGB;
            mLegendValueField = "RGB";
            mLegendDescrField = "Band Number";

            mNumClasses = 3;
            // +4: description (field); red; green; blue
            mNumLegendRows = mNumClasses + 1;
        }
        else
        {
            this->mLegendType = NMLayer::NM_LEGEND_RAMP;
            mLegendValueField = "Pixel Values";
            mLegendDescrField = "Pixel Values";

            mNumClasses = 256;
            // +4: description (field); nodata; > upper; < lower
            mNumLegendRows = 4;
        }

		this->mColourRamp = NMLayer::NM_RAMP_BLUE2RED_DIV;
	}

	NMDebugAI(<< "Layer Statistics: " << std::endl);
	NMDebugAI(<< "min: " << mStats[0] << std::endl);
	NMDebugAI(<< "max: " << mStats[1] << std::endl);
	NMDebugAI(<< "mean: " << mStats[2] << std::endl);
	NMDebugAI(<< "median: " << mStats[3] << std::endl);
	NMDebugAI(<< "sdev: " << mStats[4] << std::endl);

	NMDebugCtx(ctxNMLayer, << "done!");
	this->updateMapping();
}

double
NMLayer::getLayerOpacity()
{
    if (this->getLayerType() == NMLayer::NM_VECTOR_LAYER)
    {
        NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
        vtkActor* actor = vtkActor::SafeDownCast(
                    const_cast<vtkProp3D*>(vl->getActor()));
        return actor->GetProperty()->GetOpacity();
    }
    else
    {
        NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
        if (il)
        {
            vtkImageSlice* slice = vtkImageSlice::SafeDownCast(
                        const_cast<vtkProp3D*>(il->getActor()));
            return slice->GetProperty()->GetOpacity();
        }

    }
}


void
NMLayer::setLayerOpacity(const double& opacity)
{
    if (    this->mActor.GetPointer() != 0
        &&  opacity >= 0
        &&  opacity <= 1
       )
    {
        if (this->getLayerType() == NMLayer::NM_VECTOR_LAYER)
        {
            NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
            vtkActor* actor = vtkActor::SafeDownCast(
                        const_cast<vtkProp3D*>(vl->getActor()));
            actor->GetProperty()->SetOpacity(opacity);
        }
        else
        {
            NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
            if (il)
            {
                vtkImageSlice* slice = vtkImageSlice::SafeDownCast(
                            const_cast<vtkProp3D*>(il->getActor()));
                slice->GetProperty()->SetOpacity(opacity);
            }
        }
    }
}



void
NMLayer::updateMapping(void)
{

    NMDebugCtx(ctxNMLayer, << "...");

	this->resetLegendInfo();

	bool clrfunc = false;
	switch(mLegendType)
	{
	case NM_LEGEND_RAMP:
        // need to call mapRGBImage to set the band min max
        // before we build the ramp
        if (mLegendValueField.startsWith(QString("Band #")))
        {
            this->mapRGBImage();
        }
        this->mapValueRamp();
        clrfunc = mLookupTable.GetPointer() != 0 ? false : true;
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				this->mapUniqueValues();
				break;

			default:
				this->mapValueClasses();
				break;
			}
		}
		break;

	case NM_LEGEND_CLRTAB:
		this->mapColourTable();
		break;

    case NM_LEGEND_RGB:
        {
            this->mapRGBImage();
        }
        break;

	case NM_LEGEND_SINGLESYMBOL:
	default:
		this->mapSingleSymbol();
		break;
	}

    if (mLayerType == NM_IMAGE_LAYER)
	{
        NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
        vtkImageProperty* iprop = const_cast<vtkImageProperty*>(il->getImageProperty());
        if (mLegendValueField != "RGB")
        {
            iprop->SetUseLookupTableScalarRange(1);
            if (!clrfunc)
            {
                iprop->SetLookupTable(mLookupTable);

            }
            else
                iprop->SetLookupTable(mClrFunc);
        }
        else
        {
            ;
        }
    }
	else
	{
		vtkOGRLayerMapper* mapper = vtkOGRLayerMapper::SafeDownCast(this->mMapper);
		if (!clrfunc)
			mapper->SetLookupTable(mLookupTable);
		else
			mapper->SetLookupTable(mClrFunc);
		mapper->UseLookupTableScalarRangeOn();
	}

	emit legendChanged(this);
	emit visibilityChanged(this);

	NMDebugCtx(ctxNMLayer, << "done!");
}

void NMLayer::updateLegend()
{
	emit legendChanged(this);
}

void
NMLayer::mapSingleSymbol()
{
	NMDebugCtx(ctxNMLayer, << "...");

	double minrange = mNodata > mUpper ? mUpper : mNodata;
	double maxrange = minrange != mNodata ? mNodata : mLower;

	long ncells = 2;
	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	if (mTableModel)
	{
		if (mLayerType == NM_IMAGE_LAYER)
		{
			mLegendDescrField = tr("All Pixel");
			ncells = mTableModel->rowCount(QModelIndex())+1;
			mNumLegendRows = 3;
		}
		else
		{
			vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
			vtkLongArray* nmids = vtkLongArray::SafeDownCast(dsAttr->GetArray("nm_id"));
			ncells = nmids->GetNumberOfTuples();
			mLegendDescrField = tr("All Features");
			minrange = 0;
			maxrange = ncells-1;
			mNumLegendRows = 2;
		}
	}
	else
	{
		mLegendDescrField = tr("All Pixel");
		ncells = 2;
		mNumLegendRows = 3;
	}
	mLookupTable->SetNumberOfTableValues(ncells);
	mLookupTable->SetTableRange(minrange, maxrange);

	// chose a random colour
	QString sVal;
	vtkMath::RandomSeed(QTime::currentTime().msec());

	double rgba[4];
	for (int i=0; i < 3; i++)
		rgba[i] = vtkMath::Random();

	if (mLayerType == NMLayer::NM_IMAGE_LAYER)
	{
		double alpha1, alpha2;
		if (minrange == maxrange && minrange == mNodata)
		{
			alpha1 = 0;
			alpha2 = 0;
		}
		else
		{
			alpha1 = minrange == mNodata ? 0 : 1;
			alpha2 = alpha1 == 0 ? 1 : 0;
		}

		mLookupTable->SetTableValue(0, rgba[0], rgba[1], rgba[2], alpha1);
		for (long r=1; r < ncells; ++r)
		{
			mLookupTable->SetTableValue(r, rgba[0], rgba[1], rgba[2], alpha2);
		}
	}
	else
	{
		for (long r=0; r < ncells; ++r)
		{
			mLookupTable->SetTableValue(r, rgba[0], rgba[1], rgba[2], 1);
		}
	}

	NMDebugCtx(ctxNMLayer, << "done!");
}

void
NMLayer::loadLegend(const QString& filename)
{
    if (this->mTableModel == 0)
    {
        NMErr(ctxNMLayer, << "Need an attribute table for mapping!");
        return;
    }

    if (    this->mLegendType != NMLayer::NM_LEGEND_INDEXED
        &&  this->mLegendClassType != NMLayer::NM_CLASS_UNIQUE
        )
    {
        return;
    }

    if (this->mLookupTable.GetPointer() == 0)
        return;

    // load the table
    vtkSmartPointer<vtkDelimitedTextReader> tabReader =
                        vtkSmartPointer<vtkDelimitedTextReader>::New();

    tabReader->SetFileName(filename.toStdString().c_str());
    tabReader->SetHaveHeaders(true);
    tabReader->DetectNumericColumnsOn();
    tabReader->SetTrimWhitespacePriorToNumericConversion(1);
    tabReader->SetFieldDelimiterCharacters(",\t");
    tabReader->SetUseStringDelimiter(1);
    tabReader->Update();

    vtkSmartPointer<vtkTable> clrTab = tabReader->GetOutput();

    //    for (int c=0; c < clrTab->GetNumberOfColumns(); ++c)
    //    {
    //        NMDebugAI(<< clrTab->GetColumn(c)->GetName() << ": "
    //                  << clrTab->GetColumn(c)->GetDataTypeAsString()
    //                  << std::endl);
    //    }

    //    vtkStringArray* cat = vtkStringArray::SafeDownCast(
    //                clrTab->GetColumnByName("Category"));
    vtkAbstractArray* cat = clrTab->GetColumnByName("Category");
    vtkDataArray* red = vtkDataArray::SafeDownCast(
                clrTab->GetColumnByName("red"));
    vtkDataArray* green = vtkDataArray::SafeDownCast(
                clrTab->GetColumnByName("green"));
    vtkDataArray* blue = vtkDataArray::SafeDownCast(
                clrTab->GetColumnByName("blue"));
    vtkDataArray* alpha = vtkDataArray::SafeDownCast(
                clrTab->GetColumnByName("alpha"));

    if (cat == 0 || red == 0 || green == 0 || blue == 0)
        return;

    int valIdx = this->getColumnIndex(mLegendValueField);
    vtkDataSetAttributes* dsa = 0;
    vtkUnsignedCharArray* hole = 0;
    vtkAbstractArray* valar = 0;
    long ncells = 0;
    int numComp = 3;
    QList<vtkDataArray*> arList;
    arList << red << green << blue;
    if (alpha)
    {
        numComp = 4;
        arList << alpha;
    }

    bool bIsNumeric = true;
    if (this->mLayerType == NMLayer::NM_VECTOR_LAYER)
    {
        dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
        hole = vtkUnsignedCharArray::SafeDownCast(dsa->GetArray("nm_hole"));
        valar = dsa->GetAbstractArray(
                    this->getColumnIndex(mLegendValueField));
        if (valar->GetDataType() == VTK_STRING)
        {
            bIsNumeric = false;
        }
        ncells = valar->GetNumberOfTuples();
    }
    else
    {
        ncells = mTableModel->rowCount();
        if (this->getColumnType(valIdx) == QVariant::String)
        {
            bIsNumeric = false;
        }
    }

    // build hash structure
    QHash<QString, int> clridx;
    for (int r=0; r < clrTab->GetNumberOfRows(); ++r)
    {
        QString valCat(cat->GetVariantValue(r).ToString().c_str());//cat->GetValue(r).c_str();
        clridx.insert(valCat, r);
    }

    QHash<QString, int>::const_iterator it;
    QString sVal;
    qlonglong val;
    double rgba[] = {0.0,0.0,0.0,1.0};
    QModelIndex vi;
    for (int t=0; t < ncells; ++t)
    {
        if (hole && hole->GetValue(t))
        {
            mLookupTable->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
            continue;
        }

        if (!bIsNumeric)
        {
            if (valar)
            {
                sVal = valar->GetVariantValue(t).ToString().c_str();
            }
            else
            {
                vi = mTableModel->index(t, valIdx);
                sVal = vi.data().toString();
            }
        }
        else
        {
            if (valar)
            {
                val = valar->GetVariantValue(t).ToLongLong();
                sVal = QString("%1").arg(val);
            }
            else
            {
                vi = mTableModel->index(t, valIdx);
                val = vi.data().toLongLong();
                sVal = QString("%1").arg(val);
            }
        }

        it = clridx.find(sVal);
        if (it != clridx.cend())
        {
            int id = clridx.value(sVal);

            for (int i=0; i < numComp; ++i)
            {
                rgba[i] = arList.at(i)->GetTuple1(id) / 255.0;
            }
        }
        else
        {
            for (int i=0; i < numComp; ++i)
            {
                rgba[i] = arList.at(i)->GetTuple1(0) / 255.0;
            }
        }
        this->mLookupTable->SetTableValue(t,
                          rgba[0], rgba[1], rgba[2], rgba[3]);
    }

    emit legendChanged(this);
    emit visibilityChanged(this);
}

void
NMLayer::saveLegend(const QString &filename)
{
    if (this->mLookupTable == 0)
    {
        return;
    }

    int numCat = this->mMapValueIndices.size();
    if (numCat == 0)
    {
        return;
    }

    vtkSmartPointer<vtkTable> tab = vtkSmartPointer<vtkTable>::New();

    vtkSmartPointer<vtkStringArray> cat = vtkSmartPointer<vtkStringArray>::New();
    cat->SetNumberOfComponents(1);
    cat->SetNumberOfTuples(numCat+1);
    cat->SetName("Category");
    vtkSmartPointer<vtkShortArray> red = vtkSmartPointer<vtkShortArray>::New();
    red->SetNumberOfComponents(1);
    red->SetNumberOfTuples(numCat+1);
    red->SetName("red");
    vtkSmartPointer<vtkShortArray> green = vtkSmartPointer<vtkShortArray>::New();
    green->SetNumberOfComponents(1);
    green->SetNumberOfTuples(numCat+1);
    green->SetName("green");
    vtkSmartPointer<vtkShortArray> blue = vtkSmartPointer<vtkShortArray>::New();
    blue->SetNumberOfComponents(1);
    blue->SetNumberOfTuples(numCat+1);
    blue->SetName("blue");
    vtkSmartPointer<vtkShortArray> alpha = vtkSmartPointer<vtkShortArray>::New();
    alpha->SetNumberOfComponents(1);
    alpha->SetNumberOfTuples(numCat+1);
    alpha->SetName("alpha");

    // fill nodata value with white colour
    // TODO: add a nodata value setting option via the user interface

    red->SetValue(0, 255);
    green->SetValue(0, 255);
    blue->SetValue(0, 255);
    alpha->SetValue(0, 255);


    QList<vtkShortArray*> arList;
    arList << red << green << blue;

    //NMDebugAI(<< "going to save " << numCat << " colours ..." << std::endl);

    QMap<QString, QVector<int> >::const_iterator it = this->mMapValueIndices.cbegin();
    int count = 1;
    while(it != this->mMapValueIndices.cend())
    {
        cat->SetValue(count, it.key().toStdString().c_str());

        double clr[4];
        mLookupTable->GetColor(it.value().at(0), clr);

        //NMDebugAI(<< cat->GetValue(count).c_str() << ": ");
        for (int i=0; i < 3; ++i)
        {
            double scaledClr = clr[i] * 255.0 + 0.5;
            short shortClr = scaledClr > 255.0 ? (short)255 : (short)scaledClr;
            //NMDebug(<< clr[i] << "->" << shortClr << " ");

            arList.at(i)->SetValue(count, shortClr);//(short) (clr[i] * 255.0 + 0.5));

        }
        double dop = mLookupTable->GetOpacity(it.value().at(0)) * 255.0 + 0.5;
        short opacity = dop > 255.0 ? (short)255 : (short)dop;
        //NMDebug(<< dop << "->" << opacity << std::endl);

        alpha->SetValue(count, opacity);

        ++count;
        it++;
    }

    tab->AddColumn(cat);
    tab->AddColumn(red);
    tab->AddColumn(green);
    tab->AddColumn(blue);
    tab->AddColumn(alpha);

    vtkSmartPointer<vtkDelimitedTextWriter> writer =
            vtkSmartPointer<vtkDelimitedTextWriter>::New();
    writer->SetFieldDelimiter(",");

    writer->SetInputData(tab);
    writer->SetFileName(filename.toStdString().c_str());
    writer->Update();

}

void
NMLayer::mapUniqueValues(void)
{
	if (mTableModel == 0)
	{
		NMErr(ctxNMLayer, << "Invalid attribute table");
		return;
	}

	// check attribute value type
	int validx = this->getColumnIndex(mLegendValueField);
	int descridx = validx;
	mLegendDescrField = mLegendValueField;
	if (validx < 0)
	{
		NMErr(ctxNMLayer, << "Value field not specified!");
		return;
	}
	if (descridx < 0) descridx = validx;

	const QModelIndex sampleIdx = mTableModel->index(0, validx);
	bool bNum = false;
	//QVariant::Type valType = mTableModel->data(sampleIdx).type();
	QVariant::Type valType = this->getColumnType(this->getColumnIndex(mLegendValueField));
	if (valType == QVariant::Double)
	{
		NMErr(ctxNMLayer, << "Continuous data types are not supported!");
		return;
	}
	else if (valType != QVariant::String)
	{
		bNum = true;
	}

	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();

	long ncells = 0;
	vtkAbstractArray* valar = 0;
	vtkUnsignedCharArray* hole = 0;
	if (mLayerType == NM_VECTOR_LAYER)
	{
		vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
		valar = dsa->GetAbstractArray(validx);
		ncells = valar->GetNumberOfTuples();
		mLookupTable->SetNumberOfTableValues(ncells);

		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
		if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
			hole = vtkUnsignedCharArray::SafeDownCast(dsa->GetArray("nm_hole"));
	}
	else
	{
        //ncells = mTableModel->rowCount(QModelIndex());
        //ncells = mTableModel-
        NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
        ncells = il->getRasterAttributeTable(1)->GetNumRows();
        //mLookupTable->SetNumberOfTableValues(ncells+2);
        mLookupTable->SetNumberOfTableValues(ncells);
	}

	// value index
	//this->mHashValueIndices.clear();
	this->mMapValueIndices.clear();

	bool bConvOk;
	qlonglong val;
	QString sVal;
	QModelIndex vi;
	vtkMath::RandomSeed(QTime::currentTime().msec());
	double rgba[4];
	for (int t=0; t < ncells; ++t)
	{
		if (hole && hole->GetValue(t))
		{
			mLookupTable->SetTableValue(t, rgba[0], rgba[1], rgba[2]);
			continue;
		}

		if (bNum)
		{
			if (valar)
			{
				val = valar->GetVariantValue(t).ToLongLong();
			}
			else
			{
				vi = mTableModel->index(t, validx);
				val = vi.data().toLongLong(&bConvOk);
			}

			sVal = QString(tr("%1")).arg(val);
		}
		else
		{
			if (valar)
			{
				sVal = valar->GetVariantValue(t).ToString().c_str();
			}
			else
			{
				vi = mTableModel->index(t, validx);
				sVal = vi.data().toString();
			}
		}

		QMap<QString, QVector<int> >::iterator it = this->mMapValueIndices.find(sVal);
		if (it == this->mMapValueIndices.end())
		{
			// generate a random triple of uchar values
			for (int i=0; i < 3; i++)
				rgba[i] = vtkMath::Random();
			rgba[3] = 1;

			// add the random colour to the mLookupTable
			// and store a reference into the LookupTable for the
			// given value;
			QVector<int> ids;
            //if (valar)
			{
				mLookupTable->SetTableValue(t, rgba[0], rgba[1], rgba[2], rgba[3]);
				ids.push_back(t);
				this->mMapValueIndices.insert(sVal, ids);
			}
            //			else
            //			{
            //				mLookupTable->SetTableValue(t+1, rgba[0], rgba[1], rgba[2], rgba[3]);
            //				ids.push_back(t+1);
            //				this->mMapValueIndices.insert(sVal, ids);
            //			}
		}
		else
		{
			mLookupTable->GetTableValue(it.value().at(0), rgba);
            //if (valar)
			{
				it.value().push_back(t);
				mLookupTable->SetTableValue(t, rgba[0], rgba[1], rgba[2], rgba[3]);
			}
            //			else
            //			{
            //				it.value().push_back(t+1);
            //				mLookupTable->SetTableValue(t+1, rgba[0], rgba[1], rgba[2], rgba[3]);
            //			}
		}
	}

//	if (valar == 0)
//	{
//		mLookupTable->SetTableValue(0, 0, 0, 0, 0);
//		mLookupTable->SetTableValue(ncells, 0, 0, 0, 0);
//		mLookupTable->SetTableRange(-1, ncells);
//		QVector<int> ov;
//		ov.push_back(0);
//		mMapValueIndices.insert("other", ov);
//	}
//	else
		mLookupTable->SetTableRange(0, ncells-1);

	mNumClasses = mMapValueIndices.size();
    //mNumLegendRows = mNumClasses + (valar ? 1 : 2);
    mNumLegendRows = mNumClasses + 1;//(valar ? 1 : 2);
}

void
NMLayer::setNodata(double val)
{
	if (mLegendType == NMLayer::NM_LEGEND_RAMP)
	{
		if (val < mLower || val > mUpper)
			mNodata = val;
		else
		{
			if (mLower == mUpper && val == mLower)
				mNodata = mNodata > mUpper ? mUpper+2*VALUE_MARGIN : mLower-2*VALUE_MARGIN;
			else if (val == mLower)
				mNodata = mLower-2*VALUE_MARGIN;
			else if (val == mUpper)
				mNodata = mUpper+2*VALUE_MARGIN;
		}
	}
	else
	{
		this->mNodata = val;
	}
	NMDebugAI(<< "mNodata = " << mNodata << std::endl);
}

void
NMLayer::setUpper(double val)
{
	this->mUpper = max(mLower, val);
	this->mLower = min(mLower, val);
	this->setNodata(mNodata);
}

void
NMLayer::setLower(double val)
{
	this->mUpper = max(mUpper, val);
	this->mLower = min(mUpper, val);
	this->setNodata(mNodata);
}

NMLayer::NMColourRamp
NMLayer::getColourRampFromStr(const QString rampStr)
{
	NMLayer::NMColourRamp ramp = NMLayer::NM_RAMP_GREY;

	     if (rampStr.compare("NM_RAMP_RAINBOW"      )==0) {ramp = NMLayer::NM_RAMP_RAINBOW       ;}
	else if (rampStr.compare("NM_RAMP_GREY"         )==0) {ramp = NMLayer::NM_RAMP_GREY          ;}
	else if (rampStr.compare("NM_RAMP_RED"          )==0) {ramp = NMLayer::NM_RAMP_RED           ;}
	else if (rampStr.compare("NM_RAMP_BLUE"         )==0) {ramp = NMLayer::NM_RAMP_BLUE          ;}
	else if (rampStr.compare("NM_RAMP_GREEN"        )==0) {ramp = NMLayer::NM_RAMP_GREEN         ;}
	else if (rampStr.compare("NM_RAMP_RED2BLUE"     )==0) {ramp = NMLayer::NM_RAMP_RED2BLUE      ;}
	else if (rampStr.compare("NM_RAMP_BLUE2RED"     )==0) {ramp = NMLayer::NM_RAMP_BLUE2RED      ;}
	else if (rampStr.compare("NM_RAMP_ALTITUDE"     )==0) {ramp = NMLayer::NM_RAMP_ALTITUDE      ;}
	else if (rampStr.compare("NM_RAMP_BLUE2RED_DIV" )==0) {ramp = NMLayer::NM_RAMP_BLUE2RED_DIV  ;}
	else if (rampStr.compare("NM_RAMP_GREEN2RED_DIV")==0) {ramp = NMLayer::NM_RAMP_GREEN2RED_DIV ;}
	else if (rampStr.compare("NM_RAMP_MANUAL"       )==0) {ramp = NMLayer::NM_RAMP_MANUAL        ;}

	return ramp;
}

//vtkSmartPointer<vtkColorTransferFunctionSpecialNodes>
vtkSmartPointer<vtkColorTransferFunction>
NMLayer::getColorTransferFunc(const NMColourRamp& ramp,
		const QList<double>& userNodes,
		const QList<QColor>& userColours,
		bool invertRamp)
{
	/* userNodes and userColours are synchronised
	 * and represent the following value for
	 * all supported colour ramp types
	 *
     * 0: lower
     * 1: upper
	 *
     * for NM_RAMP_MANUAL colour nodes are
     * interpreted as starting with the lowest
     * value at index 0 to the hightest value
     * at index userNodes.size()-1
	 */

    double lower, upper;
	std::vector<double> nodataclr, lowerclr, upperclr, lowermarclr, uppermarclr;
    if (userNodes.size() < 2)
	{
		lower = 0;
		upper = 1;
    }
	else
	{
        lower    = userNodes.at(0);
        upper    = userNodes.at(userNodes.size()-1);
	}

	if (invertRamp)
	{
		double t = lower;
		lower = upper;
		upper = t;
	}

    vtkSmartPointer<vtkDiscretizableColorTransferFunction> cf =
            vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();

    cf->SetNumberOfValues(256);
    cf->DiscretizeOn();

	switch (mColourRamp)
	{
	case NM_RAMP_RED:
		cf->AddRGBPoint(lower, 1.0, 1.0, 1.0);
		cf->AddRGBPoint(upper, 1.0, 0.0, 0.0);
		break;

	case NM_RAMP_BLUE:
		cf->AddRGBPoint(lower, 1.0, 1.0, 1.0);
		cf->AddRGBPoint(upper, 0.0, 0.0, 1.0);
		break;

	case NM_RAMP_GREEN:
		cf->AddRGBPoint(lower, 1.0, 1.0, 1.0);
		cf->AddRGBPoint(upper, 0.0, 1.0, 0.0);
		break;

	case NM_RAMP_RED2BLUE:
		cf->AddRGBPoint(lower, 1.0, 0.0, 0.0);
		cf->AddRGBPoint(upper, 0.0, 0.0, 1.0);
		cf->SetColorSpaceToRGB();
		break;

	case NM_RAMP_BLUE2RED:
		cf->AddRGBPoint(lower, 0.0, 0.0, 1.0);
		cf->AddRGBPoint(upper, 1.0, 0.0, 0.0);
		cf->SetColorSpaceToRGB();
		break;

	case NM_RAMP_BLUE2RED_DIV:
		cf->AddRGBPoint(lower, 0.0, 0.0, 1.0);
		cf->AddRGBPoint(upper, 1.0, 0.0, 0.0);
		cf->SetColorSpaceToDiverging();
		break;

	case NM_RAMP_GREEN2RED_DIV:
		cf->AddRGBPoint(lower, 0.0, 1.0, 0.0);
		cf->AddRGBPoint(upper, 1.0, 0.0, 0.0);
		cf->SetColorSpaceToDiverging();
		break;

	case NM_RAMP_RAINBOW:
		{
			double d = abs(upper - lower);
			double s = d/5.0;
			for (int i=0, hue=300; i < 6; ++i, hue-=60)
			{
				if (invertRamp)
					cf->AddHSVPoint(lower-((double)i*s), (double)hue/360.0, 1.0, 1.0);
				else
					cf->AddHSVPoint(lower+((double)i*s), (double)hue/360.0, 1.0, 1.0);
			}
			cf->SetColorSpaceToHSV();
		}
		break;

	case NM_RAMP_MANUAL:
		{
            if (userNodes.size() == userColours.size())
            {
                for (int c=0; c < userNodes.size(); ++c)
                {
                    cf->AddRGBPoint(
                            userNodes.at(c),
                            userColours.at(c).redF(),
                            userColours.at(c).greenF(),
                            userColours.at(c).blueF()
                            );
                }
            }
		}
		break;

	case NM_RAMP_ALTITUDE:
        {
            QList<QColor> uc;
            uc << QColor(252,255,73)
               << QColor(0,111,1)
               << QColor(255,153,0)
               << QColor(133,76,7)
               << QColor(57,34,4)
               << QColor(255,255,255)
               << QColor(74,243,255)
               << QColor(255,124,235);

            const int ncls = uc.size();
            double range = abs(upper-lower);
            // note: ncls, but ncls-1 steps up
            double step  = range/((double)ncls-1.0);

            for (int i=0; i < ncls; ++i)
            {
                const double f= invertRamp ? -1.0 : 1.0;
                cf->AddRGBPoint(lower+(double)i*step*f, uc.at(i).redF(),
                                                      uc.at(i).greenF(),
                                                      uc.at(i).blueF());
            }
        }
        break;

	case NM_RAMP_GREY:
	default:
		cf->AddRGBPoint(lower, 0.0, 0.0, 0.0);
		cf->AddRGBPoint(upper, 1.0, 1.0, 1.0);
		break;
	}

	cf->Build();

	return cf;
}

void
NMLayer::mapValueRamp(void)
{
	NMDebugCtx(ctxNMLayer, << "...");

	mNumClasses = 256;
	mNumLegendRows = 4;

	// just in case we jut now ran through initiateLegend, we
	// still have to calc the value field statistics
	//this->setLegendValueField("Pixel Values");

	// ensure value and display field are the same
	//this->setLegendDescrField("Pixel Values");

    QList<QColor> userColours;
	QList<double> userNodes;

    userNodes << mLower << mUpper;
	mClrFunc = this->getColorTransferFunc(mColourRamp,
			userNodes, userColours);

    if (mLookupTable.GetPointer() != 0)
    {
        mLookupTable = 0;
    }

    //  fill lookup table based on the legend value field
    if (this->mLayerType == NM_VECTOR_LAYER)
	{
        mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
        // prepare look-up table
        long nrows = mTableModel->rowCount(QModelIndex());

		vtkUnsignedCharArray* hole = 0;

        NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
        vtkQtEditableTableModelAdapter* model =
                static_cast<vtkQtEditableTableModelAdapter*>(mTableModel);
        vtkTable* tab = vtkTable::SafeDownCast(model->GetVTKDataObject());

        if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
        {
            hole = vtkUnsignedCharArray::SafeDownCast(tab->GetColumnByName("nm_hole"));
        }

        mLookupTable->SetNumberOfTableValues(nrows);
        mLookupTable->SetTableRange(0, nrows-1);

		const int cidx = this->getColumnIndex(mLegendValueField);
		QVariant::Type coltype = this->getColumnType(cidx);

		double fc[3];
		double value;
		bool bok;
        for (int row=0; row < nrows; ++row)
		{
			if (hole && hole->GetValue(row))
			{
				mLookupTable->SetTableValue(row, fc[0], fc[1], fc[2], 1);
				continue;
			}

            const QModelIndex mi = mTableModel->index(row, cidx, QModelIndex());
			value = mi.data().toDouble(&bok);

            if (value < mLower)
            {
                fc[0] = mClrLowerMar.redF();
                fc[1] = mClrLowerMar.greenF();
                fc[2] = mClrLowerMar.blueF();
                fc[3] = 1;
            }
            else if (value > mUpper)
            {
                fc[0] = mClrUpperMar.redF();
                fc[1] = mClrUpperMar.greenF();
                fc[2] = mClrUpperMar.blueF();
                fc[3] = 1;
            }
            else
            {
                mClrFunc->GetColor(value, fc);
            }
            mLookupTable->SetTableValue(row, fc[0], fc[1], fc[2], 1);
		}
	}
    else // NM_IMAGE_LAYER
    {
        vtkSmartPointer<NMVtkLookupTable> lut = vtkSmartPointer<NMVtkLookupTable>::New();
        lut->SetNumberOfColors(mNumClasses);

        double* rgb;
        double lower = mLower;
        double upper = mUpper;
        double range = upper - lower;
        double step = range/255.0;

        for (int i=0; i < mNumClasses; ++i)
        {
            double incr = ((double)i * step);
            double sample = lower + incr;
            //double pos = abs(incr/range);

            rgb = mClrFunc->GetColor(sample);
            lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1);
        }
        lut->setLowerUpperClrOn();

        unsigned char clr[4];
        clr[0] = mClrLowerMar.red();
        clr[1] = mClrLowerMar.green();
        clr[2] = mClrLowerMar.blue();
        clr[3] = mClrLowerMar.alpha();
        lut->setLowerClr(lower, clr);

        clr[0] = mClrUpperMar.red();
        clr[1] = mClrUpperMar.green();
        clr[2] = mClrUpperMar.blue();
        clr[3] = mClrUpperMar.alpha();
        lut->setUpperClr(upper, clr);
        lut->SetTableRange(lower, upper);
        lut->SetIndexedLookup(0);
        mLookupTable = lut;
    }

	NMDebugCtx(ctxNMLayer, << "done!");
}

void
NMLayer::mapValueClasses(void)
{

}

bool
NMLayer::hasColourTable(void)
{
	if (mTableModel == 0)
		return false;

	mHasClrTab = false;
	for (int i=0; i < 4; ++i)
		mClrTabIdx[i] = -1;

	// -------------------------------------------------------------------------------------
	// ANALYSE ATTRIBUTE TABLE

	int ncols = this->mTableModel->columnCount(QModelIndex());
	for (int i=0; i < ncols; ++i)
	{
		const QModelIndex ci = mTableModel->index(0, i, QModelIndex());
		if (mTableModel->data(ci, Qt::DisplayRole).type() != QVariant::String)
		{
			QString fieldName = mTableModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();

			if (fieldName.compare("red", Qt::CaseInsensitive) == 0)
				mClrTabIdx[0] = i;
			else if (fieldName.compare("green", Qt::CaseInsensitive) == 0)
				mClrTabIdx[1] = i;
			else if (fieldName.compare("blue", Qt::CaseInsensitive) == 0)
				mClrTabIdx[2] = i;
			else if (	fieldName.compare("alpha", Qt::CaseInsensitive) == 0
					 || fieldName.compare("opacity", Qt::CaseInsensitive) == 0
					)
				mClrTabIdx[3] = i;
		}
	}

	if (	mClrTabIdx[0] >= 0
		&&  mClrTabIdx[1] >= 0
		&&  mClrTabIdx[2] >= 0
		&&  mClrTabIdx[3] >= 0
	   )
	{
		mHasClrTab = true;
	}

	return mHasClrTab;

}

void
NMLayer::mapColourTable(void)
{
	NMDebugCtx(ctxNMLayer, << "...");

	if (mTableModel == 0 || !this->hasColourTable())
	{
		NMErr(ctxNMLayer, << "Invalid attribute table");
		return;
	}

	mLegendValueField = tr("Colour Table");

	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	long ncells = 0;
	bool bvect = false;
	bool bfloat = false;
	vtkUnsignedCharArray* hole = 0;
	vtkDataArray* ar=0;
	vtkDataArray* ag=0;
	vtkDataArray* ab=0;
	vtkDataArray* aa=0;
	if (mLayerType == NM_VECTOR_LAYER)
	{
		bvect = true;
		vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
		ar = dsa->GetArray(mClrTabIdx[0]);
		ag = dsa->GetArray(mClrTabIdx[1]);
		ab = dsa->GetArray(mClrTabIdx[2]);
		aa = dsa->GetArray(mClrTabIdx[3]);
		if (	ar->GetDataType() == VTK_DOUBLE
			||  ar->GetDataType() == VTK_FLOAT)
		{
			bfloat = true;
		}

		ncells = aa->GetNumberOfTuples();
		mLookupTable->SetNumberOfTableValues(ncells);

		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
		if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
			hole = vtkUnsignedCharArray::SafeDownCast(dsa->GetArray("nm_hole"));
	}
	else
	{
		ncells = mTableModel->rowCount(QModelIndex());
		mLookupTable->SetNumberOfTableValues(ncells+2);
	}

	bool bok;
	double fc[4];
	for (long row=0; row < ncells; ++row)
	{
		if (bvect)
		{
			if (hole && hole->GetValue(row))
			{
				mLookupTable->SetTableValue(row, fc);
				continue;
			}

			fc[0] = ar->GetVariantValue(row).ToDouble();
			fc[1] = ag->GetVariantValue(row).ToDouble();
			fc[2] = ab->GetVariantValue(row).ToDouble();
			fc[3] = aa->GetVariantValue(row).ToDouble();

			if (!bfloat)
			{
				fc[0] /= 255.0;
				fc[1] /= 255.0;
				fc[2] /= 255.0;
				fc[3] /= 255.0;
			}
			mLookupTable->SetTableValue(row, fc);
		}
		else
		{
			const QModelIndex mired   = mTableModel->index(row, mClrTabIdx[0], QModelIndex());
			const QModelIndex migreen = mTableModel->index(row, mClrTabIdx[1], QModelIndex());
			const QModelIndex miblue  = mTableModel->index(row, mClrTabIdx[2], QModelIndex());
			const QModelIndex mialpha = mTableModel->index(row, mClrTabIdx[3], QModelIndex());

			fc[0] = mTableModel->data(mired, Qt::DisplayRole).toDouble(&bok);
			fc[1] = mTableModel->data(migreen, Qt::DisplayRole).toDouble(&bok);;
			fc[2] = mTableModel->data(miblue, Qt::DisplayRole).toDouble(&bok);;
			fc[3] = mTableModel->data(mialpha, Qt::DisplayRole).toDouble(&bok);;
			if (mTableModel->data(mired, Qt::DisplayRole).type() != QVariant::Double)
			{
				fc[0] /= 255.0;
				fc[1] /= 255.0;
				fc[2] /= 255.0;
				fc[3] /= 255.0;
			}
			mLookupTable->SetTableValue(row+1, fc);
		}
	}

	// everything outside the table index range is set to transparent
	if (!bvect)
	{
		mLookupTable->SetTableValue(0, 0, 0, 0, 0);
		mLookupTable->SetTableValue(ncells, 0, 0, 0, 0);
		mLookupTable->SetTableRange(-1, ncells);
		mNumLegendRows = ncells + 2;
	}
	else
	{
		mLookupTable->SetTableRange(0, ncells-1);
		mNumLegendRows = ncells+1;
	}

	NMDebugCtx(ctxNMLayer, << "done!");
}


bool  NMLayer::getLegendColour(const int legendRow, double* rgba)
{
	//NMDebugCtx(ctxNMLayer, << "...");

	// check whether row is valid or not
	if (legendRow < 1 || legendRow >= mNumLegendRows) //this->mLegendInfo->GetNumberOfRows())
		return false;

	switch(mLegendType)
	{
	case NM_LEGEND_CLRTAB:
		{
			mLookupTable->GetTableValue(legendRow-1, rgba);
		}
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				{
					if (legendRow-1 < mNumLegendRows)
					{
						QList<QString> keys = mMapValueIndices.keys();
						if (legendRow-1 < keys.size())
						{
							int tabidx = mMapValueIndices.value(keys[legendRow-1]).at(0);
							mLookupTable->GetTableValue(tabidx, rgba);
						}
					}
				}
				break;


			default:
				break;
			}
		}
		break;

    case NM_LEGEND_RGB:
	case NM_LEGEND_RAMP:
        if (mLegendValueField != "RGB")
        {
            if (legendRow == 3)
            {
                rgba[0] = mClrLowerMar.redF();
                rgba[1] = mClrLowerMar.greenF();
                rgba[2] = mClrLowerMar.blueF();
                rgba[3] = mClrLowerMar.alphaF();
            }
            else if (legendRow == 1)
            {
                rgba[0] = mClrUpperMar.redF();
                rgba[1] = mClrUpperMar.greenF();
                rgba[2] = mClrUpperMar.blueF();
                rgba[3] = mClrUpperMar.alphaF();
            }
        }
        else
        {
            const int row = legendRow;
            switch(row)
            {
            case 1:
                rgba[0] = 1.0;
                rgba[1] = 0.0;
                rgba[2] = 0.0;
                rgba[3] = 1.0;
                break;
            case 2:
                rgba[0] = 0.0;
                rgba[1] = 1.0;
                rgba[2] = 0.0;
                rgba[3] = 1.0;
                break;
            case 3:
                rgba[0] = 0.0;
                rgba[1] = 0.0;
                rgba[2] = 1.0;
                rgba[3] = 1.0;
                break;
            }
        }
		break;

	case NM_LEGEND_SINGLESYMBOL:
		{
			if (mLayerType == NMLayer::NM_IMAGE_LAYER)
			{
                if (legendRow == 1)
                {
                    if (mLookupTable->GetTableRange()[1] == mNodata)
                        mLookupTable->GetTableValue(1, rgba);
                    else
                        mLookupTable->GetTableValue(0, rgba);
                }
                else
				{
					if (mLookupTable->GetTableRange()[1] == mNodata)
						mLookupTable->GetTableValue(0, rgba);
					else
						mLookupTable->GetTableValue(1, rgba);
				}
			}
			else
			{
				mLookupTable->GetTableValue(legendRow-1, rgba);
			}
		}
		break;

	default:
		break;
	}

	//NMDebugCtx(ctxNMLayer, << "done!");
	return true;
}

void
NMLayer::setLegendValueField(QString field)
{
	if (field == "Colour Table")
	{
		return;
	}

	if (field.compare(mLegendValueField, Qt::CaseInsensitive) != 0)
		this->mLegendValueField = field;
	else
		return;

    NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
    if (il)
    {
        if (field == "Pixel Values")
        {
            std::vector<double> istats = il->getWindowStatistics();
            int i=0;
            for (; i < istats.size(); ++i)
            {
                mStats[i] = istats[i];
            }
            for (; i < 7; ++i)
            {
                mStats[i] = -9999;
            }
            mLegendDescrField = field;
            setLower(mStats[0]);
            setUpper(mStats[1]);
            //            //if (mStats[0] == mNodata)
            //                setLower(mStats[0] + 3*VALUE_MARGIN);
            //            else
            //                setLower(mStats[0]);

            //            if (mStats[1] == mNodata)
            //                setUpper(mStats[1] - 1);
            //            else
            //                setUpper(mStats[1]);
        }
        else if (il->getColumnIndex(mLegendValueField) != -1)
        {
            il->setUpdateScalars();
        }
	}
	else
	{
		NMTableCalculator calc(mTableModel);
		QList<int> raw2source;
		NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
		if (	vl != 0
			&&  vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT
		   )

		{
			if (mTableView == 0)
			{
                vtkQtEditableTableModelAdapter* vtkmodel =
                        qobject_cast<vtkQtEditableTableModelAdapter*>(mTableModel);
				vtkTable* tab  = 0;
				if (vtkmodel != 0)
				{
					tab = vtkTable::SafeDownCast(vtkmodel->GetVTKDataObject());
				}
				vtkUnsignedCharArray* hole = 0;
				if (tab != 0)
				{
					hole = vtkUnsignedCharArray::SafeDownCast(tab->GetColumnByName("nm_hole"));
				}
				if (hole != 0)
				{
					for (long row=0; row < tab->GetNumberOfRows(); ++row)
					{
						if (hole->GetValue(row))
							raw2source << -1;
						else
							raw2source << row;
					}
					calc.setRaw2Source(&raw2source);
				}
			}
			else
			{
				calc.setRaw2Source(const_cast<QList<int>* >(mTableView->getRaw2Source()));
			}
		}

		std::vector<double> colstats = calc.calcColumnStats(mLegendValueField);
		for (int s=0; s < colstats.size(); ++s)
			mStats[s] = colstats[s];

		setLower(mStats[0]);
		setUpper(mStats[1]);

	}

	this->updateMapping();
}

void
NMLayer::setLegendColour(const int legendRow, double* rgba)
{
	//NMDebugCtx(ctxNMLayer, << "...");

	// check whether row is valid or not
	if (legendRow < 1 || legendRow >= mNumLegendRows)
		return;

	switch(mLegendType)
	{
	case NM_LEGEND_CLRTAB:
		{
			mLookupTable->SetTableValue(legendRow-1, rgba);
		}
		break;

	case NM_LEGEND_RAMP:
		if (legendRow == 1)
		{
			mClrUpperMar.setRedF(rgba[0]);
			mClrUpperMar.setGreenF(rgba[1]);
			mClrUpperMar.setBlueF(rgba[2]);
			mClrUpperMar.setAlphaF(rgba[3]);
		}
		else if (legendRow == 3)
		{
			mClrLowerMar.setRedF(rgba[0]);
			mClrLowerMar.setGreenF(rgba[1]);
			mClrLowerMar.setBlueF(rgba[2]);
			mClrLowerMar.setAlphaF(rgba[3]);
		}
		this->updateMapping();
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				{
					if (legendRow-1 < mNumLegendRows)
					{
						QList<QString> keys = mMapValueIndices.keys();
						if (legendRow-1 < keys.size())
						{
							// the ugly part is that we have to set the
							// new colour for each element with the
							// given value ...
							const QString key = keys[legendRow-1];
							QVector<int> ids = mMapValueIndices.value(key);
							for (int i=0; i < ids.size(); ++ i)
							{
								int ti = ids.at(i);
								mLookupTable->SetTableValue(ti, rgba);

								// the even uglier part is that we have to
								// give any possbile hole-rings in a polygon
								// the same colour as the polygon itself ...
								if (mLayerType == NMLayer::NM_VECTOR_LAYER)
								{
									NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
									if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
									{
										vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
										vtkUnsignedCharArray* hole = vtkUnsignedCharArray::SafeDownCast(
												dsa->GetArray("nm_hole"));

										//vtkOGRLayerMapper* contourMapper =
										//		const_cast<vtkOGRLayerMapper*>(vl->getContourMapper());
										//vtkLookupTable* lut = vtkLookupTable::SafeDownCast(
										//		contourMapper->GetLookupTable());
										//lut->SetTableValue(ti, 0, 0, 0, 0);

										++ti;
										while (		ti < mLookupTable->GetNumberOfTableValues()
												&&  hole->GetValue(ti)
											  )
										{
											mLookupTable->SetTableValue(ti, rgba);
											//lut->SetTableValue(ti, 0, 0, 0, rgba[3]);
											++ti;
										}
									}
								}
							}
						}
					}
				}
				break;


			default:
				break;
			}
		}
		break;

	case NM_LEGEND_SINGLESYMBOL:
		{
			if (mLayerType == NMLayer::NM_IMAGE_LAYER)
			{
                if (legendRow == 1)
                {
                    mLookupTable->SetTableValue(0, rgba);
                }
                else
				{
                    if (mTableModel)
                    {
                        for (int i=1; i <= mTableModel->rowCount(); ++i)
                        {
                            mLookupTable->SetTableValue(i, rgba);
                        }
                    }
                    else
                    {
                        mLookupTable->SetTableValue(1, rgba);
                    }
				}
			}
			else
			{
				vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
				vtkLongArray* nmids = vtkLongArray::SafeDownCast(dsAttr->GetArray("nm_id"));
				long ncells = nmids->GetNumberOfTuples();
				for (long r=0; r < ncells; ++r)
				{
					mLookupTable->SetTableValue(r, rgba);
				}
			}
		}
		break;

	default:
		break;
	}

	emit legendChanged(this);
	emit visibilityChanged(this);

	//NMDebugCtx(ctxNMLayer, << "done!");
	return;
}

std::vector<double>
NMLayer::getValueFieldStatistics()
{
	NMDebugCtx(ctxNMLayer, << "...");

	std::vector<double> stats;

    int colidx = this->getColumnIndex(mLegendValueField);
    QVariant::Type type = this->getColumnType(colidx);

    if (    type == QVariant::Invalid
        ||  (   type != QVariant::Int
             && type != QVariant::Double
             && type != QVariant::LongLong
            )
        ||  mLegendValueField == "Colour Table"
        ||  mLegendValueField == "Pixel Values"
       )
    {
        NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
        if (il)
        {
            vtkImageData* vtkImg = vtkImageData::SafeDownCast(
                        const_cast<vtkDataSet*>(il->getDataSet()));
            stats = il->getWindowStatistics();
            for (int i=stats.size()-1; i < 7; ++i)
            {
                stats[i] = -9999;
            }
        }

        return stats;
    }


    // determine non-hole features in case we've got a
    // polygon layer here
    QList<int> raw2source;
    if (this->getLayerType() == NMLayer::NM_VECTOR_LAYER)
    {
        NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
        if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
        {
            raw2source.reserve(mTableModel->rowCount());
            int holeidx = this->getColumnIndex("nm_hole");
            bool bok;
            for (long row=0; row < mTableModel->rowCount(); ++row)
            {
                const QModelIndex ridx = mTableModel->index(row, holeidx, QModelIndex());
                int val = mTableModel->data(ridx, Qt::DisplayRole).toInt(&bok);
                if (val < 0)
                {
                    raw2source << -1;
                }
                else
                {
                    raw2source << row;
                }
            }
        }
    }

    NMTableCalculator calc(mTableModel, this);
    if (raw2source.size() > 0)
        calc.setRaw2Source(&raw2source);

    stats = calc.calcColumnStats(mLegendValueField);

    // copy stats: min, max, mean, median, sample size, sdev
    for (int i=0; i < stats.size(); ++i)
        mStats[i] = stats[i];

	NMDebugCtx(ctxNMLayer, << "done!");
	return stats;
}

QIcon NMLayer::getLegendIcon(const int legendRow)
{
	QIcon icon;

	if (mLegendType == NM_LEGEND_RAMP && legendRow == NM_LEGEND_RAMP_ROW)
	{
		icon = this->getColourRampIcon();
	}
	else
	{
		double rgba[4] = {0,0,0,0};
		this->getLegendColour(legendRow, rgba);
		QColor clr;
		clr.setRgbF(rgba[0], rgba[1], rgba[2], rgba[3]);

		QPixmap pix(32,32);
		if (mLayerType == NM_VECTOR_LAYER)
		{
			NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
			QColor cclr = vl->getContourColour();
			//cclr.setAlphaF(clr.alphaF());

			QPainter painter(&pix);
			QRect inner = pix.rect();
			QRect outer = pix.rect();
			//outer.adjust(0, 0, 1, 1);
			inner.adjust(1,1,-1,-1);
			//painter.setPen(cclr);
			//painter.drawRect(outer);
			painter.fillRect(pix.rect(), cclr);
			painter.fillRect(inner, clr);
		}
		else
		{
			pix.fill(clr);
		}

		icon = QIcon(pix);
	}

	return icon;
}

QIcon NMLayer::getColourRampIcon()
{
	QIcon rampIcon;

	QPixmap pix(18, 60);
	QPainter p(&pix);

	QLinearGradient ramp;
	ramp.setCoordinateMode(QGradient::StretchToDeviceMode);
	ramp.setStart(0.5, 1.0);
	ramp.setFinalStop(0.5, 0.0);

	// here, we just sample the colour transfer function 256 times
	// to build the colour ramp
	double* rgb;
	double lower = mLower;// = min(mLower, mUpper);
	double upper = mUpper; //max(mLower, mUpper);
	double range = upper - lower;
	double step = range/255.0;
	QColor clr;

	for (int i=0; i < 256; ++i)
	{
		double incr = ((double)i * step);
		double sample = lower + incr;
		double pos = abs(incr/range);
        if (pos < 0 || pos > 1)
            continue;

		rgb = mClrFunc->GetColor(sample);
		clr.setRedF((qreal)rgb[0]);
		clr.setGreenF((qreal)rgb[1]);
		clr.setBlueF((qreal)rgb[2]);
		ramp.setColorAt(pos, clr);
	}

	p.fillRect(pix.rect(), ramp);

	rampIcon = QIcon(pix);
	return rampIcon;
}

int NMLayer::getLegendItemCount(void)
{
	return this->mNumLegendRows;
}

QString  NMLayer::getLegendName(const int legendRow)
{
	QString name;

	// check whether row is valid or not
	if (legendRow < 0 || legendRow >= mNumLegendRows)//this->mLegendInfo->GetNumberOfRows())
		return tr("");

	if (legendRow == 0)
	{
		return this->mLegendDescrField;
	}

	switch(mLegendType)
	{
	case NM_LEGEND_CLRTAB:
		{
			int colidx = this->getColumnIndex(mLegendDescrField);
			if (mLayerType == NMLayer::NM_VECTOR_LAYER)
			{
				const QModelIndex mi = mTableModel->index(legendRow-1, colidx, QModelIndex());
				name = mTableModel->data(mi, Qt::DisplayRole).toString();
			}
			else
			{
				if (legendRow == 1)
				{
					name = tr("other");
				}
				else
				{
					const QModelIndex mi = mTableModel->index(legendRow-2, colidx, QModelIndex());
					name = mTableModel->data(mi, Qt::DisplayRole).toString();
				}
			}
		}
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				{
					QList<QString> keys = mMapValueIndices.keys();
					if (legendRow-1 < keys.size())
					{
						name = keys[legendRow-1];
					}
				}
				break;


			default:
				break;
			}
		}
		break;

	case NM_LEGEND_SINGLESYMBOL:
		name = tr("Valid Values");
		if (mLayerType == NMLayer::NM_IMAGE_LAYER)
		{
			if (legendRow == 1)
				name = tr("Nodata");
		}
		break;

    case NM_LEGEND_RGB:
    case NM_LEGEND_RAMP:
		{
			//name = "";
            if (mLegendValueField != "RGB")
            {
                switch (legendRow)
                {
                    //case 1:	name = tr("Nodata"); break;
                    case 1: name = tr("> Upper"); break;
                    case 3: name = tr("< Lower"); break;
                    default: name = tr(""); break;
                }
            }
            else
            {
                NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
                std::vector<int> bm;
                if (il)
                {
                    bm = il->getBandMap();
                }
                switch (legendRow)
                {
                    case 1:
                        name = QString(tr("Band #%1")).arg(bm.size() >= 1 ? bm[0] : 1);
                        break;
                    case 2:
                        name = QString(tr("Band #%1")).arg(bm.size() >= 2 ? bm[1] : 2);
                        break;
                    case 3:
                        name = QString(tr("Band #%1")).arg(bm.size() >= 3 ? bm[2] : 3);
                        break;
                    default: name = tr(""); break;
                }
            }
		}
		break;

	default:
		break;
	}

	return name;

}


double NMLayer::getArea(void)
{
	if (this->mTotalArea != -1)
		return this->mTotalArea;

	if (this->mDataSet.GetPointer() == 0)
	{
		this->mTotalArea = -1;
		return this->mTotalArea;
	}

	this->mTotalArea = (this->mBBox[1] - this->mBBox[0]) *
					   (this->mBBox[3] - this->mBBox[2]);

	return this->mTotalArea;
}

int NMLayer::setLayerPos(int pos)
{
	if (pos < 0)
		return -1;

	int oldpos = this->mRenderer->GetLayer()-1;
	// account for the background renderer -> so we add one
	// to the desired layer position
	this->mRenderer->SetLayer(pos+1);

	return oldpos;
}

int NMLayer::getLayerPos()
{
	// we account for the presence of the background renderer, so
	// substract that one from the actual layer position
	return this->mRenderer->GetLayer()-1;
}

void NMLayer::removeFromMap(void)
{
	if (this->mActor != 0)
	{
		this->mActor->SetVisibility(0);
		if (this->mRenderer != 0)
		{
			this->mRenderer->RemoveActor(this->mActor);
			if (this->mRenderWindow != 0)
				this->mRenderWindow->RemoveRenderer(this->mRenderer);
		}
	}
}

void NMLayer::setDataSet(vtkDataSet* dataset)
{
	if (!dataset)
	{
		NMErr(ctxNMLayer, << "dataset is NULL!");
		return;
	}

	this->mDataSet = dataset;
	this->mDataSet->GetBounds(this->mBBox);
	this->mTotalArea = -1;
}

const vtkRenderer* NMLayer::getRenderer(void)
{
	return this->mRenderer;
}

//const vtkDataSet* NMLayer::getDataSet(void)
//{
//	return this->mDataSet;
//}

const vtkProp3D* NMLayer::getActor(void)
{
	return this->mActor;
}

const vtkAbstractMapper* NMLayer::getMapper(void)
{
	return this->mMapper;
}

NMLayer::NMLayerType NMLayer::getLayerType(void)
{
	return this->mLayerType;
}

void NMLayer::setBBox(double bbox[6])
{
	if (bbox[0] > bbox[1] ||
		bbox[2] > bbox[3] ||
		bbox[4] > bbox[5])
	{
		NMErr(ctxNMLayer, <<
				"invalid bounding box!");
		return;
	}

	for (int i=0; i < 6; i++)
		this->mBBox[i] = bbox[i];
}

const double* NMLayer::getBBox(void)
{
	return this->mBBox;
}

void NMLayer::getBBox(double bbox[6])
{
	if (!bbox)
		return;

	for (int i=0; i < 6; i++)
		bbox[i] = this->mBBox[i];
}

void NMLayer::setVisible(bool visible)
{
	///if (this->mDataSet == 0)
	if (this->mRenderer.GetPointer() == 0)
		return;

	if (this->mIsVisible != visible)
	{
		if (this->mActor != 0)
			this->mActor->SetVisibility(visible);

		this->mIsVisible = visible;
		switch(this->mLayerType)
		{
		case NMLayer::NM_VECTOR_LAYER:
			this->mLayerIcon = mIsVisible ?
					QIcon(":vector_layer.png") : QIcon(":vector_layer_invisible.png");
			break;
		case NMLayer::NM_IMAGE_LAYER:
			this->mLayerIcon = mIsVisible ?
					QIcon(":image_layer.png") : QIcon(":image_layer_invisible.png");
			break;
		default:
			break;
		}

		emit visibilityChanged(this);
	}
}

bool NMLayer::isVisible(void)
{
	return this->mIsVisible;
}

void NMLayer::setSelectable(bool selectable)
{
	//if (this->mDataSet == 0)
	//	return;

	if (this->mIsSelectable != selectable)
	{
		this->mIsSelectable = selectable;
        if (!selectable && this->mSelectionModel != 0)
			this->mSelectionModel->clearSelection();
		emit selectabilityChanged(selectable);
	}
}

QIcon NMLayer::getLayerIcon(void)
{
	QIcon ic;
	switch(mLayerType)
	{
	case NM_VECTOR_LAYER:
		ic = mIsVisible ? QIcon(":vector_layer.png") : QIcon(":vector_layer_invisible.png");
		break;
	case NM_IMAGE_LAYER:
		ic = mIsVisible ? QIcon(":image_layer.png") : QIcon(":image_layer_invisible.png");
		break;
	default:
		ic = mLayerIcon;
		break;
	}
	return ic;
}

QImage NMLayer::getLayerIconAsImage(void)
{
	QImage img;
	switch(mLayerType)
	{
	case NM_VECTOR_LAYER:
		img = mIsVisible ? QImage(":vector_layer.png") : QImage(":vector_layer_invisible.png");
		break;
	case NM_IMAGE_LAYER:
		img = mIsVisible ? QImage(":image_layer.png") : QImage(":image_layer_invisible.png");
		break;
	default:
		break;
	}
	return img;
}

bool NMLayer::isSelectable(void)
{
	return this->mIsSelectable;
}

bool NMLayer::isVectorLayer(void)
{
	if (this->mLayerType == NMLayer::NM_VECTOR_LAYER)
		return true;

	return false;
}

bool NMLayer::isImageLayer(void)
{
	if (this->mLayerType == NMLayer::NM_IMAGE_LAYER)
		return true;

	return false;
}

void NMLayer::showAttributeTable(void)
{
    if (this->mTableView == 0 && this->mSqlTableView == 0)
		this->createTableView();

	if (this->mTableView != 0)
	{
		this->mTableView->show();
		this->mTableView->update();
    }

    if (this->mSqlTableView != 0)
    {
        this->mSqlTableView->show();
        this->mSqlTableView->update();
    }
}

void NMLayer::createTableView(void)
{
	// implemented in subclasses
}

int NMLayer::updateAttributeTable(void)
{
	// attribute table from changes in data set
	// subclasses to implement
	return 0;
}

//void NMLayer::updateDataSet(QStringList& slAlteredColumns,
//		QStringList& slDeletedColumns)
//{
//	// update data set from changes in attribute table
//	// subclasses to implement
//}

void
NMLayer::mapExtentChanged(void)
{
    // sub-classes who care to implement
}

void
NMLayer::disconnectTableSel(void)
{
	disconnect(mTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
			this, SLOT(tableDataChanged(const QModelIndex &, const QModelIndex &)));
	disconnect(mTableModel, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
			this, SLOT(tableColumnsInserted(const QModelIndex &, int, int)));
	disconnect(mTableModel, SIGNAL(columnsRemoved(const QModelIndex &, int , int)),
			this, SLOT(tableColumnsRemoved(const QModelIndex &, int, int)));
	disconnect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
	//disconnect(mTableView, SIGNAL(notifyLastClickedRow(long)), this, SIGNAL(notifyLastClickedRow(long)));
}

void
NMLayer::connectTableSel(void)
{
	connect(mTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
		this, SLOT(tableDataChanged(const QModelIndex &, const QModelIndex &)));
	connect(mTableModel, SIGNAL(columnsInserted(const QModelIndex &, int, int)),
		this, SLOT(tableColumnsInserted(const QModelIndex &, int, int)));
	connect(mTableModel, SIGNAL(columnsRemoved(const QModelIndex &, int , int)),
		this, SLOT(tableColumnsRemoved(const QModelIndex &, int, int)));
	connect(mSelectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
	//connect(mTableView, SIGNAL(notifyLastClickedRow(long)), this, SIGNAL(notifyLastClickedRow(long)));
}

void
NMLayer::forwardLastClickedRowSignal(long cellID)
{
	emit notifyLastClickedRow(this, cellID);
}

const QAbstractItemModel*
NMLayer::getTable(void)
{
	// subclass to implement
	if (mTableModel == 0)
		this->updateAttributeTable();

	return this->mTableModel;
}

const QItemSelection
NMLayer::getSelection(void)
{
	return this->mSelectionModel->selection();
}

void NMLayer::writeDataSet()
{
	// subclass  to implement
	// and call this method prior to
	// re-implementation;
	this->mHasChanged = false;
}

void
NMLayer::setIsSelected(bool sel)
{
    if (sel != this->mIsSelected)
    {
        this->mIsSelected = sel;
        emit IsSelectedChanged(sel);
    }
}

bool NMLayer::isInteractive(void)
{
    bool interactive = false;
    if (this->mRenderer != 0)
    {
        interactive = this->mRenderer->GetInteractive();
    }

    return interactive;
}

void NMLayer::selectedLayerChanged(const NMLayer* layer)
{
	NMLayer* l = const_cast<NMLayer*>(layer);
	if (l == this)
	{
        this->mRenderer->SetInteractive(1);
	}
	else
	{
		this->mRenderer->SetInteractive(0);
	}
}

double NMLayer::getLegendItemUpperValue(const int legendRow)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return -9;
	}

	vtkDoubleArray* range = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("range"));

	double r[2];
	range->GetTuple(legendRow, r);

	return r[1];
}


double NMLayer::getLegendItemLowerValue(const int legendRow)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return -9;
	}

	vtkDoubleArray* range = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("range"));

	double r[2];
	range->GetTuple(legendRow, r);

	return r[0];
}

bool NMLayer::getLegendItemRange(const int legendRow, double* range)
{
	if (!(0 <= legendRow < this->mLegendInfo->GetNumberOfRows()))
	{
		NMErr(ctxNMLayer, << "legend row outside bounds!");
		return false;
	}

	vtkDoubleArray* rar = vtkDoubleArray::SafeDownCast(
			this->mLegendInfo->GetColumnByName("range"));

	rar->GetTuple(legendRow, range);

	return true;
}

//void NMLayer::updateSelection(void)
//{
//	// subclass to implement
//}

void NMLayer::updateSelectionData(void)
{
	// subclass to implement
}

void
NMLayer::selectionChanged(const QItemSelection& newSel,
		const QItemSelection& oldSel)
{
	//NMDebugAI(<< this->objectName().toStdString() << ": selection changed!" << std::endl);
	emit layerSelectionChanged(this);
}

void
NMLayer::tableDataChanged(const QModelIndex& tl, const QModelIndex& br)
{
	//NMDebugAI(<< this->objectName().toStdString()
	//		<< ": data changed at " << tl.column() << ", " << tl.row() << std::endl);
	this->mHasChanged = true;
	emit dataSetChanged(this);
}

void
NMLayer::tableColumnsInserted(const QModelIndex& parent, int startsection,
		int endsection)
{
	NMDebugAI(<< this->objectName().toStdString() << ": column inserted at "
			  << startsection << std::endl);
	this->mHasChanged = true;
	emit dataSetChanged(this);
}

void
NMLayer::selectCell(int cellID, NMLayerSelectionType type)
{
	const QModelIndex idx = this->mTableModel->index(cellID, 0, QModelIndex());
	switch(type)
	{
	case NM_SEL_ADD:
		//mSelectionModel->select(idx, QItemSelectionModel::Select);
		mSelectionModel->toggleRow(cellID, 0, QModelIndex());
		break;
	case NM_SEL_REMOVE:
		//mSelectionModel->select(idx, QItemSelectionModel::Deselect);
		mSelectionModel->toggleRow(cellID, 0, QModelIndex());
		break;
	case NM_SEL_NEW:
		mSelectionModel->clearSelection();
		mSelectionModel->toggleRow(cellID, 0, QModelIndex());
		//mSelectionModel->select(idx, QItemSelectionModel::ClearAndSelect);
		break;
	case NM_SEL_CLEAR:
		mSelectionModel->clearSelection();
		break;
	default:
		return;
		break;
	}
	emit legendChanged(this);
	emit visibilityChanged(this);
}

void
NMLayer::tableColumnsRemoved(const QModelIndex& parent, int startsection,
		int endsection)
{
	NMDebugAI(<< this->objectName().toStdString() << ": column removed at "
			  << startsection << std::endl);
	this->mHasChanged = true;
	emit dataSetChanged(this);
}

void
NMLayer::printSelRanges(const QItemSelection& selection, const QString& msg)
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
