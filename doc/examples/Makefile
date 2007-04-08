PACMAN_JAR = /usr/share/java/pacman.jar

all: compile-java compile-csharp

compile-java:
	CLASSPATH=$$CLASSPATH:$(PACMAN_JAR) javac main.java

run-java:
	CLASSPATH=$$CLASSPATH:$(PACMAN_JAR) java main

compile-csharp:
	mcs /r:/usr/lib/mono/pacman-csharp/pacman-csharp.dll runme.cs

clean:
	rm -f main.class runme.exe
