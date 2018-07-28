#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

YBTCD=${YBTCD:-$SRCDIR/ybd}
YBTCCLI=${YBTCCLI:-$SRCDIR/ybc}
YBTCTX=${YBTCTX:-$SRCDIR/ybt}
YBTCQT=${YBTCQT:-$SRCDIR/qt/ybq}

[ ! -x $YBTCD ] && echo "$YBTCD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
BTCVER=($($YBTCCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for ybd if --version-string is not set,
# but has different outcomes for ybq and ybc.
echo "[COPYRIGHT]" > footer.h2m
$YBTCD --version | sed -n '1!p' >> footer.h2m

for cmd in $YBTCD $YBTCCLI $YBTCTX $YBTCQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${BTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${BTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
