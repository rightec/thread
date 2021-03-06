.. _test-framework:

Test Framework
###############

The Zephyr Test Framework (Ztest) provides a simple testing framework intended
to be used during development.  It provides basic assertion macros and a generic
test structure.

The framework can be used in two ways, either as a generic framework for
integration testing, or for unit testing specific modules.

Quick start - Integration testing
*********************************

A simple working base is located at :zephyr_file:`samples/subsys/testsuite/integration`.  Just
copy the files to ``tests/`` and edit them for your needs. The test will then
be automatically built and run by the twister script. If you are testing
the **bar** component of **foo**, you should copy the sample folder to
``tests/foo/bar``. It can then be tested with::

    ./scripts/twister -s tests/foo/bar/test-identifier


In the example above ``tests/foo/bar`` signifies the path to the test and the
``test-identifier`` references a test defined in the :file:`testcase.yaml` file.

To run all tests defined in a test project, run::

    ./scripts/twister -T tests/foo/bar/

The sample contains the following files:

CMakeLists.txt

.. literalinclude:: ../../../samples/subsys/testsuite/integration/CMakeLists.txt
   :language: CMake
   :linenos:

testcase.yaml

.. literalinclude:: ../../../samples/subsys/testsuite/integration/testcase.yaml
   :language: yaml
   :linenos:

prj.conf

.. literalinclude:: ../../../samples/subsys/testsuite/integration/prj.conf
   :language: text
   :linenos:

src/main.c (see :ref:`best practices <main_c_bp>`)

.. literalinclude:: ../../../samples/subsys/testsuite/integration/src/main.c
   :language: c
   :linenos:

.. contents::
   :depth: 1
   :local:
   :backlinks: top



A test case project may consist of multiple sub-tests or smaller tests that
either can be testing functionality or APIs. Functions implementing a test
should follow the guidelines below:

* Test cases function names should be prefix with **test_**
* Test cases should be documented using doxygen
* Test function names should be unique within the section or component being
  tested


An example can be seen below::

    /**
     * @brief Test Asserts
     *
     * This test verifies the zassert_true macro.
     */
    static void test_assert(void)
    {
            zassert_true(1, "1 was false");
    }


The above test is then enabled as part of the testsuite using::

    ztest_unit_test(test_assert)


Listing Tests
=============

Tests (test projects) in the Zephyr tree consist of many testcases that run as
part of a project and test similar functionality, for example an API or a
feature. The ``twister`` script can parse the testcases in all
test projects or a subset of them, and can generate reports on a granular
level, i.e. if cases have passed or failed or if they were blocked or skipped.

Twister parses the source files looking for test case names, so you
can list all kernel test cases, for example, by entering::

        twister --list-tests -T tests/kernel

Skipping Tests
==============

Special- or architecture-specific tests cannot run on all
platforms and architectures, however we still want to count those and
report them as being skipped.  Because the test inventory and
the list of tests is extracted from the code, adding
conditionals inside the test suite is sub-optimal.  Tests that need
to be skipped for a certain platform or feature need to explicitly
report a skip using :c:func:`ztest_test_skip`. If the test runs,
it needs to report either a pass or fail.  For example::

	#ifdef CONFIG_TEST1
	void test_test1(void)
	{
		zassert_true(1, "true");
	}
        #else
	void test_test1(void)
	{
		ztest_test_skip();
	}
	#endif


	void test_main(void)
	{
		ztest_test_suite(common,
				 ztest_unit_test(test_test1),
				 ztest_unit_test(test_test2)
				 );
		ztest_run_test_suite(common);
	}

Quick start - Unit testing
**************************

Ztest can be used for unit testing. This means that rather than including the
entire Zephyr OS for testing a single function, you can focus the testing
efforts into the specific module in question. This will speed up testing since
only the module will have to be compiled in, and the tested functions will be
called directly.

Since you won't be including basic kernel data structures that most code
depends on, you have to provide function stubs in the test. Ztest provides
some helpers for mocking functions, as demonstrated below.

In a unit test, mock objects can simulate the behavior of complex real objects
and are used to decide whether a test failed or passed by verifying whether an
interaction with an object occurred, and if required, to assert the order of
that interaction.

.. _main_c_bp:

Best practices for declaring the test suite
===========================================

*twister* and other validation tools need to obtain the list of
subcases that a Zephyr *ztest* test image will expose.

.. admonition:: Rationale

   This all is for the purpose of traceability. It's not enough to
   have only a semaphore test project.  We also need to show that we
   have testpoints for all APIs and functionality, and we trace back
   to documentation of the API, and functional requirements.

   The idea is that test reports show results for every sub-testcase
   as passed, failed, blocked, or skipped.  Reporting on only the
   high-level test project level, particularly when tests do too
   many things, is too vague.

There exist two alternatives to writing tests. The first, and more verbose,
approach is to directly declare and run the test suites.
Here is a generic template for a test showing the expected use of
:c:func:`ztest_test_suite`:

