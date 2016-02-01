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
 * NMMfwException.h
 *
 *  Created on: 6/01/2013
 *      Author: alex
 */

#ifndef NMMFWEXCEPTION_H_
#define NMMFWEXCEPTION_H_

#include <string>
#include <exception>
#include "nmmodframe_export.h"

using namespace std;

class NMMODFRAME_EXPORT NMMfwException: public std::exception
{
public:

    typedef enum
    {
        NMProcess_UninitialisedDataObject,
        NMProcess_UninitialisedProcessObject,
        NMProcess_InvalidInput,
        NMProcess_InvalidParameter,
        NMProcess_MissingParameter,
        NMProcess_ExecutionError,
        NMDataComponent_InvalidParameter,
        NMModelController_UnregisteredModelComponent,
        NMMosra_InvalidParameter,
        NMModelComponent_InvalidUserID,
        NMModelComponent_InvalidParameter,
        NMModelComponent_UninitialisedDataObject,
        Unspecified
    } ExceptionType;

	NMMfwException(ExceptionType t = Unspecified);
	virtual ~NMMfwException() throw();

	virtual const char* what() const throw ();

	void setMsg(const string& msg)
	{
		this->msg = msg;
	}

protected:
	string msg;
	ExceptionType type;

	string type2string(void) const ;

};

#endif /* NMMFWEXCEPTION_H_ */
