# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/uttec/revita/zephyr_workspace/loraTest/central")
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/central")
endif()
file(MAKE_DIRECTORY
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/central"
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix"
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/tmp"
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/src/central-stamp"
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/src"
  "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/src/central-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/src/central-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/central/build/_sysbuild/sysbuild/images/central-prefix/src/central-stamp${cfgdir}") # cfgdir has leading slash
endif()
