
if(PYTHONINTERP_FOUND)
  FILE(GLOB PACMAN_G2_TEST_FILES tests/*.py)
  foreach(PACMAN_G2_TEST_FILE ${PACMAN_G2_TEST_FILES})
    GET_FILENAME_COMPONENT(PACMAN_G2_TEST_NAME ${PACMAN_G2_TEST_FILE} NAME_WE)
    add_test(${PACMAN_G2_TEST_NAME} python ${CMAKE_CURRENT_SOURCE_DIR}/pactest.py --test=${PACMAN_G2_TEST_FILE} -p ${CMAKE_BINARY_DIR}/src/pacman-g2/pacman-g2 --debug=-1)
  endforeach(PACMAN_G2_TEST_FILE)
else()
  message(WARNING "Running the tests requires python and a successful build")
endif(PYTHONINTERP_FOUND)

