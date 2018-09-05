#!/bin/sh

if [ -n "$DESTDIR" ] ; then
    case $DESTDIR in
        /*) # ok
            ;;
        *)
            /bin/echo "DESTDIR argument must be absolute... "
            /bin/echo "otherwise python's distutils will bork things."
            exit 1
    esac
    DESTDIR_ARG="--root=$DESTDIR"
fi

echo_and_run() { echo "+ $@" ; "$@" ; }

echo_and_run cd "/home/srisadha/ros/tremor_mech/src/weiss_kms40_emulator"

# ensure that Python install destination exists
echo_and_run mkdir -p "$DESTDIR/home/srisadha/ros/tremor_mech/install/lib/python2.7/dist-packages"

# Note that PYTHONPATH is pulled from the environment to support installing
# into one location when some dependencies were installed in another
# location, #123.
echo_and_run /usr/bin/env \
    PYTHONPATH="/home/srisadha/ros/tremor_mech/install/lib/python2.7/dist-packages:/home/srisadha/ros/tremor_mech/build/lib/python2.7/dist-packages:$PYTHONPATH" \
    CATKIN_BINARY_DIR="/home/srisadha/ros/tremor_mech/build" \
    "/usr/bin/python" \
    "/home/srisadha/ros/tremor_mech/src/weiss_kms40_emulator/setup.py" \
    build --build-base "/home/srisadha/ros/tremor_mech/build/weiss_kms40_emulator" \
    install \
    $DESTDIR_ARG \
    --install-layout=deb --prefix="/home/srisadha/ros/tremor_mech/install" --install-scripts="/home/srisadha/ros/tremor_mech/install/bin"
