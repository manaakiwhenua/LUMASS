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
}

NMMfwException::~NMMfwException() throw()
{
}

const char*
NMMfwException::what() const throw()
{
	stringstream usermsg;
	usermsg << endl << "EXCEPTION - " << type2string() << endl
			<< msg << endl;

	return usermsg.str().c_str();
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
	case Unspecified:
		ret = "Unspecified problem.";
		break;
	default:
		ret = "";
		break;
	}

	return ret;
}







