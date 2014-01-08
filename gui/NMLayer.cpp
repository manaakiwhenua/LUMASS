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

#include <QTime>
#include <QColor>
#include <QPainter>

#include "vtkMapper.h"
#include "vtkLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkMath.h"


NMLayer::NMLayer(vtkRenderWindow* renWin,
		vtkRenderer* renderer, QObject* parent)
	: QObject(parent), mSelectionModel(0), mTableModel(0), mTableView(0),
	  mDataSet(0), mActor(0), mMapper(0),
	  mIsVisible(false), mIsSelectable(true), mHasChanged(false),
	  mLayerType(NM_UNKNOWN_LAYER), mClrFunc(0), mLookupTable(0), mLegendInfo(0),
	  mLegendClassType(NM_CLASS_JENKS), mColourRamp(NM_RAMP_BLUE2RED_DIV)
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


	// init some enum to string conversion lists
	mLegendTypeStr << "NM_LEGEND_SINGLESYMBOL"
			       << "NM_LEGEND_RAMP"
			       << "NM_LEGEND_INDEXED"
	               << "NM_LEGEND_CLRTAB";

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
			       << "NM_RAMP_GREEN2YELLOW2RED"
			       << "NM_RAMP_MANUAL";

}

NMLayer::~NMLayer()
{
	NMDebugCtx(ctxNMLayer, << " - " << this->objectName().toStdString());

	if (this->mRenderer != 0)
	{

		this->removeFromMap();
	}

	if (this->mTableView != 0)
	{
		this->mTableView->close();
		delete this->mTableView;
	}

	if (mSelectionModel != 0)
		delete mSelectionModel;

	if (mTableModel != 0)
		delete mTableModel;

	NMDebugCtx(ctxNMLayer, << "done!");
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

void NMLayer::resetLegendInfo(void)
{
	// clear the HashMap
	this->mHashValueIndices.clear();

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
	if (mTableModel == 0)
		return QVariant::Invalid;

	QModelIndex midx = mTableModel->index(0, colidx, QModelIndex());
	return mTableModel->data(midx, Qt::DisplayRole).type();
}

void
NMLayer::initiateLegend(void)
{
	NMDebugCtx(ctxNMLayer, << "...");
	this->updateAttributeTable();

	// ----------------------------------------------------------------------------------
	// GATHER IMAGE LAYER INFORMATION
	if (this->getLayerType() == NMLayer::NM_IMAGE_LAYER)
	{
		NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
		const double* imgStats = il->getStatistics();
		for (int i=0; i < 5; ++i)
			mStats[i] = imgStats[i];

		mLower = mStats[0];
		mUpper = mStats[1];

		switch(il->getITKComponentType())
		{
			case otb::ImageIOBase::UCHAR:  mNodata = 255; mUpper = 254;  break;
			case otb::ImageIOBase::DOUBLE: mNodata = -3.40282e38; break;
			default: /* LONG */			   mNodata = -2147483647; break;
		}
		if (mNodata == mLower)
			mLower = 0;
	}
	else
	{
		mNodata = -2147483647;
	}


	// do we have a table
	if (mTableModel)
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
			mLegendType == NMLayer::NM_LEGEND_SINGLESYMBOL;
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

				// determine non-hole features in case we've got a
				// polygon layer here
				QList<int> raw2source;
				if (this->getLayerType() == NMLayer::NM_VECTOR_LAYER)
				{
					NMVectorLayer* vl = qobject_cast<NMVectorLayer*>(this);
					if (vl->getFeatureType() == NMVectorLayer::NM_POLYGON_FEAT)
					{
						raw2source.reserve(mNumClasses);
						int holeidx = this->getColumnIndex("nm_hole");
						bool bok;
						for (long row=0; row < mNumClasses; ++row)
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

					NMTableCalculator calc(mTableModel, this);
					if (raw2source.size() > 0)
						calc.setRaw2Source(&raw2source);

					std::vector<double> colstats = calc.calcColumnStats(mLegendValueField);
					mStats[0] = colstats[0];	// min
					mStats[1] = colstats[1];	// max
					mStats[2] = colstats[2];	// mean
					mStats[4] = colstats[3];	// sdev
				}

				mLower = mStats[0];
				mUpper = mStats[1];

				mLegendType = NMLayer::NM_LEGEND_RAMP;
				mColourRamp = NMLayer::NM_RAMP_BLUE2RED_DIV;
			}
			// ..................................................................................
			// CLASS_UNIQUE STRING ATTRIBUTE
			else
			{
				NMDebugAI(<< "... mapping unique values of '" << mLegendValueField.toStdString() << "' ..." << std::endl);
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

		mNumClasses = 256;
		// +4: description (field); nodata; > upper; < lower
		mNumLegendRows = mNumClasses + 4;

		mLegendValueField = "Pixel Values";
		mLegendDescrField = "Pixel Values";

		this->mLegendType = NMLayer::NM_LEGEND_RAMP;
		this->mColourRamp = NMLayer::NM_RAMP_BLUE2RED_DIV;
	}

	NMDebugCtx(ctxNMLayer, << "done!");
	this->updateLegend();
}

void
NMLayer::updateLegend(void)
{
	NMDebugCtx(ctxNMLayer, << "...");
	this->resetLegendInfo();

	switch(mLegendType)
	{
	case NM_LEGEND_RAMP:
		this->mapValueRamp();
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				//mNumClasses = 0;//mTableModel->rowCount(QModelIndex());
				//mNumLegendRows = mNumClasses+2;
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

	case NM_LEGEND_SINGLESYMBOL:
	default:
		this->mapSingleSymbol();
		break;
	}

	if (mLayerType == NM_IMAGE_LAYER)
	{
		NMImageLayer* il = qobject_cast<NMImageLayer*>(this);
		vtkImageProperty* iprop = const_cast<vtkImageProperty*>(il->getImageProperty());
		iprop->SetUseLookupTableScalarRange(1);
		iprop->SetLookupTable(mLookupTable);
	}
	else
	{
		vtkOGRLayerMapper* mapper = vtkOGRLayerMapper::SafeDownCast(this->mMapper);
		mapper->SetLookupTable(mLookupTable);
		mapper->UseLookupTableScalarRangeOn();
	}

	emit legendChanged(this);
	// do we need this?
	emit visibilityChanged(this);
	NMDebugCtx(ctxNMLayer, << "done!");
}

void
NMLayer::mapSingleSymbol()
{
	NMDebugCtx(ctxNMLayer, << "...");

	mNumLegendRows = 3;

	double minrange = mNodata > mUpper ? mUpper : mNodata;
	double maxrange = minrange != mNodata ? mNodata : mLower;

	long numclrs = 2;
	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	if (mTableModel)
	{
		if (mLayerType == NM_IMAGE_LAYER)
		{
			mLegendDescrField = tr("All Pixel");
			numclrs = mTableModel->rowCount(QModelIndex())+1;
		}
		else
		{
			vtkDataSetAttributes* dsAttr = this->mDataSet->GetAttributes(vtkDataSet::CELL);
			vtkLongArray* nmids = vtkLongArray::SafeDownCast(dsAttr->GetArray("nm_id"));
			numclrs = nmids->GetNumberOfTuples()+2;
			mLegendDescrField = tr("All Features");
			minrange = -1;
			maxrange = numclrs-2;
		}
	}
	else
	{
		mLegendDescrField = tr("All Pixel");
		numclrs = 2;
	}
	mLookupTable->SetNumberOfTableValues(numclrs);
	mLookupTable->SetTableRange(minrange, maxrange);

	// chose a random colour
	QString sVal;
	vtkMath::RandomSeed(QTime::currentTime().msec());

	double rgba[4];
	for (int i=0; i < 3; i++)
		rgba[i] = vtkMath::Random();
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

	long r = mLayerType == NM_VECTOR_LAYER ? 0 : 1;
	mLookupTable->SetTableValue(0, rgba[0], rgba[1], rgba[2], alpha1);
	for (; r < numclrs; ++r)
	{
		mLookupTable->SetTableValue(r, rgba[0], rgba[1], rgba[2], alpha2);
	}

	NMDebugCtx(ctxNMLayer, << "done!");
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
	const int validx = this->getColumnIndex(mLegendValueField);
	const int descridx = this->getColumnIndex(mLegendDescrField);
	if (validx < 0)
	{
		NMErr(ctxNMLayer, << "Value field not specified!");
		return;
	}
	if (descridx < 0) descridx = validx;

	const QModelIndex sampleIdx = mTableModel->index(0, validx);
	if (mTableModel->data(sampleIdx).type() == QVariant::Double)
	{
		NMErr(ctxNMLayer, << "Continuous data types are not supported!");
		return;
	}

	long ncells = 0;
	if (mLayerType == NM_VECTOR_LAYER)
	{
		vtkDataSetAttributes* dsa = this->mDataSet->GetAttributes(vtkDataSet::CELL);
		vtkAbstractArray* nmid = dsa->GetArray("nm_id");
		ncells = nmid->GetNumberOfTuples();
	}
	else
	{
		ncells = mTableModel->rowCount(QModelIndex());
	}



}


void
NMLayer::mapValueRamp(void)
{

}

void
NMLayer::mapValueClasses(void)
{

}


void
NMLayer::mapColourTable(void)
{
	NMDebugCtx(ctxNMLayer, << "...");

	//mNumClasses = mTableModel->rowCount(QModelIndex());
	//mNumLegendRows = mNumClasses + 2;

	mLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	mLookupTable->SetNumberOfTableValues(mNumClasses+2);
	mLookupTable->SetTableRange(-1, mNumClasses);

	// everything outside the table index range is set to transparent
	mLookupTable->SetTableValue(0, 0, 0, 0, 0);
	mLookupTable->SetTableValue(mNumClasses, 0, 0, 0, 0);

	bool bok;
	double fc[4];
	for (long row=0; row < mNumClasses; ++row)
	{
		const QModelIndex mired   = mTableModel->index(row, mClrTabIdx[0], QModelIndex());
		const QModelIndex migreen = mTableModel->index(row, mClrTabIdx[1], QModelIndex());
		const QModelIndex miblue  = mTableModel->index(row, mClrTabIdx[2], QModelIndex());
		const QModelIndex mialpha = mTableModel->index(row, mClrTabIdx[3], QModelIndex());

		const QVariant vred   = mTableModel->data(mired, Qt::DisplayRole);
		const QVariant vgreen = mTableModel->data(migreen, Qt::DisplayRole);
		const QVariant vblue  = mTableModel->data(miblue, Qt::DisplayRole);
		const QVariant valpha = mTableModel->data(mialpha, Qt::DisplayRole);
		if (vred.type() == QVariant::Double)
		{
			fc[0] = vred.toDouble(&bok);
			fc[1] = vgreen.toDouble(&bok);
			fc[2] = vblue.toDouble(&bok);
			fc[3] = valpha.toDouble(&bok);
		}
		else
		{
			fc[0] = vred.toDouble(&bok)   / 255.0;
			fc[1] = vgreen.toDouble(&bok) / 255.0;
			fc[2] = vblue.toDouble(&bok)  / 255.0;
			fc[3] = valpha.toDouble(&bok) / 255.0;
		}
		mLookupTable->SetTableValue(row+1, fc);
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
			if (legendRow-1 < mLookupTable->GetNumberOfTableValues())
			{
				mLookupTable->GetTableValue(legendRow-1, rgba);
			}
			else
				return false;
		}
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				{
					if (legendRow > 1)
					{
						vtkDoubleArray* clrs = vtkDoubleArray::SafeDownCast(this->mLegendInfo->GetColumnByName("rgba"));
						clrs->GetTuple(legendRow-2, rgba);
					}
					else
					{
						for (int c=0; c < 4; ++c)
							rgba[c] = 0;
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
		break;

	default:
		break;
	}

	//NMDebugCtx(ctxNMLayer, << "done!");
	return true;
}

QIcon NMLayer::getLegendIcon(const int legendRow)
{
	QIcon icon;

	if (mLegendType == NM_LEGEND_RAMP && legendRow == 3)
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
			QPainter painter(&pix);
			QRect inner = pix.rect();
			inner.adjust(1,1,-1,-1);
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

	QPixmap pix(16,60);
	QPainter p(&pix);

	QLinearGradient ramp;
	ramp.setCoordinateMode(QGradient::StretchToDeviceMode);
	ramp.setStart(0.5, 0.0);
	ramp.setFinalStop(0.5, 1.0);

	// dummy, needs to be replaced by one which makes use of
	// the actually specified colour ramp
	ramp.setColorAt(0.0, QColor(255,255,255,255));
	ramp.setColorAt(0.5, QColor(0, 255, 0, 255));
	ramp.setColorAt(1.0, QColor(0, 0, 255, 255));

	p.fillRect(pix.rect(), ramp);

	rampIcon = QIcon(pix);
	return rampIcon;
}

int NMLayer::getLegendItemCount(void)
{
	//if (this->mLegendInfo.GetPointer() == 0)
	//	return 0;
	//return this->mLegendInfo->GetNumberOfRows();
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
			if (legendRow > 1)
			{
				int colidx = this->getColumnIndex(mLegendDescrField);
				const QModelIndex mi = mTableModel->index(legendRow-1, colidx, QModelIndex());
				name = mTableModel->data(mi, Qt::DisplayRole).toString();
			}
			else // legendRow == 1
			{
				name = tr("other");
			}
		}
		break;

	case NM_LEGEND_INDEXED:
		{
			switch(mLegendClassType)
			{
			case NM_CLASS_UNIQUE:
				{
					if (legendRow > 1)
					{
						vtkStringArray* names = vtkStringArray::SafeDownCast(this->mLegendInfo->GetColumnByName("name"));
						name = names->GetValue(legendRow-2).c_str();
					}
					else
					{
						name = tr("other");
					}
				}
				break;


			default:
				break;
			}
		}
		break;

	case NM_LEGEND_SINGLESYMBOL:
		if (legendRow == 1)
			name = tr("Nodata");
		else
			name = tr("Valid Values");
		break;

	case NM_LEGEND_RAMP:
		{
			switch (legendRow)
			{
				case 1:	name = tr("Nodata"); break;
				case 2: name = tr("> Upper"); break;
				//case 3: name = tr("Upper\n\n\nLower"); break;
				case 4: name = tr("< Lower"); break;
				default: name = tr(""); break;
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
		if (!selectable)
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
	if (this->mTableView == 0)
		this->createTableView();

	if (this->mTableView != 0)
		this->mTableView->show();

	this->mTableView->update();
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
