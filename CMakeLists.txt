project(libtasks-root)
cmake_minimum_required(VERSION 2.6)

if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

if(CMAKE_VERSION VERSION_GREATER "3.0.1")
  cmake_policy(SET CMP0048 OLD)
endif(CMAKE_VERSION VERSION_GREATER "3.0.1")

file(STRINGS ${PROJECT_SOURCE_DIR}/VERSION PROJECT_VERSION)

if(CMAKE_BUILD_TYPE MATCHES "Doc")

  # Generate only documentation in this mode
  find_package(Doxygen)
  find_program(GRAPHVIZ NAMES dot)
  find_program(EPSTOPDF NAMES epstopdf)
  find_program(PDFLATEX NAMES pdflatex)

  if(NOT DOXYGEN_FOUND OR NOT GRAPHVIZ OR NOT EPSTOPDF OR NOT PDFLATEX)
    message(FATAL_ERROR "Doxygen, graphviz, epstopdf or pdflatex were not found. API documetation will not be generated.")
  else(NOT DOXYGEN_FOUND OR NOT GRAPHVIZ OR NOT EPSTOPDF OR NOT PDFLATEX)
    configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/docs/Doxyfile)
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/docs/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
      )
    add_custom_target(pdf-doc
      make pdf
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/latex
      COMMENT "Generating PDF API documentation" VERBATIM
      )
    add_dependencies(pdf-doc doc)
    install(DIRECTORY ${PROJECT_BINARY_DIR}/doc/html DESTINATION share/doc/${PROJECT_NAME}-${PROJECT_VERSION})
    install(FILES ${PROJECT_BINARY_DIR}/doc/latex/refman.pdf DESTINATION share/doc/${PROJECT_NAME}-${PROJECT_VERSION}/pdf)
  endif(NOT DOXYGEN_FOUND OR NOT GRAPHVIZ OR NOT EPSTOPDF OR NOT PDFLATEX)

else(CMAKE_BUILD_TYPE MATCHES "Doc")

  # Normal compilation mode
  file(GLOB tasks_headers "include/tasks/*.h")
  file(GLOB net_headers "include/tasks/net/*.h")
  file(GLOB serial_headers "include/tasks/serial/*.h")
  file(GLOB tools_headers "include/tasks/tools/*.h")
  install(FILES ${tasks_headers} DESTINATION include/tasks)
  install(FILES ${net_headers} DESTINATION include/tasks/net)
  install(FILES ${serial_headers} DESTINATION include/tasks/serial)
  install(FILES ${tools_headers} DESTINATION include/tasks/tools)

  find_program(VERA++ vera++)
  if(VERA++)
    execute_process(COMMAND ${VERA++} --version OUTPUT_VARIABLE VERA++_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(VERA++_VERSION VERSION_GREATER "1.2.0")
      file(GLOB_RECURSE tasks_srcs "tasks/*.cpp")
      add_custom_target(StyleCheck ALL
        ${VERA++} -d -e -x stylecheck.xml
        -R F001 -R F002 -R L002 -R T001 -R T003 -R T004 -R T005
        -R T006 -R T007 -R T008 -R T009 -R T010 -R T017 -R T019
        ${tasks_headers}
        ${net_headers}
        ${tools_headers}
        ${tasks_srcs}
        )
    else(VERA++_VERSION VERSION_GREATER "1.2.0")
      message(WARNING "Style checks deactivated: vera++ version >=1.2.1 required but ${VERA++_VERSION} found")
    endif(VERA++_VERSION VERSION_GREATER "1.2.0")
  else(VERA++)
    message(WARNING "Style checks deactivated: vera++ not found")
  endif(VERA++)
  
  SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
  
  add_subdirectory(tasks)

  if(WITH_EXAMPLES MATCHES "y" OR WITH_EXAMPLES MATCHES "Y")
    add_subdirectory(examples)
  endif(WITH_EXAMPLES MATCHES "y" OR WITH_EXAMPLES MATCHES "Y")

  if(NOT(DISABLE_TESTS MATCHES "y" OR DISABLE_TESTS MATCHES "Y"))
    add_subdirectory(tests)
    enable_testing()
    add_test(NAME UnitTests_single_loop
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests
      COMMAND bash ${PROJECT_SOURCE_DIR}/tests/runtests.sh ./libtasks-tests ${PROJECT_SOURCE_DIR}/tests single)
    add_test(NAME UnitTests_multi_loop
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests
      COMMAND bash ${PROJECT_SOURCE_DIR}/tests/runtests.sh ./libtasks-tests ${PROJECT_SOURCE_DIR}/tests multi)
  endif(NOT(DISABLE_TESTS MATCHES "y" OR DISABLE_TESTS MATCHES "Y"))

endif(CMAKE_BUILD_TYPE MATCHES "Doc")
