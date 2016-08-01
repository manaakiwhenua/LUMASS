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
//#include "otbFocalDistanceWeightingFilter_ExplicitInst.h"

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
            typename FilterType::WeightMatrixType weights)
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
            //if (pos >= p->mRadiusList.size())
            //	pos = 0;
            pos = p->mapHostIndexToPolicyIndex(step, p->mRadiusList.size());

			unsigned int radius = p->mRadiusList.at(pos).toInt(&bok);
			if (bok && radius > 0)
				f->SetRadius(radius);
		}

		// --------------------------------------------------------------------
		// set the values list
		if (p->mValues.size() > 0)
		{
			pos = step;
            //if (pos >= p->mValues.size())
            //	pos = 0;
            pos = p->mapHostIndexToPolicyIndex(step, p->mValues.size());

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
            //if (pos >= p->mWeights.size())
            //	pos = 0;
            pos = p->mapHostIndexToPolicyIndex(step, p->mWeights.size());

			QList<QStringList> strMatrix = p->mWeights.at(pos);
			int nrows = strMatrix.size();
			NMDebugAI( << "... we've got " << nrows << " rows ..." << std::endl);
			if (nrows > 0)
			{
				// get the first row to check the number of columns
				QStringList firstRow = strMatrix.at(0);
				const int ncols = firstRow.size();
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

						std::vector<typename FilterType::WeightType> vw;
						//typename FilterType::WeightType vw[numcols];
						for (int col=0; col < ncols; ++col)
						{
							QString w = strCols.at(col);
							if (!w.isEmpty())
							{
								typename FilterType::WeightType weight =
										static_cast<typename FilterType::WeightType>(w.toFloat(&bok));
								if (bok)
								{
									//vw[col] = weight; 
									vw.push_back(weight);
								}
								else
								{
									//vw[col] = 0; 
									vw.push_back(0);
								}
							}
						}
						weights.set_row(row, (typename FilterType::WeightType*)&vw.at(0)); //vw.data());
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

template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, char, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, short, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, int, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, long, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, float, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, double, 1>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, char, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, short, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, int, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, long, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, float, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, double, 2>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned char, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<char, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned short, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<short, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned int, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<int, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<unsigned long, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<long, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<float, double, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, char, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, short, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, int, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, unsigned long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, long, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, float, 3>;
template class NMFocalNeighbourhoodDistanceWeightingWrapper_Internal<double, double, 3>;

InstantiateObjectWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
SetNthInputWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
GetOutputWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )
LinkInternalParametersWrap( NMFocalNeighbourhoodDistanceWeightingWrapper, NMFocalNeighbourhoodDistanceWeightingWrapper_Internal )

NMFocalNeighbourhoodDistanceWeightingWrapper
::NMFocalNeighbourhoodDistanceWeightingWrapper(QObject* parent)
{
    this->setParent(parent);
    this->setObjectName("NMFocalNeighbourhoodDistanceWeightingWrapper");
    this->mInputComponentType = otb::ImageIOBase::SHORT;
    this->mOutputComponentType = otb::ImageIOBase::INT;
}

NMFocalNeighbourhoodDistanceWeightingWrapper
::~NMFocalNeighbourhoodDistanceWeightingWrapper()
{
}
