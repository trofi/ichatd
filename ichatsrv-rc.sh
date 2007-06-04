#!/bin/sh

case "$1" in
	start)
		echo -n " ichatd START"
		./ichatd
		;;
	vg-start)
		echo -n " ichatd START"
		valgrind --log-file=vg_log --leak-check=full --track-fds=yes \
		./ichatd
		;;
	stop)
		echo -n " ichatd STOP"
		killall ichatd
		echo
		;;
	restart)
		$0 stop
		$0 start
		;;
	*)
		echo "usage: `basename $0` {start|stop|restart}"
		;;
esac
