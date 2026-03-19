# create the library where to add sources
add_library(libsdb
        process.cpp
        pipe.cpp)
# assign an ALIAS with a namespace
add_library(sdb::libsdb ALIAS libsdb)

# set the name to a simplified one
# so in linux it does not generate liblibsdb.so
set_target_properties(
        libsdb
        PROPERTIES OUTPUT_NAME sdb
)

# specify the version of C++ to C++17
target_compile_features(libsdb PUBLIC cxx_std_17)

# The include folders, one is PRIVATE in src/include/
# and the public one in include/
target_include_directories(libsdb
        PUBLIC # other projects need this include
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        PRIVATE
            include # other projects do not need this include
)