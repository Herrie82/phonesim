set(PHONESIM_SRCS
    control.cpp
    controlbase.ui
    qatutils.cpp
    qatresult.cpp
    qwsppdu.cpp
    qgsmcodec.cpp
    callmanager.cpp
    qsimterminalresponse.cpp
    qsmsmessagelist.cpp
    qsmsmessage.cpp
    qsimcontrolevent.cpp
    conformancesimapplication.cpp
    server.cpp
    aidapplication.cpp
    gsmitem.cpp
    simauth.cpp
    phonesim.cpp
    qsimcommand.cpp
    qsimenvelope.cpp
    qcbsmessage.cpp
    hardwaremanipulator.cpp
    simapplication.cpp
    qatresultparser.cpp
    attranslator.cpp
    gsmspec.cpp
    main.cpp
    simfilesystem.cpp
    aes.c
    comp128.c
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(phonesim ${PHONESIM_SRCS})
target_compile_definitions(phonesim PRIVATE -DVERSION=${PHONESIM_VERSION} -DQT_NO_FOREACH)

if(Qt6_FOUND)
    target_link_libraries(phonesim Qt6::Core Qt6::Widgets Qt6::Qml Qt6::Network Qt6::DBus Qt6::Core5Compat)
else()
    target_link_libraries(phonesim Qt5::Core Qt5::Widgets Qt5::Qml Qt5::Network Qt5::DBus)
endif()

install(TARGETS phonesim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES default.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/phonesim/)