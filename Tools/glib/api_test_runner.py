#!/usr/bin/env python
#
# Copyright (C) 2011, 2012, 2017 Igalia S.L.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

import subprocess
import os
import errno
import sys
import re
from signal import alarm, signal, SIGALRM, SIGKILL, SIGSEGV

top_level_directory = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(top_level_directory, "Tools", "glib"))
import common
from webkitpy.common.host import Host


class SkippedTest:
    ENTIRE_SUITE = None

    def __init__(self, test, test_case, reason, bug, build_type=None):
        self.test = test
        self.test_case = test_case
        self.reason = reason
        self.bug = bug
        self.build_type = build_type

    def __str__(self):
        skipped_test_str = "%s" % self.test

        if not(self.skip_entire_suite()):
            skipped_test_str += " [%s]" % self.test_case

        skipped_test_str += ": %s (https://bugs.webkit.org/show_bug.cgi?id=%d)" % (self.reason, self.bug)
        return skipped_test_str

    def skip_entire_suite(self):
        return self.test_case == SkippedTest.ENTIRE_SUITE

    def skip_for_build_type(self, build_type):
        if self.build_type is None:
            return True

        return self.build_type == build_type


class TestTimeout(Exception):
    pass


class TestRunner(object):
    TEST_DIRS = []
    SKIPPED = []
    SLOW = []

    def __init__(self, port, options, tests=[]):
        self._options = options

        self._build_type = "Debug" if self._options.debug else "Release"
        common.set_build_types((self._build_type,))
        self._port = Host().port_factory.get(port)
        self._driver = self._create_driver()

        self._programs_path = common.binary_build_path()
        self._tests = self._get_tests(tests)
        self._skipped_tests = [skipped for skipped in TestRunner.SKIPPED if skipped.skip_for_build_type(self._build_type)]
        self._disabled_tests = []

    def _test_programs_base_dir(self):
        return os.path.join(self._programs_path, "TestWebKitAPI")

    def _get_tests_from_dir(self, test_dir):
        if not os.path.isdir(test_dir):
            return []

        tests = []
        for test_file in os.listdir(test_dir):
            if not test_file.lower().startswith("test"):
                continue
            test_path = os.path.join(test_dir, test_file)
            if os.path.isfile(test_path) and os.access(test_path, os.X_OK):
                tests.append(test_path)
        return tests

    def _get_tests(self, initial_tests):
        tests = []
        for test in initial_tests:
            if os.path.isdir(test):
                tests.extend(self._get_tests_from_dir(test))
            else:
                tests.append(test)
        if tests:
            return tests

        tests = []
        for test_dir in self.TEST_DIRS:
            absolute_test_dir = os.path.join(self._test_programs_base_dir(), test_dir)
            tests.extend(self._get_tests_from_dir(absolute_test_dir))
        return tests

    def _create_driver(self, port_options=[]):
        self._port._display_server = self._options.display_server
        driver = self._port.create_driver(worker_number=0, no_timeout=True)._make_driver(pixel_tests=False)
        if not driver.check_driver(self._port):
            raise RuntimeError("Failed to check driver %s" % driver.__class__.__name__)
        return driver

    def _setup_testing_environment(self):
        self._test_env = self._driver._setup_environ_for_test()
        self._test_env["TEST_WEBKIT_API_WEBKIT2_RESOURCES_PATH"] = common.top_level_path("Tools", "TestWebKitAPI", "Tests", "WebKit")
        self._test_env["TEST_WEBKIT_API_WEBKIT2_INJECTED_BUNDLE_PATH"] = common.library_build_path()
        self._test_env["WEBKIT_EXEC_PATH"] = self._programs_path

        return True

    def _tear_down_testing_environment(self):
        if self._driver:
            self._driver.stop()

    def _test_cases_to_skip(self, test_program):
        if self._options.skipped_action != 'skip':
            return []

        test_cases = []
        for skipped in self._skipped_tests:
            if test_program.endswith(skipped.test) and not skipped.skip_entire_suite():
                test_cases.append(skipped.test_case)
        return test_cases

    def _should_run_test_program(self, test_program):
        for disabled_test in self._disabled_tests:
            if test_program.endswith(disabled_test):
                return False

        if self._options.skipped_action != 'skip':
            return True

        for skipped in self._skipped_tests:
            if test_program.endswith(skipped.test) and skipped.skip_entire_suite():
                return False
        return True

    def _kill_process(self, pid):
        try:
            os.kill(pid, SIGKILL)
        except OSError:
            # Process already died.
            pass

    @staticmethod
    def _start_timeout(timeout):
        if timeout <= 0:
            return

        def _alarm_handler(signum, frame):
            raise TestTimeout

        signal(SIGALRM, _alarm_handler)
        alarm(timeout)

    @staticmethod
    def _stop_timeout(timeout):
        if timeout <= 0:
            return

        alarm(0)

    def _waitpid(self, pid):
        while True:
            try:
                dummy, status = os.waitpid(pid, 0)
                if os.WIFSIGNALED(status):
                    return -os.WTERMSIG(status)
                if os.WIFEXITED(status):
                    return os.WEXITSTATUS(status)

                # Should never happen
                raise RuntimeError("Unknown child exit status!")
            except (OSError, IOError) as e:
                if e.errno == errno.EINTR:
                    continue
                if e.errno == errno.ECHILD:
                    # This happens if SIGCLD is set to be ignored or waiting
                    # for child processes has otherwise been disabled for our
                    # process.  This child is dead, we can't get the status.
                    return 0
                raise

    def _run_test_glib(self, test_program):
        command = ['gtester', '-k']
        if self._options.verbose:
            command.append('--verbose')
        for test_case in self._test_cases_to_skip(test_program):
            command.extend(['-s', test_case])
        command.append(test_program)

        timeout = self._options.timeout
        test = os.path.join(os.path.basename(os.path.dirname(test_program)), os.path.basename(test_program))
        if test in TestRunner.SLOW:
            timeout *= 5

        test_context = {"child-pid": -1, "did-timeout": False, "current_test": None}

        def parse_line(line, test_context=test_context):
            if not line:
                return

            match = re.search(r'\(pid=(?P<child_pid>[0-9]+)\)', line)
            if match:
                test_context["child-pid"] = int(match.group('child_pid'))
                sys.stdout.write(line)
                return

            def set_test_result(test, result):
                if result == "FAIL":
                    if test_context["did-timeout"] and result == "FAIL":
                        test_context[test] = "TIMEOUT"
                    else:
                        test_context[test] = result
                test_context["did-timeout"] = False
                test_context["current_test"] = None
                self._stop_timeout(timeout)
                self._start_timeout(timeout)

            normalized_line = line.strip().replace(' ', '')
            if not normalized_line:
                return

            if normalized_line[0] == '/':
                test, result = normalized_line.split(':', 1)
                if result in ["OK", "FAIL"]:
                    set_test_result(test, result)
                else:
                    test_context["current_test"] = test
            elif normalized_line in ["OK", "FAIL"]:
                set_test_result(test_context["current_test"], normalized_line)

            sys.stdout.write(line)

        pid, fd = os.forkpty()
        if pid == 0:
            os.execvpe(command[0], command, self._test_env)
            sys.exit(0)

        self._start_timeout(timeout)

        while (True):
            try:
                common.parse_output_lines(fd, parse_line)
                break
            except TestTimeout:
                assert test_context["child-pid"] > 0
                self._kill_process(test_context["child-pid"])
                test_context["child-pid"] = -1
                test_context["did-timeout"] = True

        self._stop_timeout(timeout)
        del test_context["child-pid"]
        del test_context["did-timeout"]
        del test_context["current_test"]

        self._waitpid(pid)
        return test_context

    def _get_tests_from_google_test_suite(self, test_program):
        try:
            output = subprocess.check_output([test_program, '--gtest_list_tests'], env=self._test_env)
        except subprocess.CalledProcessError:
            sys.stderr.write("ERROR: could not list available tests for binary %s.\n" % (test_program))
            sys.stderr.flush()
            return 1

        skipped_test_cases = self._test_cases_to_skip(test_program)

        tests = []
        prefix = None
        for line in output.split('\n'):
            if not line.startswith('  '):
                prefix = line
                continue
            else:
                test_name = prefix + line.strip()
                if not test_name in skipped_test_cases:
                    tests.append(test_name)
        return tests

    def _run_google_test(self, test_program, subtest):
        command = [test_program, '--gtest_filter=%s' % (subtest)]
        timeout = self._options.timeout
        if subtest in TestRunner.SLOW:
            timeout *= 5

        pid, fd = os.forkpty()
        if pid == 0:
            os.execvpe(command[0], command, self._test_env)
            sys.exit(0)

        self._start_timeout(timeout)
        try:
            common.parse_output_lines(fd, sys.stdout.write)
            status = self._waitpid(pid)
        except TestTimeout:
            self._kill_process(pid)
            return {subtest: "TIMEOUT"}

        self._stop_timeout(timeout)

        if status == -SIGSEGV:
            sys.stdout.write("**CRASH** %s\n" % subtest)
            sys.stdout.flush()
            return {subtest: "CRASH"}

        if status != 0:
            return {subtest: "FAIL"}

        return {}

    def _run_google_test_suite(self, test_program):
        result = {}
        for subtest in self._get_tests_from_google_test_suite(test_program):
            result.update(self._run_google_test(test_program, subtest))
        return result

    def is_glib_test(self, test_program):
        raise NotImplementedError

    def is_google_test(self, test_program):
        raise NotImplementedError

    def _run_test(self, test_program):
        if self.is_glib_test(test_program):
            return self._run_test_glib(test_program)

        if self.is_google_test(test_program):
            return self._run_google_test_suite(test_program)

        return {}

    def run_tests(self):
        if not self._tests:
            sys.stderr.write("ERROR: tests not found in %s.\n" % (self._test_programs_base_dir()))
            sys.stderr.flush()
            return 1

        if not self._setup_testing_environment():
            return 1

        # Remove skipped tests now instead of when we find them, because
        # some tests might be skipped while setting up the test environment.
        self._tests = [test for test in self._tests if self._should_run_test_program(test)]

        crashed_tests = {}
        failed_tests = {}
        timed_out_tests = {}
        try:
            for test in self._tests:
                results = self._run_test(test)
                for test_case, result in results.iteritems():
                    if result == "FAIL":
                        failed_tests.setdefault(test, []).append(test_case)
                    elif result == "TIMEOUT":
                        timed_out_tests.setdefault(test, []).append(test_case)
                    elif result == "CRASH":
                        crashed_tests.setdefault(test, []).append(test_case)
        finally:
            self._tear_down_testing_environment()

        if failed_tests:
            sys.stdout.write("\nUnexpected failures (%d)\n" % (sum(len(value) for value in failed_tests.itervalues())))
            for test in failed_tests:
                sys.stdout.write("    %s\n" % (test.replace(self._test_programs_base_dir(), '', 1)))
                for test_case in failed_tests[test]:
                    sys.stdout.write("        %s\n" % (test_case))
            sys.stdout.flush()

        if crashed_tests:
            sys.stdout.write("\nUnexpected crashes (%d)\n" % (sum(len(value) for value in crashed_tests.itervalues())))
            for test in crashed_tests:
                sys.stdout.write("    %s\n" % (test.replace(self._test_programs_base_dir(), '', 1)))
                for test_case in crashed_tests[test]:
                    sys.stdout.write("        %s\n" % (test_case))
            sys.stdout.flush()

        if timed_out_tests:
            sys.stdout.write("\nUnexpected timeouts (%d)\n" % (sum(len(value) for value in timed_out_tests.itervalues())))
            for test in timed_out_tests:
                sys.stdout.write("    %s\n" % (test.replace(self._test_programs_base_dir(), '', 1)))
                for test_case in timed_out_tests[test]:
                    sys.stdout.write("        %s\n" % (test_case))
            sys.stdout.flush()

        return len(failed_tests) + len(timed_out_tests)


def add_options(option_parser):
    option_parser.add_option('-r', '--release',
                             action='store_true', dest='release',
                             help='Run in Release')
    option_parser.add_option('-d', '--debug',
                             action='store_true', dest='debug',
                             help='Run in Debug')
    option_parser.add_option('-v', '--verbose',
                             action='store_true', dest='verbose',
                             help='Run gtester in verbose mode')
    option_parser.add_option('--skipped', action='store', dest='skipped_action',
                             choices=['skip', 'ignore', 'only'], default='skip',
                             metavar='skip|ignore|only',
                             help='Specifies how to treat the skipped tests')
    option_parser.add_option('-t', '--timeout',
                             action='store', type='int', dest='timeout', default=10,
                             help='Time in seconds until a test times out')
