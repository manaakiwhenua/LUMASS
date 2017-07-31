/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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

#ifndef NMMODELOBJECT_H
#define NMMODELOBJECT_H

#include "NMObject.h"
#include "nmmodframe_export.h"

class NMModelController;

class NMMODFRAME_EXPORT NMModelObject : public NMObject
{
public:
    NMModelObject();

    virtual void setModelController(NMModelController* controller);
    NMModelController* getModelController(void);

protected:
    NMModelController* mController;

};

#endif // NMMODELOBJECT_H
