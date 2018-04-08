
all:
	gcc -Wall -fPIC -c psemu-joystick-ng-plugin.c -o psemu-joystick-ng-plugin.o
	gcc -Wall -fPIC -c config.c -o config.o
	gcc -fPIC -shared psemu-joystick-ng-plugin.o config.o -o libjoystick-ng-plugin.so -lc -lpthread

install:
	[ -d ~/.pcsx  ] && cp libjoystick-ng-plugin.so ~/.pcsx/plugins  || true
	[ -d ~/.pcsxr ] && cp libjoystick-ng-plugin.so ~/.pcsxr/plugins || true

clean:
	rm -f *.o *.so

uninstall:
	rm -f ~/.pcsx/plugins/libjoystick-ng-plugin.so
