# Synthetic terrain/border/morpho test data are generated at test time into
# HCMMS_FIXTURE_DIR. The interface passes paths in fixed 63-character fields,
# so "<dir>/morpho" must fit into 63 characters (otherwise every case fails
# with error 36 because the path is silently truncated).
set(HCMMS_FIXTURE_DIR "${CMAKE_BINARY_DIR}/fx" CACHE PATH "Directory for generated test data (max. 56 characters)")
string(LENGTH "${HCMMS_FIXTURE_DIR}" _fx_len)
if(_fx_len GREATER 56)
  message(FATAL_ERROR
    "HCMMS_FIXTURE_DIR is ${_fx_len} characters long (max. 56):\n  ${HCMMS_FIXTURE_DIR}\n"
    "The HCMMS interface limits data paths to 63 characters. Use a shorter build "
    "directory or pass -DHCMMS_FIXTURE_DIR=<short path>.")
endif()

function(hcmms_test_exe name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(${name} PRIVATE hcmms::hcmms)
  hcmms_set_compile_options(${name})
endfunction()

# --- Test data generator --------------------------------------------------------
add_executable(hcmms_fixtures fixtures.cpp)
hcmms_set_compile_options(hcmms_fixtures)
add_test(NAME fixtures_generate COMMAND hcmms_fixtures ${HCMMS_FIXTURE_DIR})
# The checksum ties the generated data to the golden data set: if it differs,
# the test data (platform math) is at fault, not the library.
set_tests_properties(fixtures_generate PROPERTIES FIXTURES_SETUP fx
  PASS_REGULAR_EXPRESSION "fnv=e99a57c8f7ce9b4e")

# --- Unit tests -------------------------------------------------------------------
hcmms_test_exe(test_fortran_io test_fortran_io.cpp)
add_test(NAME fortran_io COMMAND test_fortran_io)

hcmms_test_exe(test_interface test_interface.cpp)
add_test(NAME interface COMMAND test_interface ${HCMMS_FIXTURE_DIR})
set_tests_properties(interface PROPERTIES FIXTURES_REQUIRED fx)

# --- Golden comparison (docs/adr/0001, tests/golden/README.md) ------------------
hcmms_test_exe(golden_compare golden_compare.cpp)
add_test(NAME golden_compare
  COMMAND golden_compare ${CMAKE_CURRENT_SOURCE_DIR}/golden/cases.txt ${HCMMS_FIXTURE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/golden/expected.txt)
set_tests_properties(golden_compare PROPERTIES FIXTURES_REQUIRED fx)

# --- Command line tool ------------------------------------------------------------
if(TARGET hcmms_cli)
  add_test(NAME cli COMMAND hcmms_cli ${CMAKE_CURRENT_SOURCE_DIR}/golden/cases.txt ${HCMMS_FIXTURE_DIR})
  set_tests_properties(cli PROPERTIES FIXTURES_REQUIRED fx
    PASS_REGULAR_EXPRESSION "p2p_base\\|err=0\\|0 45\\.78985213650[0-9]* 186 517")
  add_test(NAME cli_usage COMMAND hcmms_cli)
  set_tests_properties(cli_usage PROPERTIES WILL_FAIL TRUE)
endif()
