/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
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

#ifndef __otbKernelScriptParserErrorr_h
#define __otbKernelScriptParserErrorr_h


#include "itkMacro.h"
#include "otbsupplfilters_export.h"

namespace otb
{

class OTBSUPPLFILTERS_EXPORT KernelScriptParserError : public itk::ExceptionObject
{
public:
    KernelScriptParserError();
    virtual ~KernelScriptParserError() throw() {}

    KernelScriptParserError(const char* file, unsigned int lineNumber);
    KernelScriptParserError(const std::string& file, unsigned int lineNumber);

    KernelScriptParserError & operator=(const KernelScriptParserError& orig);

    itkTypeMacro(KernelScriptParserError, ExceptionObject)
};

}

#endif // header guard
