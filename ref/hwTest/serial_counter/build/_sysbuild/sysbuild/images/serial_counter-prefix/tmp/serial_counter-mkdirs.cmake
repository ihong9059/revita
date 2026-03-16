# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/uttec/revita/hwTest/serial_counter")
  file(MAKE_DIRECTORY "/home/uttec/revita/hwTest/serial_counter")
endif()
file(MAKE_DIRECTORY
  "/home/uttec/revita/hwTest/serial_counter/build/serial_counter"
  "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix"
  "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/tmp"
  "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/src/serial_counter-stamp"
  "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/src"
  "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/src/serial_counter-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/src/serial_counter-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/uttec/revita/hwTest/serial_counter/build/_sysbuild/sysbuild/images/serial_counter-prefix/src/serial_counter-stamp${cfgdir}") # cfgdir has leading slash
endif()
