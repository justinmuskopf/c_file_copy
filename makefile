all: clean file_generator file_copy
	gcc -o file_copier file_generator file_copy -lrt

clean:
	rm -rf *MB.txt
	rm -rf temp_copy.txt
	rm -rf file_generator file_copy file_copier

file_generator:
	gcc -c -o file_generator file_generator.c -lrt

file_copy: file_generator
	gcc -c -o file_copy file_copy.c -lrt
