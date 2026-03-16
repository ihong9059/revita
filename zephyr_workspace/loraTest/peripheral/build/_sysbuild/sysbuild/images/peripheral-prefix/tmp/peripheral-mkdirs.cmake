# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/uttec/revita/zephyr_workspace/loraTest/peripheral")
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/peripheral")
endif()
file(MAKE_DIRECTORY
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/peripheral"
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix"
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/tmp"
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/src/peripheral-stamp"
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/src"
  "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/src/peripheral-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/src/peripheral-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/loraTest/peripheral/build/_sysbuild/sysbuild/images/peripheral-prefix/src/peripheral-stamp${cfgdir}") # cfgdir has leading slash
endif()
