#******************************************************************************
# Created by Alexander Herzig
# Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
# 
# This file is part of 'LUMASS', which is free software: you can redistribute
# it and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************

PROJECT(shared)

include_directories(${QT5_INCLUDE_DIRS})
#link_directories(${QT5_LINK_DIRS})

set(CMAKE_AUTOMOC YES)
include(GenerateExportHeader)

file(GLOB SHARED_CPP 
    ${shared_SOURCE_DIR}/NMLogger.cpp 
    ${shared_SOURCE_DIR}/NMObject.cpp
)

add_definitions(-DNMShared_EXPORTS)
ADD_LIBRARY(NMShared SHARED ${SHARED_CPP})
set_property(TARGET NMShared PROPERTY LINKER_LANGUAGE CXX)
generate_export_header(NMShared)
target_link_libraries(NMShared Qt5::Core)

if (WIN32)
    install(TARGETS NMShared 
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin
    )
else()
    install(TARGETS NMShared LIBRARY DESTINATION lib)
endif()

FILE(GLOB SHARED_HEADER
        ${shared_SOURCE_DIR}/*.h
)

FILE(GLOB SHARED_SQL
        ${shared_SOURCE_DIR}/*.sql
)

file(COPY
        ${SHARED_HEADER}
        ${SHARED_SQL}
        DESTINATION ${shared_BINARY_DIR}
)

install(FILES ${SHARED_HEADER} DESTINATION include)
