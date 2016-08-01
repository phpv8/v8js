#!/bin/sh
php -r "echo(preg_replace('/-dev|RC.*/','',PHP_VERSION));"
