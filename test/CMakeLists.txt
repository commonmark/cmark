# To get verbose output: cmake --build build --target "test" -- ARGS='-V'

# By default, we run the spec tests only if python3 is available.
# To require the spec tests, compile with -DSPEC_TESTS=1

if(SPEC_TESTS)
  set(PYTHON_REQUIRED REQUIRED)
else()
  set(PYTHON_REQUIRED)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
  find_package(Python3 ${PYTHON_REQUIRED} COMPONENTS Interpreter)
else()
  find_package(PythonInterp 3 ${PYTHON_REQUIRED})
  set(Python3_Interpreter_FOUND ${PYTHONINTERP_FOUND})
  add_executable(Python3::Interpreter IMPORTED)
  set_target_properties(Python3::Interpreter PROPERTIES
    IMPORTED_LOCATION ${PYTHON_EXECUTABLE})
endif()

IF (Python3_Interpreter_FOUND)

  add_test(NAME html_normalization
           COMMAND "$<TARGET_FILE:Python3::Interpreter>" -m doctest "${CMAKE_CURRENT_SOURCE_DIR}/normalize.py")

  if(BUILD_SHARED_LIBS)
    add_test(NAME spectest_library
             COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py"
                                                           --no-normalize
                                                           --spec "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt"
                                                           --library-dir "$<TARGET_FILE_DIR:cmark>")

    add_test(NAME pathological_tests_library
             COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/pathological_tests.py"
                                                           --library-dir "$<TARGET_FILE_DIR:cmark>")

    add_test(NAME roundtriptest_library
             COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/roundtrip_tests.py"
                                                           --spec "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt"
                                                           --library-dir "$<TARGET_FILE_DIR:cmark>")

    add_test(NAME entity_library
             COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/entity_tests.py"
                                                           --library-dir "$<TARGET_FILE_DIR:cmark>")
  endif()

  add_test(NAME spectest_executable
           COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py"
                                                         --no-normalize
                                                         --spec "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt"
                                                         --program "$<TARGET_FILE:cmark_exe>")

  add_test(NAME smartpuncttest_executable
           COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py"
                                                         --no-normalize
                                                         --spec "${CMAKE_CURRENT_SOURCE_DIR}/smart_punct.txt"
                                                         --program "$<TARGET_FILE:cmark_exe> --smart")

  add_test(NAME regressiontest_executable
           COMMAND "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py"
                                                         --no-normalize
                                                         --spec "${CMAKE_CURRENT_SOURCE_DIR}/regression.txt"
                                                         --program "$<TARGET_FILE:cmark_exe>")

ELSE(Python3_Interpreter_FOUND)

  message(WARNING "A Python 3 Interpreter is required to run the spec tests")

ENDIF(Python3_Interpreter_FOUND)
