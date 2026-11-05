#!/bin/sh
# ui.sh tap PX PY | type TEXT | shot NAME | drag PX PY PX2 PY2  (portrait coordinates 480x854)
export DISPLAY=${D:-:9}
case $1 in
  tap) xdotool mousemove $3 $((480-$2)) click 1;;
  type) xdotool type --delay 150 "$2";;
  key) xdotool key $2;;
  drag) xdotool mousemove $3 $((480-$2)) mousedown 1; for i in 1 2 3 4 5 6 7 8; do xdotool mousemove $(( $3 + ($5-$3)*i/8 )) $(( 480-$2 - ($4-$2)*i/8 )); sleep 0.1; done; xdotool mouseup 1;;
  shot) import -window root ${X:-$HOME/n9emu}/$2.png && python3 -c "
from PIL import Image; im=Image.open('${X:-$HOME/n9emu}/$2.png'); im.rotate(-90,expand=True).save('${X:-$HOME/n9emu}/$2.png')";;
esac
