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
/*
 *  NMScriptableKernelFilterWrapper.cpp
 *
 *  Created on: 2016-05-22
 *      Author: Alexander Herzig
 */

#include "NMScriptableKernelFilterWrapper.h"
#include "nmlog.h"
#include "NMMacros.h"
#include "NMMfwException.h"
#include "NMModelController.h"

#include "itkProcessObject.h"
#include "otbImage.h"
#include "otbNMScriptableKernelFilter.h"

/*! Internal templated helper class linking to the core otb/itk filter
 *  by static methods.
 */
template<class TInputImage, class TOutputImage, unsigned int Dimension>
class NMScriptableKernelFilterWrapper_Internal
{
public:
    typedef otb::Image<TInputImage, Dimension>  InImgType;
    typedef otb::Image<TOutputImage, Dimension> OutImgType;
    typedef typename otb::NMScriptableKernelFilter<InImgType, OutImgType>      FilterType;
    typedef typename FilterType::Pointer        FilterTypePointer;

    // more typedefs
    typedef typename InImgType::PixelType  InImgPixelType;
    typedef typename OutImgType::PixelType OutImgPixelType;

    typedef typename OutImgType::SpacingType      OutSpacingType;
    typedef typename OutImgType::SpacingValueType OutSpacingValueType;
    typedef typename OutImgType::PointType        OutPointType;
    typedef typename OutImgType::PointValueType   OutPointValueType;
    typedef typename OutImgType::SizeValueType    SizeValueType;

	static void createInstance(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands)
	{
		FilterTypePointer f = FilterType::New();
		otbFilter = f;
	}

    static void setNthInput(itk::ProcessObject::Pointer& otbFilter,
                    unsigned int numBands, unsigned int idx, itk::DataObject* dataObj)
    {
        //InImgType* img = dynamic_cast<InImgType*>(dataObj);
        FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
        filter->SetFilterInput(idx, dataObj);
    }


	static itk::DataObject* getOutput(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, unsigned int idx)
	{
		FilterType* filter = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		return dynamic_cast<OutImgType*>(filter->GetOutput(idx));
	}

/*$<InternalRATGetSupport>$*/

/*$<InternalRATSetSupport>$*/


