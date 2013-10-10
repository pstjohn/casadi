# A macro for embedding parts of the source into code
#
# Example:  embed_resource("sqic" "wsqic.f90" resource_file)
# 
# This will make available a "const char *" named resource_sqic in the CasADi namespace
# that contains the content of "wsqic.f90" by generating resource_file (a header)
macro (embed_resource symbol inputfile resultfile)
  include_directories(${CMAKE_BINARY_DIR})
  set(${resultfile} "${CMAKE_CURRENT_BINARY_DIR}/resource_${symbol}.hpp")
  add_custom_command(
      OUTPUT "${${resultfile}}"
      COMMAND ${CMAKE_COMMAND} -D "OUTPUT=${${resultfile}}" -D "INPUT=${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}" -D "SYMBOL=${symbol}" -P ${CMAKE_SOURCE_DIR}/cmake_modules/embed_resource.cmake 
      DEPENDS
        ${CMAKE_SOURCE_DIR}/cmake_modules/embed_resource.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}
      VERBATIM
    )
set_source_files_properties( ${${resultfile}} PROPERTIES GENERATED TRUE )
endmacro (embed_resource)