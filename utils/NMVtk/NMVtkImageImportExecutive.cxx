/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImportExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
        - adapted from original vtkImageImportExecutive (VTK-8.2.0) for use in LUMASS
        - by Alexander Herzig, August 2020
*/


#include "NMVtkImageImportExecutive.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "NMVtkImageImport.h"

vtkStandardNewMacro(NMVtkImageImportExecutive);

//----------------------------------------------------------------------------
int NMVtkImageImportExecutive::ProcessRequest(vtkInformation* request,
                                            vtkInformationVector** inInfoVec,
                                            vtkInformationVector* outInfoVec)
{
  if(this->Algorithm && request->Has(REQUEST_INFORMATION()))
  {
    // Invoke the callback
    NMVtkImageImport *ii = NMVtkImageImport::SafeDownCast(this->Algorithm);
    ii->InvokeUpdateInformationCallbacks();
  }

  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}
