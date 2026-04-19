cmake_minimum_required(VERSION 3.16)

project(MIA_Proyecto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(backend

    Backend/main.cpp

    Backend/CommandParser/CommandParser.cpp

    Backend/Estructuras/Mbr.cpp
    Backend/Estructuras/Ebr.cpp

    Backend/DiskManagement/Mkdisk.cpp
    Backend/DiskManagement/Fdisk.cpp
    Backend/DiskManagement/Rmdisk.cpp

    Backend/FilesFolders/Mkfile.cpp
    Backend/FilesFolders/Mkdir.cpp
    Backend/FilesFolders/Cat.cpp
    Backend/FilesFolders/Helpers.cpp
    Backend/FilesFolders/Remove.cpp
    Backend/FilesFolders/Rename.cpp
    Backend/FilesFolders/Copy.cpp
    Backend/FilesFolders/Move.cpp
    Backend/FilesFolders/Find.cpp
    Backend/FilesFolders/Chown.cpp

    Backend/FileSystem/Login.cpp
    Backend/FileSystem/Logout.cpp   
    Backend/FileSystem/Mkfs.cpp
    Backend/FileSystem/Reset.cpp
    Backend/Mounting/Mount.cpp

    Backend/Mounting/Mounted.cpp

    Backend/Reportes/Rep.cpp
    Backend/Reportes/RepHelpers.cpp
    Backend/Reportes/Rep_Sb.cpp
    Backend/Reportes/Rep_bm_inode.cpp
    Backend/Reportes/Rep_bm_bloc.cpp
    Backend/Reportes/RepInode.cpp

    Backend/Reportes/RepBlock.cpp
    Backend/Reportes/RepTree.cpp
    Backend/Reportes/RepFile.cpp
    Backend/Reportes/RepLs.cpp


    Backend/Users/Chgrp.cpp
    Backend/Users/Mkgrp.cpp
    Backend/Users/Rmgrp.cpp
    Backend/Users/Mkusr.cpp
    Backend/Users/Rmusr.cpp
    Backend/Users/Users.cpp

    Backend/Utilidades/Utilities.cpp
    Backend/Utilidades/ResetUtil.cpp
)

target_include_directories(backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/external
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/CommandParser
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Estructuras
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/DiskManagement
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/FilesFolders
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/FileSystem
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Mounting
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Reportes
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Users
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Utilidades
)

# Threads
find_package(Threads REQUIRED)

# OpenSSL — requerido por httplib
find_package(OpenSSL REQUIRED)

target_link_libraries(backend PRIVATE
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)