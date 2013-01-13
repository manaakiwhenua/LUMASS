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
 * NMFocalNeighbourhoodDistanceWeightingWrapper.cpp
 *
 *  Created on: 12/01/2013
 *      Author: alex
 */

#include "NMFocalNeighbourhoodDistanceWeightingWrapper.h"
#include "NMMacros.h"
#include "NMMfwException.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbFocalDistanceWeightingFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template <class InPixelType, class OutPixelType, unsigned int Dimension=2>
class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal
{
public:
	typedef otb::Image<InPixelType, Dimension> InImgType;
	typedef otb::Image<OutPixelType, Dimension> OutImgType;
	typedef otb::FocalDistanceWeightingFilter<InImgType, OutImgType> FilterType;
	typedef typename FilterType::Pointer FilterTypePointer;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		FilterTypePointer f = FilterType::New();
		otbFilter = f;
	}

	static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
	{
		InImgType* img = dynamic_cast<InImgType*>(dataObj);
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetInput(img);
	}

	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
	}

	static void setInternalRadius(itk::ProcessObject::Pointer& otbFilter,
			unsigned int userradius)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetRadius(userradius);
	}

	static void setInternalValues(itk::ProcessObject::Pointer& otbFilter,
			std::vector<InPixelType> values)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetValues(values);
	}

	static void setWeights(itk::ProcessObject::Pointer& otbFilter,
			itk::Array2D<typename FilterType::WeightMatrixType> weights)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		filter->SetWeights(weights);
	}

	static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMFocalNeighbourhoodDistanceWeightingWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMFocalNeighbourhoodDistanceWeightingWrapper* p =
				dynamic_cast<NMFocalNeighbourhoodDistanceWeightingWrapper*>(proc);

		// make sure we've got a valid filter object
		if (f == 0)
		{
			NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
			e.setMsg("We're trying to link, but the filter doesn't seem to be initialised properly!");
			throw e;
			return;
		}

		int pos = step;
		bool bok;

		// --------------------------------------------------------------------
		// set the radius
		if (p->mRadiusList.size() > 0)
		{
			if (pos >= p->mRadiusList.size())
				pos = 0;

			unsigned int radius = p->mRadiusList.at(pos).toInt(&bok);
			if (bok && radius > 0)
				f->SetRadius(radius);
		}

		// --------------------------------------------------------------------
		// set the values list
		if (p->mValues.size() > 0)
		{
			pos = step;
			if (pos >= p->mValues.size())
				pos = 0;

			std::vector<InPixelType> values;
			QStringList valuesList = p->mValues.at(pos);
			foreach(const QString strVal, valuesList)
			{
				InPixelType v = static_cast<InPixelType>(strVal.toFloat(&bok));
				if (bok)
					values.push_back(v);
				else
				{
					NMErr("NMFocalNeighbourhoodDistanceWeightingWrapper_Internal",
							<< "Couldn't parse influence value: " << strVal.toStdString() << "!");
				}
			}

			if (values.size() > 0)
				f->SetValues(values);
		}

		// --------------------------------------------------------------------
		NMDebugAI(<< "setting weight matrix ... " << std::endl);
		// set the weights matrix
		if (p->mWeights.size() > 0)
		{
			pos = step;
			if (pos >= p->mWeights.size())
				pos = 0;

			QList<QStringList> strMatrix = p->mWeights.at(pos);
			int nrows = strMatrix.size();
			NMDebugAI( << "... we've got " << nrows << " rows ..." << std::endl);
			if (nrows > 0)
			{
				// get the first row to check the number of columns
				QStringList firstRow = strMatrix.at(0);
				int ncols = firstRow.size();
				NMDebugAI(<< "... we've got " << ncols << " columns ..." << std::endl);

				if (ncols > 0)
				{
					typename FilterType::WeightMatrixType weights(nrows, ncols);
					for(int row=0; row < nrows; ++row)
					{
						QStringList strCols = strMatrix.at(row);
						if (strCols.size() != ncols)
						{
							NMErr("NMFocalNeighbourhoodDistanceWeighting_Internal",
									<< "Invalid weights matrix detected!");

							NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
							e.setMsg("Invalid weights matrix detected!");
							throw e;
						}

						//std::vector<typename FilterType::WeightType> vw;
						typename FilterType::WeightType vw[ncols];
						for (int col=0; col < ncols; ++col)
						{
							QString w = strCols.at(col);
							if (!w.isEmpty())
							{
								typename FilterType::WeightType weight =
										static_cast<typename FilterType::WeightType>(w.toFloat(&bok));
								if (bok)
								{
									vw[col] = weight; //vw.push_back(weight);
								}
								else
									vw[col] = 0; // vw.push_back(0);
							}
						}
						weights.set_row(row, vw); //vw.data());
					}
					f->SetWeights(weights);
				}
			}
		}

		// DEBUG
		// let's print the filter's self information
		f->Print(std::cout, itk::Indent(2));

		NMDebugCtx("NMFocalNeighbourhoodDistanceWeightingWrapper_Internal", << "done!");
	}
};

InstantiateObjectWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
SetNthInputWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
GetOutputWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
LinkInternalParametersWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )

NMFocalNeighbourhoodDistanceWeightingWrapper
::NMFocalNeighbourhoodDistanceWeightingWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMFocalNeighbourhoodDistanceWeightingWrapper");
	this->mInputComponentType = itk::ImageIOBase::SHORT;
	this->mInputComponentType = itk::ImageIOBase::INT;
}

NMFocalNeighbourhoodDistanceWeightingWrapper
::~NMFocalNeighbourhoodDistanceWeightingWrapper()
{
}
