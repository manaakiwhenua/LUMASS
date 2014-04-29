/*=========================================================================

  Program:   Visualization Toolkit
  Module:    NMVtkQtAbstractModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/******************************************************************************
* Adapted by Alexander Herzig
* Copyright 2014 Landcare Research New Zealand Ltd
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
 * see header file as to why the file needed changing
 *
 *
 */


#include "vtkObject.h" // For vtkGenericWarningMacro
#include "NMVtkQtAbstractModelAdapter.h"

int NMVtkQtAbstractModelAdapter::ModelColumnToFieldDataColumn(int col) const
{
  int result = -1;
  switch (this->ViewType)
    {
    case FULL_VIEW:
      result = col;
      break;
    case DATA_VIEW:
      result = this->DataStartColumn + col;
      break;
    default:
      vtkGenericWarningMacro("NMVtkQtAbstractModelAdapter: Bad view type.");
      break;
    };
  return result;
}
