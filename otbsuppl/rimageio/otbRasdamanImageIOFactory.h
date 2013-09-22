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
	name: otbRasdamanImageIOFactory.h


*/

#ifndef __otbRasdamanImageIOFactory_h
#define __otbRasdamanImageIOFactory_h

#include "itkObjectFactoryBase.h"
#include "otbImageIOBase.h"

namespace otb
{
/** \class RasdamanImageIOFactory
 * \brief Creation d'un instance d'un objet RasdamanImageIO utilisant les object factory.
 */
class ITK_EXPORT RasdamanImageIOFactory : public itk::ObjectFactoryBase
{
public:
  /** Standard class typedefs. */
  typedef RasdamanImageIOFactory   Self;
  typedef itk::ObjectFactoryBase  Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;

  /** Class methods used to interface with the registered factories. */
  virtual const char* GetITKSourceVersion(void) const;
  virtual const char* GetDescription(void) const;

  /** Method for class instantiation. */
  itkFactorylessNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RasdamanImageIOFactory, itk::ObjectFactoryBase);

  /** Register one factory of this type  */
  static void RegisterOneFactory(void)
  {
    RasdamanImageIOFactory::Pointer RasdamanFactory = RasdamanImageIOFactory::New();
    itk::ObjectFactoryBase::RegisterFactory(RasdamanFactory);
  }

protected:
  RasdamanImageIOFactory();
  virtual ~RasdamanImageIOFactory();

private:
  RasdamanImageIOFactory(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};


} // end namespace otb

#endif
