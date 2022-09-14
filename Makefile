files := main.c

all: fo

fo: $(files)
	gcc $(files) -o fo

test: fo
	./make_fake_dir
	mv fo test_dir
	cd test_dir && ./fo -D

clean:
	rm -rf test_dir
	rm -f fo
