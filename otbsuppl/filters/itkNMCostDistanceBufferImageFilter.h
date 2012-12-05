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

/**
 * References:
 *
 * ITK: itkDanielssonDistanceMapImageFilter
 *
 * Eastman JR: PUSHBROOM ALGORITHMS FOR CALCULATING DISTANCES IN RASTER GRIDS.
 * http://http://mapcontext.com/autocarto/proceedings/auto-carto-9/pdf/pushbroom-algorithms-for-calculating-distances-in-raster-grids.pdf
 * (accessed Nov 2012)
 *
 *
 *
 */

#ifndef __itkNMCostDistanceBufferImageFilter_h
#define __itkNMCostDistanceBufferImageFilter_h

#include <map>
#include <itkImageToImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>

namespace itk
{

/** \class NMCostDistanceBufferImageFilter
 *
 *
 */

template <class TInputImage,class TOutputImage>
class ITK_EXPORT NMCostDistanceBufferImageFilter :
    public ImageToImageFilter<TInputImage,TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NMCostDistanceBufferImageFilter             Self;
  typedef ImageToImageFilter<TInputImage,TOutputImage> Superclass;
  typedef SmartPointer<Self>                           Pointer;
  typedef SmartPointer<const Self>                     ConstPointer;

  /** Method for creation through the object factory */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro( NMCostDistanceBufferImageFilter, ImageToImageFilter );

  /** Type for input image. */
  typedef   TInputImage       InputImageType;

  typedef typename InputImageType::PixelType InPixelType;

  /** Type for two of the three output images: the VoronoiMap and the
   * DistanceMap.  */
  typedef   TOutputImage      OutputImageType;

  /** Type for the region of the input image. */
  typedef typename InputImageType::RegionType   RegionType;

  /** Type for the image spacing */
  typedef typename InputImageType::SpacingType SpacingType;

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

  /** Pointer Type for input image. */
  typedef typename InputImageType::Pointer InputImagePointer;
  typedef typename InputImageType::ConstPointer InputConstImagePointer;

  /** Pointer Type for the output image. */
  typedef typename OutputImageType::Pointer OutputImagePointer;

  /** Pixel type of the output image */
  typedef typename OutputImageType::PixelType OutPixelType;

  /** Set if image spacing should be used in computing distances. */
  itkSetMacro( UseImageSpacing, bool );

  /** Get whether spacing is used. */
  itkGetConstReferenceMacro( UseImageSpacing, bool );

  /** Set On/Off whether spacing is used. */
  itkBooleanMacro( UseImageSpacing );

  itkSetMacro( ProcessDownward, bool);
  itkBooleanMacro( ProcessDownward );

  itkSetMacro( ProcessUpward, bool);
  itkBooleanMacro( ProcessUpward );

  void SetCategories(std::vector<double> cats);

  /** Set the maximum allowed distance measure */
  itkSetMacro( MaxDistance, double);

  /** Get the currently set maximum distance */
  itkGetMacro( MaxDistance, double);

  /** Set a buffer zone indicator */
  itkSetMacro( BufferZoneIndicator, int);

  /** Get the currently set BufferZoneIndicator */
  itkGetMacro( BufferZoneIndicator, int);

  /** */
  itkSetMacro( CreateBuffer, bool);

  /** Set on/off buffer creation */
  itkBooleanMacro( CreateBuffer );

  /** reset the number of execution */
  void resetExecCounter(void)
  	  {
	    this->m_NumExec = 1;
  	    this->m_UpwardCounter = 1;
  	  }

  /** Get Distance map image. */
  OutputImageType * GetDistanceMap(void);

  /** set the required input data*/
  void GenerateInputRequestedRegion(void);

  /** Set the output requested region to the largest possible region */
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
  NMCostDistanceBufferImageFilter();
  virtual ~NMCostDistanceBufferImageFilter();
  void PrintSelf(std::ostream& os, Indent indent) const;

  /** calculate the output distance map*/
  void GenerateData();  


  void initPixel(OutPixelType* obuf,
		  	     InPixelType* ibuf,
		  	     OutPixelType& maxDist,
		  	     int& cidx);

  void calcPixelDistance(OutPixelType* obuf,
		                 InPixelType* ibuf,
		                 InPixelType* cbuf,
		                 double* colDist,
		                 double* rowDist,
		                 int** noff,
		                 bool& bleftright,
		                 bool& binit,
		                 int& row,
		                 int& ncols,
		                 int& nrows,
		                 int& bufrow,
			             OutPixelType& maxDist,
		                 SpacingType& spacing);

   void writeBufferZoneValue(OutPixelType* obuf,
		  	  	  	  	     OutPixelType& userDist,
		  	  	  	  	     int& row,
		  	  	  	  	     int& ncols);

private:   
  NMCostDistanceBufferImageFilter(const Self&);
  void operator=(const Self&);

  /** input data mapping
   *
   * 0 = feature image
   * 1 = cost image
   */

  double* m_Categories;
  int m_NumCategories;
  OutPixelType m_MaxDistance;

  double* m_colDist;
  double* m_rowDist;

  OutputImagePointer m_TempOutput;

