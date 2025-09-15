.PHONY: all clean format systemd-service install remove

all: clean
	mkdir -p ./dist && gcc -o ./dist/a36-display ./src/a36-display.c -lsensors -lusb-1.0 -Wall -Wextra -Werror
clean:
	rm -rf ./dist
format:
	clang-format -i -Werror ./src/a36-display.c
systemd-service:
	cp ./templates/a36-display.service.template ./a36-display.service
install: all
	sudo sh ./install-service.sh
remove:
	sudo sh ./remove-service.sh
