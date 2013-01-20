get_property(pacman-g2-location TARGET ${CMAKE_PROJECT_NAME_LOWER} PROPERTY LOCATION)

if(PYTHONINTERP_FOUND AND (EXISTS  ${pacman-g2-location}))
  message("running tests on: ${pacman-g2-location}")
  set(TEST_PARAMS "-v;--test=./tests/*.py;-p ${pacman-g2-location}")
  if(TESTS_GDB)
    set(TEST_PARAMS "${TEST_PARAMS} --gdb")
  endif(TESTS_GDB)
  if(TESTS_VALGRIND)
    set(TEST_PARAMS "${TEST_PARAMS} --valgrind")
  endif(TESTS_VALGRIND)
  if(TESTS_ADDITIONAL_ARGS)
    set(TEST_PARAMS "${TEST_PARAMS} ${TESTS_ADDITIONAL_ARGS}")
  endif(TESTS_ADDITIONAL_ARGS)
  execute_process(COMMAND python ./pactest.py ${TEST_PARAMS}
		  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		  OUTPUT_VARIABLE outTest
		  RESULT_VARIABLE resTest
		  ERROR_VARIABLE  errTest)
  message("test output: ${outTest}")
  #message("res output: ${resTest}")
  #message("err output: ${errTest}")
else()
  message(FATAL_ERROR "Running the tests requires python and a successful build")
endif(PYTHONINTERP_FOUND AND (EXISTS  ${pacman-g2-location}))