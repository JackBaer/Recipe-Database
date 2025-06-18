# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/jackbaer/Desktop/Recipe-Database/build/plutosvg_source")
  file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/plutosvg_source")
endif()
file(MAKE_DIRECTORY
  "/Users/jackbaer/Desktop/Recipe-Database/build/plutosvg_build"
  "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix"
  "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/tmp"
  "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/src/plutosvg-populate-stamp"
  "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/src"
  "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/src/plutosvg-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/src/plutosvg-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/jackbaer/Desktop/Recipe-Database/build/external/imgui_bundle/external/hello_imgui/hello_imgui/src/hello_imgui/plutosvg-subbuild/plutosvg-populate-prefix/src/plutosvg-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
