file(GLOB_RECURSE MODULE_FILES *.cppm)
add_library(virtlib STATIC )
target_sources(virtlib PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_FILES})