    static void internalLinkParameters(itk::ProcessObject::Pointer& otbFilter,
			unsigned int numBands, NMProcess* proc,
			unsigned int step, const QMap<QString, NMModelComponent*>& repo)
	{
		NMDebugCtx("NMScriptableKernelFilterWrapper_Internal", << "...");

		FilterType* f = dynamic_cast<FilterType*>(otbFilter.GetPointer());
		NMScriptableKernelFilterWrapper* p =
				dynamic_cast<NMScriptableKernelFilterWrapper*>(proc);

		// make sure we've got a valid filter object
		if (f == 0)
		{
			NMMfwException e(NMMfwException::NMProcess_UninitialisedProcessObject);
			e.setMsg("We're trying to link, but the filter doesn't seem to be initialised properly!");
			throw e;
			return;
		}

		/* do something reasonable here */
		bool bok;
		int givenStep = step;

		
        QVariant curRadiusVar = p->getParameter("Radius");
        if (curRadiusVar.isValid())
        {
           std::vector<int> vecRadius;
           QStringList curValVarList = curRadiusVar.toStringList();
           foreach(const QString& vStr, curValVarList) 
           {
                int curRadius = vStr.toInt(&bok);
                if (bok)
                {
                    vecRadius.push_back(static_cast<int>(curRadius));
                }
                else
                {
                    NMErr("NMScriptableKernelFilterWrapper_Internal", << "Invalid value for 'Radius'!");
                    NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                    e.setMsg("Invalid value for 'Radius'!");
                    throw e;
                }
            }
            if (vecRadius.size() > 0)
            {
                f->SetRadius(static_cast<int*>(&vecRadius[0]));
            }
            else
            {
                f->SetRadius(0);
            }
        }

        QVariant curKernelScriptVar = p->getParameter("KernelScript");
        std::string curKernelScript;
        if (curKernelScriptVar.isValid())
        {
           curKernelScript = curKernelScriptVar.toString().toStdString();
            f->SetKernelScript(curKernelScript);
        }

        //QVariant curKernelShapeVar = p->getParameter("KernelShapeType");
        //std::string curKernelShape;
        //if (curKernelShapeVar.isValid())
        {
           //curKernelShape = curKernelShapeVar.toString().toStdString();
           f->SetKernelShape(p->getKernelShapeType().toStdString());
        }

        QVariant curOutputVarNameVar = p->getParameter("OutputVarName");
        std::string curOutputVarName;
        if (curOutputVarNameVar.isValid())
        {
           curOutputVarName = curOutputVarNameVar.toString().toStdString();
            f->SetOutputVarName(curOutputVarName);
        }

        QVariant curNodataVar = p->getParameter("Nodata");
        double curNodata;
        if (curNodataVar.isValid())
        {
           curNodata = curNodataVar.toDouble(&bok);
            if (bok)
            {
                f->SetNodata((curNodata));
            }
            else
            {
                NMErr("NMScriptableKernelFilterWrapper_Internal", << "Invalid value for 'Nodata'!");
                NMMfwException e(NMMfwException::NMProcess_InvalidParameter);
                e.setMsg("Invalid value for 'Nodata'!");
                throw e;
            }
        }


                
	    step = p->mapHostIndexToPolicyIndex(givenStep, p->mInputComponents.size());				
	    std::vector<std::string> userIDs;                                                                       
	    QStringList currentInputs;                                                                              
	    if (step < p->mInputComponents.size())                                                                  
	    {                                                                                                       
		    currentInputs = p->mInputComponents.at(step);                                                   
		    int cnt=0;                                                                                      
		    foreach (const QString& input, currentInputs)                                                   
		    {                                                                                               
		        std::stringstream uid;                                                                      
		        uid << "L" << cnt;                                                                          
		        QString inputCompName = NMModelController::getComponentNameFromInputSpec(input);            
		        NMModelComponent* comp = NMModelController::getInstance()->getComponent(inputCompName);     
		        if (comp != 0)                                                                              
		        {                                                                                           
			        if (comp->getUserID().isEmpty())                                                        
			        {                                                                                       
				        userIDs.push_back(uid.str());                                                   
			        }                                                                                       
			        else                                                                                    
			        {                                                                                       
				        userIDs.push_back(comp->getUserID().toStdString());                             
			        }                                                                                       
		        }                                                                                           
		        else                                                                                        
		        {                                                                                           
			        userIDs.push_back(uid.str());                                                           
		        }                                                                                           
		        ++cnt;                                                                                      
		    }                                                                                               
	    }                                                                                                       
	    f->SetInputNames(userIDs);


		NMDebugCtx("NMScriptableKernelFilterWrapper_Internal", << "done!");
	}
};

InstantiateObjectWrap( NMScriptableKernelFilterWrapper, NMScriptableKernelFilterWrapper_Internal )
SetNthInputWrap( NMScriptableKernelFilterWrapper, NMScriptableKernelFilterWrapper_Internal )
GetOutputWrap( NMScriptableKernelFilterWrapper, NMScriptableKernelFilterWrapper_Internal )
LinkInternalParametersWrap( NMScriptableKernelFilterWrapper, NMScriptableKernelFilterWrapper_Internal )
/*$<RATGetSupportWrap>$*/
/*$<RATSetSupportWrap>$*/

NMScriptableKernelFilterWrapper
::NMScriptableKernelFilterWrapper(QObject* parent)
{
	this->setParent(parent);
	this->setObjectName("NMScriptableKernelFilterWrapper");
	this->mParameterHandling = NMProcess::NM_USE_UP;

    mKernelShapeEnum << "Square" << "Circle";
}

NMScriptableKernelFilterWrapper
::~NMScriptableKernelFilterWrapper()
{
}
