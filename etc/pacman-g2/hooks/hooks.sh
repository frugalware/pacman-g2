#!/bin/sh

# Hooks should avoid using external binaries whenever possible.
# There's no telling if they'll be available when a hook is called.

do_fonts()
{
	local _mkfontscale
	local _mkfontdir
	local _fccache
	
	_mkfontscale="/usr/bin/mkfontscale"
	_mkfontdir="/usr/bin/mkfontdir"
	_fccache="/usr/bin/fc-cache"

	for i in /usr/share/fonts/X11/*; do
		if [ -d "$i" ]; then
			if [ -x "$_mkfontscale" ]; then
				"$_mkfontscale" "$i"
			fi
			if [ -x "$_mkfontdir" ]; then
				"$_mkfontdir" "$i"
			fi
			if [ -x "$_fccache" ]; then
				"$_fccache" --force "$i"
			fi
		fi
	done

	return 0
}
