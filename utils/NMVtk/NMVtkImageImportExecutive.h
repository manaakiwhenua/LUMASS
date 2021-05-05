/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImportExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageImportExecutive
 *
 * vtkImageImportExecutive
*/

/*
        - adapted from original vtkImageImportExecutive (VTK-8.2.0) for use 
          with adapted vtkImageImport (-> NMVtkImageImport) in LUMASS to 
          support 'long long' and 'unsigned long long' data types
        - by Alexander Herzig, August 2020
*/

#ifndef NMVtkImageImportExecutive_h
#define NMVtkImageImportExecutive_h

//#include "vtkIOImageModule.h" // For export macro
#include "vtkStreamingDemandDrivenPipeline.h"

class NMVtkImageImportExecutive :
  public vtkStreamingDemandDrivenPipeline
{
public:
  static NMVtkImageImportExecutive* New();
  vtkTypeMacro(NMVtkImageImportExecutive,
                       vtkStreamingDemandDrivenPipeline);

  /**
   * Override to implement some requests with callbacks.
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo) override;

protected:
  NMVtkImageImportExecutive() {}
  ~NMVtkImageImportExecutive() override {}

private:
  NMVtkImageImportExecutive(const NMVtkImageImportExecutive&) = delete;
  void operator=(const NMVtkImageImportExecutive&) = delete;
};

#endif
