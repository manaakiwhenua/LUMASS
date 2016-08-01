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
 * DEMSlopeAspectFilter.h
 *
 *  Created on: 13/01/2011
 *      Author: alex
 */

#ifndef DEMSlopeAspectFilter_H_
#define DEMSlopeAspectFilter_H_

#include "itkImageToImageFilter.h"
#include "itkNeighborhoodAllocator.h"
#include "itkNeighborhoodIterator.h"
#include "itkNeighborhood.h"
#include "nmlog.h"
// ToDo: check, if really required
//#include "itkConceptChecking.h"

#include "otbsupplfilters_export.h"

namespace otb {

template <class TInputImage, class TOutputImage >
class OTBSUPPLFILTERS_EXPORT DEMSlopeAspectFilter
			: public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:

  typedef DEMSlopeAspectFilter							Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>		Superclass;
  typedef itk::SmartPointer<Self>								Pointer;
  typedef itk::SmartPointer<const Self>							ConstPointer;

  itkNewMacro(Self);
  itkTypeMacro(DEMSlopeAspectFilter, itk::ImageToImageFilter);

  typedef TInputImage						InputImageType;
  typedef typename InputImageType::Pointer	InputImagePointer;
  typedef typename InputImageType::RegionType InputImageRegionType;
  typedef typename InputImageType::PixelType  InputImagePixelType;
  typedef typename InputImageType::SizeType   InputImageSizeType;

  typedef TOutputImage						OutputImageType;
  typedef typename OutputImageType::Pointer	OutputImagePointer;
  typedef typename OutputImageType::RegionType OutputImageRegionType;
  typedef typename OutputImageType::PixelType  OutputImagePixelType;



//  typedef typename TInputImageType::OffsetType	InputOffsetType;
//  typedef itk::NeighborhoodIterator<TInputImage> NeighborhoodIteratorType;

  typedef enum {GRADIENT_HORN, GRADIENT_ZEVEN} NmGradientAlgorithm;
  typedef enum {GRADIENT_DEGREE, GRADIENT_PERCENT, GRADIENT_ASPECT} NmGradientUnit;

//  typedef typename TInputImage::ImageDimension ImgDimType;
  typedef itk::NeighborhoodAllocator<InputImagePixelType> NeighborhoodAllocType;
  typedef itk::Neighborhood<InputImagePixelType, 2, NeighborhoodAllocType> NeighborhoodType;

  void SetGradientAlgorithm(NmGradientAlgorithm algo);
  void SetGradientUnit(NmGradientUnit unit);

  virtual void GenerateInputRequestedRegion() throw(itk::InvalidRequestedRegionError);

protected:
    DEMSlopeAspectFilter();
    virtual ~DEMSlopeAspectFilter();
//	void PrintSelf(std::ostream& os, itk::Indent indent) const;

//	virtual void GenerateOutputInformation();

    //void aspect(const NeighborhoodType& nh, double* val);
        void slope(const NeighborhoodType& nh, double* val);
	void dZdX(const NeighborhoodType& nh, double* val);
	void dZdY(const NeighborhoodType& nh, double* val);

//	int getUpslopeArea(const NeighborhoodType& nh, )

//	OffsetType GetNextUpwardCell(NeighborhoodIteratorType iter, double xdist, double ydist, double digdist);
//	InputImagePixelType GetNumUpwardCells(NeighborhoodIteratorType iter);

    virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                           itk::ThreadIdType threadId);

    //void GenerateData(void);

private:
	double m_xdist;
	double m_ydist;

	NmGradientAlgorithm m_algo;
	NmGradientUnit m_unit;

};

} // end namespace

//#include "otbDEMSlopeAspectFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbDEMSlopeAspectFilter.txx"
#endif

#endif /* DEMSlopeAspectFilter_H_ */


