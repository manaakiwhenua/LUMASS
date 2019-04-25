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
 * DEMSlopeAspectFilter.txx
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef __otbDEMSlopeAspectFilter_txx
#define __otbDEMSlopeAspectFilter_txx

#include <algorithm>
#include <cmath>

#include "otbDEMSlopeAspectFilter.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkNeighborhoodIterator.h"
#include "itkProgressReporter.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"



namespace otb {

template <class TInputImage, class TOutputImage>
const double DEMSlopeAspectFilter<TInputImage, TOutputImage>::RtoD = 57.29578;

template <class TInputImage, class TOutputImage>
const double DEMSlopeAspectFilter<TInputImage, TOutputImage>::DegToRad = 0.0174533;

template <class TInputImage, class TOutputImage>
const double DEMSlopeAspectFilter<TInputImage, TOutputImage>::Pi = 3.1415926535897932384626433832795;


template <class TInputImage, class TOutputImage>
DEMSlopeAspectFilter<TInputImage, TOutputImage>
::DEMSlopeAspectFilter()
    : m_TerrainAlgorithm("Zevenbergen"),
      m_TerrainAttribute("Slope"),
      m_AttributeUnit("Degree")
{
    this->SetNumberOfRequiredInputs(1);
    this->SetNumberOfRequiredOutputs(1);

    this->m_Pixcounter = 0;
}

template <class TInputImage, class TOutputImage>
DEMSlopeAspectFilter<TInputImage, TOutputImage>
::~DEMSlopeAspectFilter()
{
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SetNodata(const double &nodata)
{
    m_Nodata = static_cast<OutputImagePixelType>(nodata);
    this->Modified();
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SetInputNames(const std::vector<std::string> &inputNames)
{
    m_DataNames.clear();
    for (int n=0; n < inputNames.size(); ++n)
    {
        m_DataNames.push_back(inputNames.at(n));
    }
    this->Modified();
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input)
{
    InputImageType* img = dynamic_cast<InputImageType*>(input);

    if (img)
    {
        int idx = num >= this->GetNumberOfIndexedInputs() ? this->GetNumberOfIndexedInputs(): num;
        Superclass::SetNthInput(idx, input);
        m_IMGNames.push_back(m_DataNames.at(num));
    }
}

template <class TInputImage, class TOutputImage>
TInputImage* DEMSlopeAspectFilter<TInputImage, TOutputImage>
::GetDEMImage()
{
    // popoulate with default position at idx=0;
    int idx = 0;

    // look for index of "dem" image
    for (int n=0; n < m_IMGNames.size(); ++n)
    {
        std::string aName = m_IMGNames.at(n);
        std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
        if (aName.compare("dem") == 0)
        {
            idx = n;
        }
    }

    InputImageType* img = nullptr;
    if (idx < this->GetNumberOfIndexedInputs())
    {
        img = dynamic_cast<InputImageType*>(this->GetIndexedInputs().at(idx).GetPointer());
    }

    return img;
}

template <class TInputImage, class TOutputImage>
TInputImage* DEMSlopeAspectFilter<TInputImage, TOutputImage>
::GetFlowAccImage()
{
    // popoulate with default position at idx=0;
    int idx = 1;

    // look for index of "dem" image
    for (int n=0; n < m_IMGNames.size(); ++n)
    {
        std::string aName = m_IMGNames.at(n);
        std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
        if (aName.compare("flowacc") == 0)
        {
            idx = n;
        }
    }

    InputImageType* img = nullptr;
    if (idx < this->GetNumberOfIndexedInputs())
    {
        img = dynamic_cast<InputImageType*>(this->GetIndexedInputs().at(idx).GetPointer());
    }

    return img;
}


template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw ()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  typename Superclass::InputImagePointer inputPtr =
    const_cast< TInputImage * >( this->GetInput() );
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  InputImageSizeType radius;
  radius.Fill(1);
  inputRequestedRegion.PadByRadius( radius );

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
    {
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion( inputRequestedRegion );

    // build an exception
    itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
    InputImageType* dem = this->GetDEMImage();
    if (dem == nullptr)
    {
        NMProcErr(<< "No DEM input layer specified!");
        return;
    }

    InputImageType* flowacc = this->GetFlowAccImage();

    // assign enums for controlling processing
    if (m_TerrainAlgorithm.compare("Horn") == 0)
    {
        m_eTerrainAlgorithm = ALGO_HORN;
    }
    else // if (m_sTerrainAlgorithm.compare("Zevenbergen") == 0)
    {
        m_eTerrainAlgorithm = ALGO_ZEVEN;
    }

    if (m_TerrainAttribute.compare("Slope") == 0)
    {
        m_eTerrainAttribute = TERRAIN_SLOPE;
    }
    else if (m_TerrainAttribute.compare("LS") == 0)
    {
        m_eTerrainAttribute = TERRAIN_LS;
        if (flowacc == nullptr)
        {
            NMProcErr(<< "No flow accumulation input layer specified!");
            return;
        }
    }
    else if (m_TerrainAttribute.compare("Wetness") == 0)
    {
        m_eTerrainAttribute = TERRAIN_WETNESS;
        if (flowacc == nullptr)
        {
            NMProcErr(<< "No flow accumulation input layer specified!");
            return;
        }
    }
    else if (m_TerrainAttribute.compare("SedTransport") == 0)
    {
        m_eTerrainAttribute = TERRAIN_SEDTRANS;
        if (flowacc == nullptr)
        {
            NMProcErr(<< "No flow accumulation input layer specified!");
            return;
        }
    }

    if (m_AttributeUnit.compare("Degree") == 0)
    {
        m_eAttributeUnit = GRADIENT_DEGREE;
    }
    else if (m_AttributeUnit.compare("Percent") == 0)
    {
        m_eAttributeUnit = GRADIENT_PERCENT;
    }
    else if (m_AttributeUnit.compare("Aspect") == 0)
    {
        m_eAttributeUnit = GRADIENT_ASPECT;
    }
    else //if (m_AttributeUnit.compare("Dim.less") == 0)
    {
        m_eAttributeUnit = GRADIENT_DIMLESS;
    }
}


template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
    OutputImagePointer pOutImg = this->GetOutput();
    InputImageConstPointer pInImg = this->GetDEMImage();
    InputImageConstPointer pFaImg = this->GetFlowAccImage();

    // Find the data-set boundary "faces"
    InputImageSizeType radius;
    radius.Fill(1);

    typename itk::NeighborhoodAlgorithm
            ::ImageBoundaryFacesCalculator<InputImageType>::FaceListType faceList;
    itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
    faceList = bC(pInImg, outputRegionForThread, radius);

    typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>
            ::FaceListType::iterator fit;

    // support progress methods/callbacks
    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());


	// get pixel size in x and y direction
    InputImageSpacingType spacing = pInImg->GetSpacing();
	m_xdist = spacing[0];
    m_ydist = spacing[1];

    itk::ZeroFluxNeumannBoundaryCondition<TInputImage> bndCond;

    KernelIterType inIter;
    RegionIterType outIter;
    InputRegionConstIterType faIter;

    typedef typename itk::PixelTraits< OutputImagePixelType >::ValueType OutputValueType;
    OutputValueType nodata = itk::NumericTraits< OutputValueType >::ZeroValue();
    switch (m_eTerrainAttribute)
    {
    case TERRAIN_SLOPE:
        for (fit = faceList.begin(); fit != faceList.end(); ++fit)
        {
            inIter = KernelIterType(radius, pInImg, *fit);
            inIter.OverrideBoundaryCondition(&bndCond);
            outIter = RegionIterType(pOutImg, *fit);

            for (inIter.GoToBegin(), outIter.GoToBegin(); !inIter.IsAtEnd(); ++inIter, ++outIter)
            {
                double val;
                NeighborhoodType nh = inIter.GetNeighborhood();
                this->Slope(nh, &val);
                val = std::isnan(val) ? nodata : val;
                outIter.Set(val);

                progress.CompletedPixel();
            }
        }
        break;

    case TERRAIN_LS:
        for (fit = faceList.begin(); fit != faceList.end(); ++fit)
        {
            inIter = KernelIterType(radius, pInImg, *fit);
            inIter.OverrideBoundaryCondition(&bndCond);

            faIter = InputRegionConstIterType(pFaImg, *fit);
            outIter = RegionIterType(pOutImg, *fit);

            for (inIter.GoToBegin(), outIter.GoToBegin(), faIter.GoToBegin();
                 !inIter.IsAtEnd();
                 ++inIter, ++outIter, ++faIter
                 )
            {
                double val;
                NeighborhoodType nh = inIter.GetNeighborhood();
                InputImagePixelType fa = faIter.Get();
                this->LS(nh, fa, &val);
                val = std::isnan(val) ? m_Nodata : val;
                outIter.Set(val);

                ++m_Pixcounter;
                progress.CompletedPixel();
            }
        }
        break;

     case TERRAIN_WETNESS:
        for (fit = faceList.begin(); fit != faceList.end(); ++fit)
        {
            inIter = KernelIterType(radius, pInImg, *fit);
            inIter.OverrideBoundaryCondition(&bndCond);

            faIter = InputRegionConstIterType(pFaImg, *fit);
            outIter = RegionIterType(pOutImg, *fit);

            for (inIter.GoToBegin(), outIter.GoToBegin(), faIter.GoToBegin();
                 !inIter.IsAtEnd();
                 ++inIter, ++outIter, ++faIter
                 )
            {
                double val;
                NeighborhoodType nh = inIter.GetNeighborhood();
                InputImagePixelType fa = faIter.Get();
                this->Wetness(nh, fa, &val);
                val = std::isnan(val) ? m_Nodata : val;
                outIter.Set(val);

                ++m_Pixcounter;
                progress.CompletedPixel();
            }
        }
        break;

     case TERRAIN_SEDTRANS:
        for (fit = faceList.begin(); fit != faceList.end(); ++fit)
        {
            inIter = KernelIterType(radius, pInImg, *fit);
            inIter.OverrideBoundaryCondition(&bndCond);

            faIter = InputRegionConstIterType(pFaImg, *fit);
            outIter = RegionIterType(pOutImg, *fit);

            for (inIter.GoToBegin(), outIter.GoToBegin(), faIter.GoToBegin();
                 !inIter.IsAtEnd();
                 ++inIter, ++outIter, ++faIter
                 )
            {
                double val;
                NeighborhoodType nh = inIter.GetNeighborhood();
                InputImagePixelType fa = faIter.Get();
                this->SedTrans(nh, fa, &val);
                val = std::isnan(val) ? m_Nodata : val;
                outIter.Set(val);

                ++m_Pixcounter;
                progress.CompletedPixel();
            }
        }

    }

    NMProcDebug(<< "num pix: " << m_Pixcounter);

    //NMDebug(<< "Leave FlowAcc::GenerateData" << std::endl);
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::AfterThreadedGenerateData()
{
        TInputImage* in = dynamic_cast<TInputImage*>(this->GetIndexedInputs().at(0).GetPointer());

        InputImageRegionType inReg = in->GetLargestPossibleRegion();

        long totpix = inReg.GetSize(0) * inReg.GetSize(1);
        NMProcDebug(<< "the total num of pix is: " << totpix);

        NMProcDebug(<< "pixelcounter is: " << m_Pixcounter);

        if (m_Pixcounter == totpix)
            m_Pixcounter = 0;


}

//template <class TInputImage, class TOutputImage>
//OffsetType DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::GetNextUpwardCell(NeighborhoodIteratorType iter, double xdist, double ydist, double digdist)
//{
//	OffsetType o1 = {{-1,-1}};
//	OffsetType o2 = {{0,-1}};
//	OffsetType o3 = {{1,-1}};
//	OffsetType o4 = {{-1,0}};
//	OffsetType o5 = {{0,0}};
//	OffsetType o6 = {{1,0}};
//	OffsetType o7 = {{-1,1}};
//	OffsetType o8 = {{0,1}};
//	OffsetType o9 = {{1,1}};
//
//
//
//
//}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::dZdX(const NeighborhoodType& nh, double* val)
{
    switch (m_eTerrainAlgorithm)
	{
    case ALGO_ZEVEN:
        *val = (-nh[3] + nh[5]) / (2*m_xdist);
		break;
    case ALGO_HORN:
	default:
        *val = ((nh[2] + 2*nh[5] + nh[8]) - (nh[0] + 2*nh[3] + nh[6])) / (8*m_xdist);
		break;
	}
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::dZdY(const NeighborhoodType& nh, double* val)
{
    switch (m_eTerrainAlgorithm)
	{
    case ALGO_ZEVEN:
        *val = (nh[1] - nh[7]) / (2*m_ydist);
		break;
    case ALGO_HORN:
	default:
        *val = ((nh[8] + 2*nh[7] + nh[6]) - (nh[2] + 2*nh[1] + nh[0])) / (8*m_ydist);
		break;
	}
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::Wetness(const NeighborhoodType& nDem, const InputImagePixelType& flowacc,
          double* val)
{
    const double cellarea = m_xdist * m_ydist;
    const double flaccx = static_cast<double>(flowacc * cellarea);
    // Desmet & Govers assume square pixels; since we support
    // non-square pixels, we need to calculate a single
    // grid cell size to calculate the effective contour length
    // through which the flow is drained; here we approximate D
    // as the diameter of a circle with area D^2 = 'cellarea'
    const double bigD = sqrt(cellarea/Pi) * 2;

    double dzNdx, dzNdy;
    dZdX(nDem, &dzNdx);
    dZdY(nDem, &dzNdy);

    const double tanbeta = sqrt(dzNdx*dzNdx + dzNdy*dzNdy);
    if (tanbeta != 0)
    {
        *val = log((flaccx/bigD) / tanbeta);
    }
    else
    {
        *val = m_Nodata;
    }
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::SedTrans(const NeighborhoodType& nDem, const InputImagePixelType& flowacc,
              double* val)
{
    const double cellarea = m_xdist * m_ydist;
    const double flaccx = static_cast<double>(flowacc * cellarea);
    //const double bigD = sqrt(cellarea/Pi) * 2;

    double dzNdx, dzNdy;
    dZdX(nDem, &dzNdx);
    dZdY(nDem, &dzNdy);

    const double sinbeta = sin(atan(sqrt(dzNdx*dzNdx + dzNdy*dzNdy)));
    *val = pow((flaccx/22.13), 0.6) * pow((sinbeta/0.0896), 1.3);
}


template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::Slope(const NeighborhoodType& nh, double* val)
{
	double zx, zy;
	dZdX(nh, &zx);
	dZdY(nh, &zy);

    switch (m_eAttributeUnit)
	{
	case GRADIENT_ASPECT:
        if (m_eTerrainAlgorithm == ALGO_HORN)
			*val = atan2(zy,zx) - vnl_math::pi / 2.0;
		else
			*val = atan2(zx,zy) - vnl_math::pi;

		*val *= 180 / vnl_math::pi;
		if (*val < 0)
			*val += 360;
		break;
    case GRADIENT_DIMLESS:
        *val = sqrt(zx * zx + zy * zy);
        break;
    case GRADIENT_PERCENT:
		*val = sqrt(zx * zx + zy * zy) * 100;
		break;
	case GRADIENT_DEGREE:
	default:
		*val = atan(sqrt(zx * zx + zy * zy)) * 180 / vnl_math::pi;
		break;
	}
}

template <class TInputImage, class TOutputImage>
void DEMSlopeAspectFilter<TInputImage, TOutputImage>
::LS(const NeighborhoodType& nDem, const InputImagePixelType &flowacc,
     double* val)
{
    const double cellarea = m_xdist * m_ydist;
    const double flaccx = static_cast<double>(flowacc * cellarea);
    // Desmet & Govers assume square pixels; since we support
    // non-square pixels, we need to calculate a single
    // grid cell size to calculate the effective contour length
    // through which the flow is drained; here we approximate D
    // as the diameter of a circle with area D^2 = 'cellarea'
    const double bigD = sqrt(cellarea/Pi) * 2;
    const int RILLEROSION = 0;
    const int THAWINGSOIL = 0;

    double rise_run, sx, ax, xij, sij, lij, m, beta;
    double dzNdx, dzNdy;
    double sinsx, sinax, cosax;

    // ----------------------------------------------------------------------
    *val = m_Nodata;


    this->dZdY(nDem, &dzNdy);
    this->dZdX(nDem, &dzNdx);

    rise_run = pow(((dzNdx*dzNdx) + (dzNdy*dzNdy)), 0.5);
    sx = atan(rise_run) * RtoD;

    //adjust aspect depending on chosen algorithm
    switch (m_eTerrainAlgorithm)
    {
    case ALGO_HORN : //HORN('81)-Method
        ax = (atan2(dzNdy,dzNdx)-1.5707963) * RtoD;
        break;

    default : //Zevenbergen & Thorne('87)-Method
        ax = (atan2(dzNdx,dzNdy)-3.1415927) * RtoD;
        break;
    }

    //Aspect nachbearbeiten
    if (ax < 0)
        ax +=360;

    //check planar slope (i.e. the neighbouring cells show equal elevation)
    bool equalelev = true;
    for (int b=0; b < 7; ++b)
    {
        if (nDem[b] != nDem[b+1])
        {
            equalelev = false;
            break;
        }
    }

    //Berechnung versch. Sinuswerte/Cosinuswerte
    sinsx = sin(sx * DegToRad);
    sinax = sin(ax * DegToRad);
    cosax = cos(ax * DegToRad);
    if (sinsx < 0) sinsx *= -1;
    if (sinax < 0) sinax *= -1;
    if (cosax < 0) cosax *= -1;

    //Weitere Input-Daten berechnen, um sie dann in Desmet&Govers-Algorithmus
    //einzusetzen

    //xij berechnen
    xij = sinax + cosax;

    //Hanglängenexponent (m) berechnen
    beta = ( (sinsx / 0.0896) / ((3 * pow(sinsx, 0.8)) + 0.56) );

    switch (RILLEROSION)
    {
    case 1 :
        beta *= 0.5;
        break;
    case 2 :
        beta *= 2;
        break;
    default:
        ;
    }
    m = beta / (1 + beta);

    //S-Faktor der RUSLE berechnen
    if ((rise_run*100) < 9)
        sij = (10.8 * sinsx) + 0.03;
    else
    {
        switch (THAWINGSOIL)
        {
        case 1 :
            sij = pow((sinsx/0.0896),0.6);
            break;
        default :
            sij = (16.8 * sinsx) - 0.5;
        }
    }

    //LFaktor berechnen
    if (flaccx != m_Nodata)// && ppOver[row][col] != 0.0)
    {
        if (equalelev)
        {

            lij = 1;
        }
        else
        {
            //	lij = ( (pow((flaccx + (cellsize*cellsize)),(m+1)) - pow(flaccx,(m+1)) ) /
                //	(pow(cellsize,(m+2)) * pow(xij,m) * pow(22.13, m)) );

            //lumass uses this version instead of the above implementation because
            //lumass calculates the contributing area at the outlet of a grid cell
            //with coordinates (i,j)
            //furthermore before flowacc calculation is performed, all grid cells are
            //initialized with the area of the grid
            lij = ( (pow(flaccx,(m+1)) - pow(flaccx-cellarea,(m+1)) ) /
                    (pow(bigD,(m+2)) * pow(xij,m) * pow(22.13, m)) );
        }


        //LS-Faktor berechnen und in LSGrid schreiben
        *val = lij * sij;
    }
}


//template <class TInputImage, class TOutputImage>
//InputImagePixelType DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::GetNumUpwardCells(NeighborhoodIteratorType iter)
//{
//
//}


//template <class TInputImage, class TOutputImage>
//void DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::GenerateOutputInformation()
//{
//	NMDebug(<< "Enter FlowAcc::GenerateOutputInfo" << endl);
//
//
//
//	NMDebug(<< "\twe don't do anything here though!" << endl);
//	NMDebug(<< "Leave FlowAcc::GenerateOutputInfo" << endl);
//}

//template <class TInputImage, class TOutputImage>
//void DEMSlopeAspectFilter<TInputImage, TOutputImage>
//::GenerateInputRequestedRegion()
//{
//	NMDebug(<< "Enter FlowAcc::GenerateInputReqReg" << endl);
//
//	Superclass::GenerateInputRequestedRegion();
//
//	// pad the input region by one pixel
////	typename TInputImage::SizeType radius;
////	radius.Fill(1);
//
//	InputImagePointer pInImg = const_cast<TInputImage* >(this->GetInput());
////	OutputImagePointer pOutImg = const_cast<TOutputImage* >(this->GetOutput());
////	typename TInputImage::RegionType inRegion;
////	inRegion = pInImg->GetRequestedRegion();
////	inRegion.PadByRadius(radius);
////	NMDebugInd(1, << "new Region-Index: " << inRegion.GetIndex() <<
////			" | new Region-Size: " << inRegion.GetSize() << endl);
//
////	inRegion.Crop(pInImg->GetLargestPossibleRegion());
//	pInImg->SetRequestedRegion(pInImg->GetLargestPossibleRegion());
//
//
//	NMDebug(<< "\twe don't do anything here though!" << endl);
//	NMDebug(<< "Leave FlowAcc::GenerateInputReqReg" << endl);
//}


} // end namespace

#endif
