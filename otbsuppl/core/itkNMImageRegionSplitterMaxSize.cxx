/******************************************************************************
* Created by Alexander Herzig
* Copyright 2015 Landcare Research New Zealand Ltd
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

#include "itkNMImageRegionSplitterMaxSize.h"

namespace itk
{

/**
 *
 */
NMImageRegionSplitterMaxSize
::NMImageRegionSplitterMaxSize()
    : m_MaxSize(8388608)    // = 8 MiB with 1 byte per pixel
//      m_NumSplitRegions(1)
{
}

void
NMImageRegionSplitterMaxSize
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

unsigned int
NMImageRegionSplitterMaxSize
::GetNumberOfSplitsInternal(unsigned int dim,
                            const IndexValueType regionIndex[],
                            const SizeValueType regionSize[],
                            unsigned int requestedSize) const
{
    double maxsize = requestedSize > 0
            ? requestedSize : m_MaxSize;
    if (maxsize < regionSize[0])
    {
        throw itk::ExceptionObject(__FILE__, __LINE__,
                   "Invalid maximum region size too small!",
                                   __FUNCTION__);
        return 0;
    }

    std::vector<unsigned int> splitRegion;
    splitRegion.resize(dim, 1);

    return this->ComputeSplits(dim, maxsize, regionSize, splitRegion);
}

unsigned int
NMImageRegionSplitterMaxSize
::ComputeSplits(unsigned int dim,
                double maxsize,
                const SizeValueType regionSize[],
                std::vector<unsigned int> &splitRegion) const
{
    if (splitRegion.size() != dim)
    {
        return 0;
    }

    // we don't split along the fastest moving index
    splitRegion[0] = regionSize[0];
    maxsize = maxsize / (double)regionSize[0];

    // determine maximum split region;
    unsigned int numberOfPieces = 1;
    int d=1;
    while(d < dim)
    {
        if (maxsize > 1)
        {
            if (maxsize >= regionSize[d])
            {
                splitRegion[d] = regionSize[d];
            }
            else
            {
                splitRegion[d] = static_cast<unsigned int>(maxsize);
                numberOfPieces *= Math::Ceil<unsigned int>(regionSize[d] / (double)splitRegion[d]);
            }
        }
        else
        {
            splitRegion[d] = 1;
            numberOfPieces *= static_cast<unsigned int>(regionSize[d]);
        }

        maxsize = maxsize / (double)regionSize[d];
        ++d;
    }

    return numberOfPieces;
}

unsigned int
NMImageRegionSplitterMaxSize
::GetSplitInternal(unsigned int dim,
                   unsigned int i,
                   unsigned int requestedSize,
                   IndexValueType regionIndex[],
                   SizeValueType regionSize[]) const
{
    // ==================================================================
    // CALC THE NUMBER OF REGION SPLITS
    // AND DETERMINE SPLIT REGION

    double maxsize = requestedSize > 0
            ? requestedSize : m_MaxSize;
    if (maxsize < regionSize[0])
    {
        throw itk::ExceptionObject(__FILE__, __LINE__,
                   "Invalid maximum region size too small!",
                                   __FUNCTION__);
        return 0;
    }

    std::vector<unsigned int> splitRegion;
    splitRegion.resize(dim, 1);

    int numberOfPieces = this->ComputeSplits(dim, maxsize, regionSize, splitRegion);

    // ==================================================================
    // NOW CALC THE ITH SPLIT REGION

    // don't do anything if requested
    // split region is out of bounds
    if (i >= numberOfPieces)
    {
        throw itk::ExceptionObject(__FILE__, __LINE__,
                                   "The split region index is too high for "
                                   "the configured region size!",
                                   __FUNCTION__);
        return 0;
    }

    double carryover = i;
    int d=1;
    while(d < dim)
    {
        if (splitRegion[d] < regionSize[d])
        {
            const unsigned int npieces = Math::Ceil<unsigned int>(regionSize[d] / (double)splitRegion[d]);
            const unsigned int fwdfactor = static_cast<unsigned int>(carryover) % npieces;

            if (fwdfactor > 0)
            {
                regionIndex[d] += fwdfactor * splitRegion[d];
                if (fwdfactor == npieces - 1)
                {
                    regionSize[d] = regionSize[d] - (fwdfactor * splitRegion[d]);
                }
                else
                {
                    regionSize[d] = splitRegion[d];
                }
            }
            else
            {
                // in case we're dealing with the first piece,
                // the size is of course the max split size for the
                // particular dimension
                regionSize[d] = i == 0 ? splitRegion[d] : 1;
            }

            carryover = carryover / (double)npieces;
        }
        ++d;
    }


    return numberOfPieces;
}


} // end namespace itk
