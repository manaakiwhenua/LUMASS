 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2013 Landcare Research New Zealand Ltd
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
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkMeanImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.7 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

/*
 * This class is based on the itk::MeanImageFilter. It has been copied and
 * adjusted by Alexander Herzig, Landcare Research New Zealand Ltd.
 *
 */

#ifndef __otbFocalDistanceWeightingFilter_h
#define __otbFocalDistanceWeightingFilter_h

#include <vector>

#include "nmlog.h"
#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkNumericTraits.h"
#include "itkArray2D.h"

#include "otbsupplfilters_export.h"

namespace otb
{
/*  \brief Implements the neighbourhood weighting parameter m_kd as described
 *         in White R, Engelen G, Ulje I, 1997: The use of constrained cellular
 *         automata for high-resolution modelling of urban land-use dynamics.
 *
 *         This filter implements a focal weighting algorithm as described by
 *         White et al. (1997). The central pixel of a circular neighbourhood
 *         of radius r is weighted according to the occurrence of certain pixel
 *         values within defined distance classes. The number of distance
 *         classes if defined by the radius of the neighbourhood and can be
 *         calculated as follows (note: zero distance is not included!):
 *
 *         int numclasses = ((radius*radius) / 2.0) + 0.5;  // for odd radii
 *         int numclasses = ((radius*radius) / 2.0) + 1.5;  // for even radii
 *
 *         The weights for certain pixel values for each of the distance classes
 *         is provided by a m x n matrix (Weights), where n (i.e. number of columns) equals
 *         the number of distance classes and m (i.e. number of rows) equals the
 *         number of pixel values to account for in the weighting procedure.
 *         The weights matrix is supposed to be ordered with classes representing
 *         increasing distance from left to right. The indices of rows for
 *         individual pixel values needs to be provided as an accordingly
 *         ordered std::vector<InputPixelType> Values (i.e. Weights(i,0) represents
 *         the weight of pixel value Values[i] for the smallest distance class.
 *
 */
template <class TInputImage, class TOutputImage>
class OTBSUPPLFILTERS_EXPORT FocalDistanceWeightingFilter :
    public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Extract dimension from input and output image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  /** Convenient typedefs for simplifying declarations. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  /** Standard class typedefs. */
  typedef FocalDistanceWeightingFilter                          Self;
  typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
  typedef itk::SmartPointer<Self>                                   Pointer;
  typedef itk::SmartPointer<const Self>                             ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(FocalDistanceWeightingFilter, itk::ImageToImageFilter);
  
  /** Image typedef support. */
  typedef typename InputImageType::PixelType                    InputPixelType;
  typedef typename OutputImageType::PixelType                   OutputPixelType;
  typedef typename itk::NumericTraits<InputPixelType>::RealType InputRealType;
  typedef float													WeightType;
  typedef itk::Array2D<float>                                   WeightMatrixType;
  
  typedef typename InputImageType::RegionType                   InputImageRegionType;
  typedef typename OutputImageType::RegionType                  OutputImageRegionType;
  typedef typename InputImageType::SizeType                     InputSizeType;

  /** Set the radius of the circular neighbourhood. */
  itkSetMacro(Radius, unsigned int);

  /** Get the radius of the neighbourhood used to compute the mean */
  itkGetConstReferenceMacro(Radius, unsigned int);

  /** Sets the distance depending weights for user defined pixel values */
  itkSetMacro(Weights, WeightMatrixType);

  /** We need to have an idea in which row of the weights matrix we find
   *  the weights for which pixel value, so we need to provide this information
   *  separately with this vector containing the individual pixel values to
   *  account for in the process in the same order as we're expecting then
   *  to be in the table.
   */
  void SetValues(std::vector<InputPixelType > values)
  	  { this->m_Values = values;}
  
  /** Since we're working on a neighbourhood, we need to override this method
   *  to ensure, we get enough input data */
  virtual void GenerateInputRequestedRegion() throw(itk::InvalidRequestedRegionError);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
                  (itk::Concept::HasNumericTraits<InputPixelType>));
  /** End concept checking */
#endif

protected:
  FocalDistanceWeightingFilter();
  virtual ~FocalDistanceWeightingFilter() {}
  void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** This filter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() */
  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                            itk::ThreadIdType threadId );

  /** Before we do the real work, we just check the
   *  input data for consistency, at least partly ...
   */
  void BeforeThreadedGenerateData(void);

private:
  FocalDistanceWeightingFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  unsigned int m_Radius;
  WeightMatrixType m_Weights;
  std::vector<InputPixelType> m_Values;

};
  
} // end namespace itk

//#include "otbFocalDistanceWeightingFilter_ExplicitInst.h"

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbFocalDistanceWeightingFilter.txx"
#endif

//#endif

#endif
