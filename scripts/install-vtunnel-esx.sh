#!/bin/sh

cp vtunnel.tgz /bootbank/vtunnel.tgz
tar -xzvf /bootbank/vtunnel.tgz -C /
grep vtunnel.tgz /bootbank/boot.cfg >/dev/null
if [ $? -eq 1 ]; then
  echo "Adding vtunnel.tgz to /bootbank/boot.cfg"
  sed -i "s/state.tgz/vtunnel.tgz --- state.tgz/" /bootbank/boot.cfg
  /etc/init.d/vtunnel enable
  esxcli network firewall refresh
fi
/etc/init.d/vtunnel restart
/etc/init.d/vtunnel status
esxcli network firewall refresh
