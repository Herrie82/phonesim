cmake_minimum_required(VERSION 2.8.12)
project(phonesim)
set(PHONESIM_VERSION 1.21)

include(FeatureSummary)
include(GNUInstallDirs)

find_package(Qt6 "6.0.0" NO_MODULE COMPONENTS Core Widgets Qml Network DBus Core5Compat)

if(NOT Qt6_FOUND)
    find_package(Qt5 "5.10.0" REQUIRED NO_MODULE COMPONENTS Core Widgets Qml Network DBus)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

################# Enable C++17 #################

set(CMAKE_CXX_STANDARD 17)

################# build and install #################
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
