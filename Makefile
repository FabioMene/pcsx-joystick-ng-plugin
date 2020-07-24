all:
	gcc -g3 -O2 -Wall -fPIC -c pcsx-joystick-ng-plugin.c -o pcsx-joystick-ng-plugin.o
	gcc -g3 -O2 -Wall -fPIC -c config.c -o config.o
	gcc -g3 -O2 -fPIC -shared pcsx-joystick-ng-plugin.o config.o -o libjoystick-ng-plugin.so -lc -lpthread

install:
	[ -d ~/.pcsx  ] && cp libjoystick-ng-plugin.so ~/.pcsx/plugins  || true
	[ -d ~/.pcsxr ] && cp libjoystick-ng-plugin.so ~/.pcsxr/plugins || true

clean:
	rm -f *.o *.so

uninstall:
	rm -f ~/.pcsx/plugins/libjoystick-ng-plugin.so
	rm -f ~/.pcsxr/plugins/libjoystick-ng-plugin.so
