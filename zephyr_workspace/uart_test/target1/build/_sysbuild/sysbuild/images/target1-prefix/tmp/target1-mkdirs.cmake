# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/uttec/revita/zephyr_workspace/uart_test/target1")
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target1")
endif()
file(MAKE_DIRECTORY
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/target1"
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix"
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/tmp"
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/src/target1-stamp"
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/src"
  "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/src/target1-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/src/target1-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target1/build/_sysbuild/sysbuild/images/target1-prefix/src/target1-stamp${cfgdir}") # cfgdir has leading slash
endif()