.. code-block:: C

   #include <ztest.h>

   extern void test_sometest1(void);
   extern void test_sometest2(void);
   #ifndef CONFIG_WHATEVER		/* Conditionally skip test_sometest3 */
   void test_sometest3(void)
   {
   	ztest_test_skip();
   }
   #else
   extern void test_sometest3(void);
   #endif
   extern void test_sometest4(void);
   ...

   void test_main(void)
   {
   	ztest_test_suite(common,
                            ztest_unit_test(test_sometest1),
                            ztest_unit_test(test_sometest2),
                            ztest_unit_test(test_sometest3),
                            ztest_unit_test(test_sometest4)
                   );
   	ztest_run_test_suite(common);
   }

Alternatively, it is possible to split tests across multiple files using
:c:func:`ztest_register_test_suite` which bypasses the need for ``extern``:

.. code-block:: C

  #include <ztest.h>

  void test_sometest1(void) {
  	zassert_true(1, "true");
  }

  ztest_register_test_suite(common, NULL,
  			    ztest_unit_test(test_sometest1)
  			    );

The above sample simple registers the test suite and uses a ``NULL`` pragma
function (more on that later). It is important to note that the test suite isn't
directly run in this file. Instead two alternatives exist for running the suite.
First, if to do nothing. A default ``test_main`` function is provided by
ztest. This is the preferred approach if the test doesn't involve a state and
doesn't require use of the pragma.

In cases of an integration test it is possible that some general state needs to
be set between test suites. This can be thought of as a state diagram in which
``test_main`` simply goes through various actions that modify the board's
state and different test suites need to run. This is achieved in the following:

.. code-block:: C

  #include <ztest.h>

  struct state {
  	bool is_hibernating;
  	bool is_usb_connected;
  }

  static bool pragma_always(const void *state)
  {
  	return true;
  }

  static bool pragma_not_hibernating_not_connected(const void *s)
  {
  	struct state *state = s;
  	return !state->is_hibernating && !state->is_usb_connected;
  }

  static bool pragma_usb_connected(const void *s)
  {
  	return ((struct state *)s)->is_usb_connected;
  }

  ztest_register_test_suite(baseline, pragma_always,
  			    ztest_unit_test(test_case0));
  ztest_register_test_suite(before_usb, pragma_not_hibernating_not_connected,
  			    ztest_unit_test(test_case1),
  			    ztest_unit_test(test_case2));
  ztest_register_test_suite(with_usb, pragma_usb_connected,,
  			    ztest_unit_test(test_case3),
  			    ztest_unit_test(test_case4));

  void test_main(void)
  {
  	struct state state;

	/* Should run `baseline` test suite only. */
	ztest_run_registered_test_suites(&state);

  	/* Simulate power on and update state. */
  	emulate_power_on();
  	/* Should run `baseline` and `before_usb` test suites. */
  	ztest_run_registered_test_suites(&state);

  	/* Simulate plugging in a USB device. */
  	emulate_plugging_in_usb();
  	/* Should run `baseline` and `with_usb` test suites. */
  	ztest_run_registered_test_suites(&state);

  	/* Verify that all the registered test suites actually ran. */
  	ztest_verify_all_registered_test_suites_ran();
  }

For *twister* to parse source files and create a list of subcases,
the declarations of :c:func:`ztest_test_suite` and
:c:func:`ztest_register_test_suite` must follow a few rules:

- one declaration per line

- conditional execution by using :c:func:`ztest_test_skip`

What to avoid:

- packing multiple testcases in one source file

  .. code-block:: C

     void test_main(void)
     {
     #ifdef TEST_feature1
             ztest_test_suite(feature1,
                              ztest_unit_test(test_1a),
                              ztest_unit_test(test_1b),
                              ztest_unit_test(test_1c)
                              );
             ztest_run_test_suite(feature1);
     #endif

     #ifdef TEST_feature2
             ztest_test_suite(feature2,
                              ztest_unit_test(test_2a),
                              ztest_unit_test(test_2b)
                              );
             ztest_run_test_suite(feature2);
     #endif
     }


- Do not use ``#if``

  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1),
                              ztest_unit_test(test_sometest2),
     #ifdef CONFIG_WHATEVER
                              ztest_unit_test(test_sometest3),
     #endif
                              ztest_unit_test(test_sometest4),
             ...

- Do not add comments on lines with a call to :c:func:`ztest_unit_test`:

  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1),
                              ztest_unit_test(test_sometest2) /* will fail */,
             /* will fail! */ ztest_unit_test(test_sometest3),
                              ztest_unit_test(test_sometest4),
             ...

