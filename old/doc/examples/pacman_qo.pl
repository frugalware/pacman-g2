use Pacman::Core;

# this does the same as pacman-g2 -Qo
sub getowner
{
	my $what = shift;
	$i = Pacman::Core::pacman_db_getpkgcache($db);
	while($i)
	{
		$pkg = Pacman::Core::void_to_PM_PKG(Pacman::Core::pacman_list_getdata($i));
		$j = Pacman::Core::void_to_PM_LIST(Pacman::Core::pacman_pkg_getinfo($pkg, $Pacman::Core::PM_PKG_FILES));
		while($j)
		{
			if(Pacman::Core::void_to_char(Pacman::Core::pacman_list_getdata($j)) eq $what)
			{
				return Pacman::Core::void_to_char(Pacman::Core::pacman_pkg_getinfo($pkg, $Pacman::Core::PM_PKG_NAME));
			}
			$j = Pacman::Core::pacman_list_next($j);
		}
		$i = Pacman::Core::pacman_list_next($i);
	}
}

Pacman::Core::pacman_initialize("/");
$db = Pacman::Core::pacman_db_register('local');

print getowner("usr/bin/pacman-g2") . "\n";
print getowner("lib/libc.so.6") . "\n";
