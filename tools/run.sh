#!/bin/sh
X=${X:-$HOME/n9emu}
rm -rf $X/testqml; cp -r $X/authmee/qml $X/testqml; sed -i 's/com.nokia.extras 1.1/com.nokia.extras 1.0/; /onLinkActivated/d' $X/testqml/*.qml
cd $X/tc
export QEMU_LD_PREFIX=$PWD/rt
pkill -f "qemu-arm-static.*authmee" 2>/dev/null
sleep 1
exec timeout ${T:-600} qemu-arm-static -E LD_LIBRARY_PATH=/usr/lib:/lib -E DISPLAY=:9 -E QT_GRAPHICSSYSTEM=raster -E HOME=$X/home -E DBUS_SESSION_BUS_ADDRESS=$(cat $X/dbus.addr) -E AUTHMEE_DEBUG=1 -E AUTHMEE_LANG=${L:-en} -E AUTHMEE_QML=$X/testqml/ -E AUTHMEE_FAKECAM=${FAKECAM} -E AUTHMEE_BACKUP=$X/home/MyDocs/AuthMee $X/authmee/build/authmee > $X/run.log 2>&1
