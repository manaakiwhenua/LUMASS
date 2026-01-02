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

PROJECT(SQLite)

INCLUDE_DIRECTORIES(
        ${SQLite_SOURCE_DIR}
)

LINK_DIRECTORIES(
    ${SQLite_BRINARY_DIR}/lib
)

#add_definitions(-DSQLITE_CORE)
#ADD_LIBRARY(SQLiteExtFunc STATIC sqlite3extfunc.c)
add_definitions(-DSQLITE_THREADSAFE=1)
add_definitions(-DSQLITE_ENABLE_RTREE=1)

if(WIN32)
    ADD_LIBRARY(SQLite STATIC sqlite3.c)
else()
    ADD_LIBRARY(SQLite SHARED sqlite3.c)
    target_link_libraries(SQLite pthread dl)
endif()


#install(FILES sqlite3.h sqlite3ext.h DESTINATION include)
if(WIN32)
#        install(TARGETS SQLiteExtFunc
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION bin
#                ARCHIVE DESTINATION lib
#        )

#        install(TARGETS SQLite
#                LIBRARY DESTINATION lib
#                RUNTIME DESTINATION bin
#        )


        install(TARGETS SQLite
                ARCHIVE DESTINATION lib
        )
else()
        #install(TARGETS SQLiteExtFunc ARCHIVE DESTINATION lib)
        install(TARGETS SQLite LIBRARY DESTINATION lib)
endif()
