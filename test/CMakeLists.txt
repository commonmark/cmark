# To get verbose output: cmake --build build --target "test" -- ARGS='-V'

# By default, we run the spec tests only if python3 is available.
# To require the spec tests, compile with -DSPEC_TESTS=1

if(SPEC_TESTS)
  set(PYTHON_REQUIRED REQUIRED)
else()
  set(PYTHON_REQUIRED)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
  find_package(Python3 ${PYTHON_REQUIRED} COMPONENTS Interperter)
else()
  find_package(PythonInterp 3 ${PYTHON_REQUIRED})
  set(Python_Interpreter_FOUND ${PYTHONINTERP_FOUND})
  add_executable(Python::Interpreter IMPORTED)
  set_target_properties(Python::Interpreter PROPERTIES
    IMPORTED_LOCATION ${PYTHON_EXECUTABLE})
endif()

if (WIN32)
  set(ROUNDTRIP "${CMAKE_CURRENT_SOURCE_DIR}/roundtrip.bat")
else(WIN32)
  set(ROUNDTRIP "${CMAKE_CURRENT_SOURCE_DIR}/roundtrip.sh")
endif(WIN32)

IF (Python_Interpreter_FOUND)

  add_test(html_normalization
    $<Python::Interpreter> "-m" "doctest"
    "${CMAKE_CURRENT_SOURCE_DIR}/normalize.py"
    )

  if (CMARK_SHARED)
    add_test(spectest_library
      $<Python::Interpreter> "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec"
      "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt" "--library-dir" "${CMAKE_CURRENT_BINARY_DIR}/../src"
      )

    add_test(pathological_tests_library
      $<Python::Interpreter> "${CMAKE_CURRENT_SOURCE_DIR}/pathological_tests.py"
      "--library-dir" "${CMAKE_CURRENT_BINARY_DIR}/../src"
      )

    add_test(roundtriptest_library
      $<Python::Interpreter>
      "${CMAKE_CURRENT_SOURCE_DIR}/roundtrip_tests.py"
      "--spec" "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt"
      "--library-dir" "${CMAKE_CURRENT_BINARY_DIR}/../src"
      )

    add_test(entity_library
      $<Python::Interpreter>
      "${CMAKE_CURRENT_SOURCE_DIR}/entity_tests.py"
      "--library-dir" "${CMAKE_CURRENT_BINARY_DIR}/../src"
      )
  endif()

  add_test(spectest_executable
    $<Python::Interpreter> "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec" "${CMAKE_CURRENT_SOURCE_DIR}/spec.txt" "--program" "${CMAKE_CURRENT_BINARY_DIR}/../src/cmark"
    )

  add_test(smartpuncttest_executable
    $<Python::Interpreter> "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec" "${CMAKE_CURRENT_SOURCE_DIR}/smart_punct.txt" "--program" "${CMAKE_CURRENT_BINARY_DIR}/../src/cmark --smart"
    )

  add_test(regressiontest_executable
    $<Python::Interpreter>
    "${CMAKE_CURRENT_SOURCE_DIR}/spec_tests.py" "--no-normalize" "--spec"
    "${CMAKE_CURRENT_SOURCE_DIR}/regression.txt" "--program"
    "${CMAKE_CURRENT_BINARY_DIR}/../src/cmark"
    )

ELSE(Python_Interpreter_FOUND)

  message(WARNING "A Python 3 Interpreter is required to run the spec tests")

ENDIF(Python_Interpreter_FOUND)

