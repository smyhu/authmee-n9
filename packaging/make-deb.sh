#!/bin/sh
# make-deb.sh - builds authmee-n9_<VER>_armel.deb from build/authmee + qml/ + packaging/
# The Aegis manifest (_aegis member, as in MeeScan) asks for camera access.
set -e
D=$(cd "$(dirname "$0")/.." && pwd)
VER=${VER:-0.1.0}
OUTDIR=${OUTDIR:-$D/dist}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
P=$T/data; mkdir -p $P/opt/authmee-n9/bin $P/opt/authmee-n9/qml $P/usr/share/applications $P/usr/share/icons/hicolor/80x80/apps $P/usr/share/doc/authmee-n9 $T/ctl
install -m 755 $D/build/authmee $P/opt/authmee-n9/bin/
install -m 644 $D/qml/*.qml $D/qml/*.js $D/qml/*.png $P/opt/authmee-n9/qml/
install -m 644 $D/packaging/authmee-n9.desktop $P/usr/share/applications/
install -m 644 $D/qml/authmee-n9.png $P/usr/share/icons/hicolor/80x80/apps/
{ cat $D/LICENSE; printf '\n\nZBar (QR decoder, built into the program) - GNU LGPL 2.1, see zbar/COPYING in the source.\n'; } > $P/usr/share/doc/authmee-n9/copyright
SIZE=$(du -sk $P | cut -f1)
cat > $T/ctl/control <<CTL
Package: authmee-n9
Version: $VER
Architecture: armel
Maintainer: smyhu <smyhu@users.noreply.github.com>
Installed-Size: $SIZE
Depends: libc6 (>= 2.10), libqtcore4 (>= 4.7.4~), libqtgui4 (>= 4.7.4~), libqt4-declarative (>= 4.7.4~), libqt4-network (>= 4.7.4~), libqtm-multimedia, libssl0.9.8, qt-components (>= 1.0)
Section: user/utilities
Priority: optional
Homepage: https://github.com/smyhu/authmee-n9
Description: Two-step verification codes (TOTP)
 Authenticator for the Nokia N9: scan the QR code shown by a website and
 the app shows a new 6-digit code every 30 seconds. Tap a code to copy it.
 Optional PIN with AES-256 encryption, encrypted backups, clock correction
 from a time server (the phone clock is not changed), import of Google
 Authenticator exports. QR decoding with ZBar. Languages: en, pl.
 Designed and written by Claude (Anthropic) together with smyhu.
Maemo-Display-Name: AuthMee
Maemo-Flags: visible
Aegis-Manifest: included
CTL
printf 'Maemo-Icon-26: %s\n' "$(base64 -w0 $D/qml/authmee-n9.png)" >> $T/ctl/control
(cd $P && find . -type f | sed 's#^\./##' | sort | xargs md5sum) > $T/ctl/md5sums
entry() { printf 'S 15 com.nokia.maemo H 40 %s R %d %s\n' "$1" "${#2}" "$2"; }
(cd $P && find . -type f | sed 's#^\./##' | grep -v '^usr/share/' | sort | while read -r f; do entry "$(sha1sum "$f" | cut -d' ' -f1)" "$f"; done) > $T/ctl/digsigsums
(cd $T/ctl && tar czf ../control.tar.gz --owner=0 --group=0 --numeric-owner --format=gnu ./control ./md5sums ./digsigsums)
(cd $P && tar czf ../data.tar.gz --owner=0 --group=0 --numeric-owner --format=gnu .)
echo 2.0 > $T/debian-binary
cp $D/packaging/authmee-n9.aegis $T/_aegis
mkdir -p $OUTDIR; DEB=$OUTDIR/authmee-n9_${VER}_armel.deb; rm -f $DEB
(cd $T && ar rc $DEB debian-binary control.tar.gz data.tar.gz _aegis)
md5sum $DEB
