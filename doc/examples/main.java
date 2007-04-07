public class main
{
	static
	{
		System.loadLibrary("pacman_java");
	}

	public static void main(String argv[])
	{
		// this will return with success (0)
		System.out.println(pacman.pacman_initialize("/"));
		// this will return with failure (-1) as the lib is already
		// initialized
		System.out.println(pacman.pacman_initialize("/"));
	}
}