- Do not define multiple definitions of unit / user unit test case per
  line


  .. code-block:: C

             ztest_test_suite(common,
                              ztest_unit_test(test_sometest1), ztest_unit_test(test_sometest2),
                              ztest_unit_test(test_sometest3),
                              ztest_unit_test(test_sometest4),
             ...


Other questions:

- Why not pre-scan with CPP and then parse? or post scan the ELF file?

  If C pre-processing or building fails because of any issue, then we
  won't be able to tell the subcases.

- Why not declare them in the YAML testcase description?

  A separate testcase description file would be harder to maintain
  than just keeping the information in the test source files
  themselves -- only one file to update when changes are made
  eliminates duplication.

Stress test framework
*********************

Zephyr stress test framework (Ztress) provides an environment for executing user
functions in multiple priority contexts. It can be used to validate that code is
resilient to preemptions. The framework tracks the number of executions and preemptions
for each context. Execution can have various completion conditions like timeout,
number of executions or number of preemptions.

The framework is setting up the environment by creating the requested number of threads
(each on different priority), optionally starting a timer. For each context, a user
function (different for each context) is called and then the context sleeps for
a randomized amount of system ticks. The framework is tracking CPU load and adjusts sleeping
periods to achieve higher CPU load. In order to increase the probability of preemptions,
the system clock frequency should be relatively high. The default 100 Hz on QEMU x86
is much too low and it is recommended to increase it to 100 kHz.

The stress test environment is setup and executed using :c:macro:`ZTRESS_EXECUTE` which
accepts a variable number of arguments. Each argument is a context that is
specified by :c:macro:`ZTRESS_TIMER` or :c:macro:`ZTRESS_THREAD` macros. Contexts
are specified in priority descending order. Each context specifies completion
conditions by providing the minimum number of executions and preemptions. When all
conditions are met and the execution has completed, an execution report is printed
and the macro returns. Note that while the test is executing, a progress report is
periodically printed.

Execution can be prematurely completed by specifying a test timeout (:c:func:`ztress_set_timeout`)
or an explicit abort (:c:func:`ztress_abort`).

User function parameters contains an execution counter and a flag indicating if it is
the last execution.

The example below presents how to setup and run 3 contexts (one of which is k_timer
interrupt handler context). Completion criteria is set to at least 10000 executions
of each context and 1000 preemptions of the lowest priority context. Additionally,
the timeout is configured to complete after 10 seconds if those conditions are not met.
The last argument of each context is the initial sleep time which will be adjusted throughout
the test to achieve the highest CPU load.

  .. code-block:: C

             ztress_set_timeout(K_MSEC(10000));
             ZTRESS_EXECUTE(ZTRESS_TIMER(foo_0, user_data_0, 10000, Z_TIMEOUT_TICKS(20)),
                            ZTRESS_THREAD(foo_1, user_data_1, 10000, 0, Z_TIMEOUT_TICKS(20)),
                            ZTRESS_THREAD(foo_2, user_data_2, 10000, 1000, Z_TIMEOUT_TICKS(20)));

Configuration
=============

Static configuration of Ztress contains:

 - :c:macro:`ZTRESS_MAX_THREADS` - number of supported threads.
 - :c:macro:`ZTRESS_STACK_SIZE` - Stack size of created threads.
 - :c:macro:`ZTRESS_REPORT_PROGRESS_MS` - Test progress report interval.

API reference
*************

Running tests
=============

.. doxygengroup:: ztest_test

Assertions
==========

These macros will instantly fail the test if the related assertion fails.
When an assertion fails, it will print the current file, line and function,
alongside a reason for the failure and an optional message. If the config
option:`CONFIG_ZTEST_ASSERT_VERBOSE` is 0, the assertions will only print the
file and line numbers, reducing the binary size of the test.

Example output for a failed macro from
``zassert_equal(buf->ref, 2, "Invalid refcount")``:

.. code-block:: none

    Assertion failed at main.c:62: test_get_single_buffer: Invalid refcount (buf->ref not equal to 2)
    Aborted at unit test function

.. doxygengroup:: ztest_assert

Mocking
=======

These functions allow abstracting callbacks and related functions and
controlling them from specific tests. You can enable the mocking framework by
setting :kconfig:`CONFIG_ZTEST_MOCKING` to "y" in the configuration file of the
test.  The amount of concurrent return values and expected parameters is
limited by :kconfig:`CONFIG_ZTEST_PARAMETER_COUNT`.

Here is an example for configuring the function ``expect_two_parameters`` to
expect the values ``a=2`` and ``b=3``, and telling ``returns_int`` to return
``5``:

.. literalinclude:: mocking.c
   :language: c
   :linenos:

.. doxygengroup:: ztest_mock

Customizing Test Output
***********************
The way output is presented when running tests can be customized.
An example can be found in :zephyr_file:`tests/ztest/custom_output`.

Customization is enabled by setting :kconfig:`CONFIG_ZTEST_TC_UTIL_USER_OVERRIDE` to "y"
and adding a file :file:`tc_util_user_override.h` with your overrides.

Add the line ``zephyr_include_directories(my_folder)`` to
your project's :file:`CMakeLists.txt` to let Zephyr find your header file during builds.

See the file :zephyr_file:`subsys/testsuite/include/tc_util.h` to see which macros and/or defines can be overridden.
These will be surrounded by blocks such as::

        #ifndef SOMETHING
        #define SOMETHING <default implementation>
        #endif /* SOMETHING */
