# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/uttec/revita/zephyr_workspace/uart_test/target2")
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target2")
endif()
file(MAKE_DIRECTORY
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/target2"
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix"
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/tmp"
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/src/target2-stamp"
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/src"
  "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/src/target2-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/src/target2-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/uttec/revita/zephyr_workspace/uart_test/target2/build/_sysbuild/sysbuild/images/target2-prefix/src/target2-stamp${cfgdir}") # cfgdir has leading slash
endif()
