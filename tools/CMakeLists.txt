add_executable(sdb sdb.cpp)
target_link_libraries(sdb PRIVATE sdb::libsdb PkgConfig::libedit fmt::fmt)

# To allow the installation of the tool into the system
include(GNUInstallDirs)
install(
        TARGETS sdb
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)