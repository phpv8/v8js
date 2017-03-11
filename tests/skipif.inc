<?php

if (!extension_loaded('v8js')) {
	die("skip");
}
function skip_if_using_valgrind() {
	// Travis encounters valgrind failures for tests of time and memory limits (even in libv8 5.7 PPA), but not able to reproduce this locally.
	// NOTE: to debug this, try dumping *.mem if `make test` has a non-zero exit code
	// VALGRIND_LAUNCHER doesn't exist for my env in 7.1, but 'TESTS' does
	// Not exactly perfect to check for '-m', but close enough.
	//
	// Travis testing or full testing may use the environment variable TEST_PHP_ARGS
	// But local testing may instead use TESTS
	if (strlen($_ENV['VALGRIND_LAUNCHER'] ?? '') > 0 ||
			in_array('-m', explode(' ', $_ENV['TESTS'] ?? '')) ||
			in_array('-m', explode(' ', $_ENV['TEST_PHP_ARGS'] ?? ''))) {
		die("skip test flaky on valgrind");
	}
}
