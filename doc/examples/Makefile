PACMAN_JAR = /usr/share/java/pacman.jar

compile-java:
	CLASSPATH=$$CLASSPATH:$(PACMAN_JAR) javac main.java

run-java:
	CLASSPATH=$$CLASSPATH:$(PACMAN_JAR) java main

clean:
	rm -f main.class
