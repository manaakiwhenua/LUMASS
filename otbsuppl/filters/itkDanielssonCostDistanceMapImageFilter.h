/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkDanielssonCostDistanceMapImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2008-10-14 19:20:32 $
  Version:   $Revision: 1.31 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkDanielssonCostDistanceMapImageFilter_h
#define __itkDanielssonCostDistanceMapImageFilter_h

#include <map>
#include <itkImageToImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>

namespace itk
{

/** \class DanielssonCostDistanceMapImageFilter
 *
 * This class is parametrized over the type of the input image
 * and the type of the output image.
 *
 * This filter computes the distance map of the input image 
 * as an approximation with pixel accuracy to the Euclidean distance.
 *
 * The input is assumed to contain numeric codes defining objects.
 * The filter will produce as output the following images:
 *
 * - A voronoi partition using the same numeric codes as the input.
 * - A distance map with the approximation to the euclidean distance.
 *   from a particular pixel to the nearest object to this pixel
 *   in the input image.
 * - A vector map containing the component of the vector relating
 *   the current pixel with the closest point of the closest object
 *   to this pixel. Given that the components of the distance are
 *   computed in "pixels", the vector is represented by an
 *   itk::Offset.  That is, physical coordinates are not used.
 *
 * This filter is N-dimensional and known to be efficient
 * in computational time.  The algorithm is the N-dimensional version
 * of the 4SED algorithm given for two dimensions in:
 * 
 * Danielsson, Per-Erik.  Euclidean Distance Mapping.  Computer
 * Graphics and Image Processing 14, 227-248 (1980).
 *
 * \ingroup ImageFeatureExtraction 
 *
 */

template <class TInputImage,class TOutputImage>
class ITK_EXPORT DanielssonCostDistanceMapImageFilter :
    public ImageToImageFilter<TInputImage,TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef DanielssonCostDistanceMapImageFilter             Self;
  typedef ImageToImageFilter<TInputImage,TOutputImage> Superclass;
  typedef SmartPointer<Self>                           Pointer;
  typedef SmartPointer<const Self>                     ConstPointer;

  /** Method for creation through the object factory */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro( DanielssonCostDistanceMapImageFilter, ImageToImageFilter );

  /** Type for input image. */
  typedef   TInputImage       InputImageType;

  /** Type for two of the three output images: the VoronoiMap and the
   * DistanceMap.  */
  typedef   TOutputImage      OutputImageType;

  /** Type for the region of the input image. */
  typedef typename InputImageType::RegionType   RegionType;

  /** Type for the index of the input image. */
  typedef typename RegionType::IndexType  IndexType;

  /** Type for the index of the input image. */
  typedef typename InputImageType::OffsetType  OffsetType;

  /** Type for the size of the input image. */
  typedef typename RegionType::SizeType   SizeType;
  
  /** The dimension of the input and output images. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      InputImageType::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  typedef itk::Vector<float, InputImageDimension> VecPixelType;

  /** Pointer Type for the vector distance image */
  typedef Image< VecPixelType, InputImageDimension > VectorImageType;

  /** Pointer Type for input image. */
  typedef typename InputImageType::Pointer InputImagePointer;

  /** Pointer Type for the output image. */
  typedef typename OutputImageType::Pointer OutputImagePointer;

  /** Pointer Type for the vector distance image. */
  typedef typename VectorImageType::Pointer VectorImagePointer;

  typedef typename OutputImageType::PixelType OutPixelType;

  /** Set if the distance should be squared. */
  itkSetMacro( SquaredDistance, bool );

  /** Get the distance squared. */
  itkGetConstReferenceMacro( SquaredDistance, bool );

  /** Set On/Off if the distance is squared. */
  itkBooleanMacro( SquaredDistance );

  /** Set if the input is binary. If this variable is set, each
   * nonzero pixel in the input image will be given a unique numeric
   * code to be used by the Voronoi partition.  If the image is binary
   * but you are not interested in the Voronoi regions of the
   * different nonzero pixels, then you need not set this.  */
  itkSetMacro( InputIsBinary, bool );

  /** Get if the input is binary.  See SetInputIsBinary(). */
  itkGetConstReferenceMacro( InputIsBinary, bool );

  /** Set On/Off if the input is binary.  See SetInputIsBinary(). */
  itkBooleanMacro( InputIsBinary );

  /** Set if image spacing should be used in computing distances. */
  itkSetMacro( UseImageSpacing, bool );

  /** Get whether spacing is used. */
  itkGetConstReferenceMacro( UseImageSpacing, bool );

  /** Set On/Off whether spacing is used. */
  itkBooleanMacro( UseImageSpacing );

  //itkSetMacro( Categories, std::unordered_map<unsigned int, double>);

  void SetCategories(const std::vector<double>& cats)
  	  {this->m_Categories = cats;};

  /** Get Distance map image. */
  OutputImageType * GetDistanceMap(void);

  /** Get vector field of distances. */
  //VectorImageType * GetVectorDistanceMap(void);

  void EnlargeOutputRequestedRegion(DataObject* data);


#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimensionCheck,
    (Concept::SameDimension<InputImageDimension, OutputImageDimension>));
  itkConceptMacro(UnsignedIntConvertibleToOutputCheck,
    (Concept::Convertible<unsigned int, typename TOutputImage::PixelType>));
  itkConceptMacro(IntConvertibleToOutputCheck,
    (Concept::Convertible<int, typename TOutputImage::PixelType>));
  itkConceptMacro(DoubleConvertibleToOutputCheck,
    (Concept::Convertible<double, typename TOutputImage::PixelType>));
  itkConceptMacro(InputConvertibleToOutputCheck,
    (Concept::Convertible<typename TInputImage::PixelType,
                          typename TOutputImage::PixelType>));
  /** End concept checking */
#endif

protected:
  DanielssonCostDistanceMapImageFilter();
  virtual ~DanielssonCostDistanceMapImageFilter() {};
  void PrintSelf(std::ostream& os, Indent indent) const;

  /** Compute Danielsson distance map and Voronoi Map. */
  void GenerateData();  

  /** Prepare data. */
  void PrepareData();  

  void calcPixelDistance(OutPixelType* obuf,
		                 VecPixelType* vbuf,
		                 int noff[3][2],
		                 int col,
		                 int row,
		                 int ncols,
		                 int nrows,
		                 unsigned int buflen);

  /**  Compute Voronoi Map. */
 // void ComputeVoronoiMap();

 // /** Update distance map locally.  Used by GenerateData(). */
 // void UpdateLocalDistance(VectorImageType*,
 //                          const IndexType&,
 //                          const OffsetType&);

private:   
  DanielssonCostDistanceMapImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  std::vector<double> m_Categories;

  VectorImagePointer m_OffsetImage;

  bool                  m_SquaredDistance;
  bool                  m_InputIsBinary;
  bool                  m_UseImageSpacing;
//  bool					m_ComputeVoronoi;
 // bool					m_ComputeVectorDistance;

}; // end of DanielssonCostDistanceMapImageFilter class

} //end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkDanielssonCostDistanceMapImageFilter.txx"
#endif

#endif