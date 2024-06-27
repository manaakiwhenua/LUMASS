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
/*
 * NMMfwException.cpp
 *
 *  Created on: 6/01/2013
 *      Author: alex
 */

#include "NMMfwException.h"
#include <string>
#include <sstream>

NMMfwException::NMMfwException(ExceptionType t)
: type(t), msg("")
{
    setMsg();
}

NMMfwException::~NMMfwException() throw()
{
}

const char*
NMMfwException::what() const throw()
{
    return msg.c_str();
}

void
NMMfwException::setType(ExceptionType excType)
{
    type = excType;
    setMsg();
}

NMMfwException::ExceptionType
NMMfwException::getType()
{
    return type;
}

void
NMMfwException::setExecStackInfo(const std::string& execstackinfo)
{
    execstack = execstackinfo;
    setMsg();
}

void
NMMfwException::setDescription(const std::string& descr)
{
    description = descr;
    setMsg();
}

void
NMMfwException::setSource(const string &src)
{
    source = src;
    setMsg();
}

string
NMMfwException::getDescription()
{
    return description;
}

void
NMMfwException::setMsg()
{
    std::stringstream usermsg;
    usermsg << "EXCEPTION - "
            << source << (source.empty() ? "" : ": ")
            << type2string() << " "
            << execstack << (execstack.empty() ? "" : ". ")
            << description;
    msg = usermsg.str();
}

string
NMMfwException::type2string(void) const
{
	string ret;
	switch(this->type)
	{
	case  NMProcess_UninitialisedDataObject:
		ret = "NMProcess: Uninitialised data object.";
		break;
	case  NMProcess_UninitialisedProcessObject:
		ret = "NMProcess: Uninitialised process object.";
		break;
	case  NMProcess_InvalidInput:
		ret = "NMProcess: Invalid input.";
		break;
	case  NMProcess_InvalidParameter:
		ret = "NMProcess: Invalid parameter.";
		break;
	case  NMProcess_MissingParameter:
		ret = "NMProcess: Missing parameter.";
		break;
    case  NMProcess_ExecutionError:
        ret = "NMProcess: Execution error.";
        break;
    case  NMDataComponent_InvalidParameter:
		ret = "NMDataComponent: Invalid parameter.";
		break;
	case  NMModelController_UnregisteredModelComponent:
		ret = "NMModelController: Specified component is not registered.";
		break;
	case NMMosra_InvalidParameter:
		ret = "NMMosra: Invalid paramter.";
		break;
    case NMModelComponent_InvalidUserID:
        ret = "NMModelComponent: Invalid UserID.";
        break;
    case NMModelComponent_InvalidParameter:
        ret = "NMModelComponent: Invalid parameter.";
        break;
    case NMModelComponent_UninitialisedDataObject:
        ret = "NMModelComponent: Uninitialised data object.";
        break;
    case NMModelComponent_RecursiveUpdate:
        ret = "NMModelComponent: Recursive update.";
        break;
	case Unspecified:
		ret = "Unspecified problem.";
		break;
	default:
		ret = "";
		break;
	}

	return ret;
}







