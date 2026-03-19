include(GNUInstallDirs)
# Information of libsdb to allow the installations
# here we specify where the library will be installed
# as well as the includes, we use GNUInstallDirs to
# get these libraries
install(TARGETS libsdb
        EXPORT sdb-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# For the public headers
install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# This will make sdb project importable from other
# files, we export that information to a file
install(
        EXPORT sdb-targets
        FILE sdb-config.cmake
        NAMESPACE sdb::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdb
)