  int m_NumExec;
  int m_UpwardCounter;
  bool m_CreateBuffer;
  bool m_UseImageSpacing;
  bool m_ProcessDownward;
  bool m_ProcessUpward;
  int  m_BufferZoneIndicator;

}; // end of NMCostDistanceBufferImageFilter class


template <class TInputImage, class TOutputImage>
inline void
NMCostDistanceBufferImageFilter<TInputImage, TOutputImage>
::initPixel(OutPixelType* obuf,
			InPixelType* ibuf,
			OutPixelType& maxDist,
			int& cidx)
{
	if (m_NumCategories)
	{
		bool bObj = false;
		for (int e=0; e < this->m_NumCategories; ++e)
		{
			if (static_cast<double>(ibuf[cidx]) == m_Categories[e])
			{
				obuf[cidx] = 0;
				bObj = true;
				break;
			}
		}

		if (!bObj)
		{
			obuf[cidx] = maxDist;
		}
	}
	else if (ibuf[cidx] > 0)
	{
		obuf[cidx] = 0;
	}
	else
	{
		obuf[cidx] = maxDist;
	}
}



template <class TInputImage,class TOutputImage>
inline void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::calcPixelDistance(OutPixelType* obuf,
		            InPixelType* ibuf,
		            InPixelType* cbuf,
		            double* colDist,
		            double* rowDist,
		            int** noff,
	                bool& bleftright,
	                bool& binit,
	                int& row,
	                int& ncols,
	                int& nrows,
	                int& bufrow,
	                OutPixelType& maxDist,
	                SpacingType& spacing)
{
	int col;
	int step;
	int end;
	if (bleftright)
	{
		col = 0;
		step = 1;
		end = ncols;
	}
	else
	{
		col = ncols - 1;
		step = -1;
		end = -1;
	}

	double voff[3][2];
	double minDist;
	double tmpDist;
	double x,y;

	for (; col != end; col += step)
	{
		int cidx = col + row * ncols;

		// we only call this when we visit the
		// pixel for the first time
		if (binit)
		{
			initPixel(obuf,ibuf,maxDist,cidx);
		}

		if (obuf[cidx] == 0)
		{
			// just make sure, we re-set the distance buffer
			// for 'object' pixels to zero again, since
			// we're re-using the same buffer over and over
			// again
			colDist[(col+1) + (bufrow * (ncols + 2))] = 0;
			rowDist[(col+1) + (bufrow * (ncols + 2))] = 0;
			continue;
		}

		minDist = maxDist;
		int ch = -9;
		for (int c=0; c < 3; ++c)
		{
			int dnidx = (col + 1 + noff[c][0]) + ((bufrow + noff[c][1]) * (ncols + 2));
			x = colDist[dnidx];
			if (x == -9)
				continue;

			if (cbuf)
			{
				if (c)
				{
					tmpDist = cbuf[cidx]
					          + obuf[(col + noff[c][0])
								     + ((row + noff[c][1]) * ncols)];
				}
				else
				{
					tmpDist = (cbuf[cidx] * 1.414214)
								+ obuf[(col + noff[c][0])
								       + ((row + noff[c][1]) * ncols)];
				}
			}
			else
			{
				y = rowDist[dnidx];
				// make sure we only work on pixel inside the buffer
				switch(c)
				{
				case 0: // diagonal
					x += spacing[0];
					y += spacing[1];
					break;

				case 1: // y-direction
					y += spacing[1];
					break;

				case 2: // x-direction
					x += spacing[0];
					break;

				default:
					continue;
					break;
				}

				tmpDist = (x * x) + (y * y);
			}

			// we're only interested in the minimum distance to the
			// source object
			if (tmpDist < minDist)
			{
				minDist = tmpDist;
				voff[c][0] = x;
				voff[c][1] = y;
				ch = c;
			}
		}
		if (ch != -9)
		{
			if (cbuf)
			{
				if (minDist < obuf[cidx])
					obuf[cidx] = minDist;
			}
			else if (minDist < (obuf[cidx] * obuf[cidx]))
			{
				obuf[cidx] = vcl_sqrt(minDist);
				colDist[col + 1 + (bufrow * (ncols + 2))] = voff[ch][0];
				rowDist[col + 1 + (bufrow * (ncols + 2))] = voff[ch][1];
			}
		}
	}
}


template <class TInputImage,class TOutputImage>
inline void
NMCostDistanceBufferImageFilter<TInputImage,TOutputImage>
::writeBufferZoneValue(OutPixelType* obuf,
					   OutPixelType& userDist,
					   int& row,
			           int& ncols)
{
	for (int c=0; c < ncols; ++c)
	{
		if (obuf[c + row * ncols] <= userDist)
		{
			if (m_BufferZoneIndicator)
			{
				if (obuf[c + row * ncols])
					obuf[c + row * ncols] = m_BufferZoneIndicator;

			}
			else
			{
				obuf[c + row * ncols] = 0; //itk::NumericTraits<OutPixelType>::Zero;
			}
		}
	}
}

} //end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNMCostDistanceBufferImageFilter.txx"
#endif

#endif
