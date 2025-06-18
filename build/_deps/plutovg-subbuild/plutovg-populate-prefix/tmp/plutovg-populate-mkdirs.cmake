# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-src")
  file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-src")
endif()
file(MAKE_DIRECTORY
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-build"
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix"
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/tmp"
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/src/plutovg-populate-stamp"
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/src"
  "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/src/plutovg-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/src/plutovg-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/_deps/plutovg-subbuild/plutovg-populate-prefix/src/plutovg-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
