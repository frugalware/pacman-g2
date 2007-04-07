#!/usr/bin/perl -w

use Pacman::Core;

Pacman::Core::pacman_initialize("/");
my $db = Pacman::Core::pacman_db_register("local");
my $lp = Pacman::Core::pacman_db_getpkgcache($db);
my $pkg = Pacman::Core::void_to_PM_PKG(Pacman::Core::pacman_list_getdata($lp));
# this will print the name of the first package in the package cache
# same as the output of pacman-g2 |head -n 1
print Pacman::Core::void_to_char(Pacman::Core::pacman_pkg_getinfo($pkg, $Pacman::Core::PM_PKG_NAME)) . "\n";
