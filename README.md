# Introduction
This project provides the ability to tunnel IP traffic through the hypervisor
so that connections can be proxied into or out of virtual machines.

vtunnel - initiates tunnels
vserver - processes tunnels. could be an option to vtunnel

# Usage for vtunnel
```
Usage:  vtunnel [--server] [--auth <string>] [--verbose]
	vtunnel [--name <vm_name>|--cid <#>] [--tcp|--udp] [--local-ip <ip>]
		<--local-port <port>|--dst-port <port>> [--dst-ip <ip>]
		[--auth <string>] [--background]

Options:
  -h, --help		print this help and exit
  -v, --verbose		print verbose output
  -V, --version		print version and exit
  -s, --server		program will run in server mode
  -t, --tcp		tunnel will be TCP (default
  -u, --udp 		tunnel will be UDP
  -b, --background	daemonize and return bound port
  -l, --local-ip	local bind address for the tunnel
  -p, --local-port	local bind port for the tunnel
  -n, --name		name of destination machine
  -c, --cid		cid of destination machine
  -r, --dst-ip		tunnel destination address
  -d, --dst-port	tunnel destination port
  -a, --auth		authentication string
  -A, --accept		acceptable destination
```

# Usage for server mode
```
vtunnel -s [-b]
```
This program will listen on a VSOCK port for a tunnel request to instruct it.
Each tunnel request will be handled by a new thread so that multiple tunnels
can be configured at the same time. It will continue to process an infinite
number of tunnel requests until the process is killed.

It currently listens on VSOCK port 6666.
	
# Usage for client mode
On guest:
```
vtunnel [-d|-p] [-b]
```
On host:
```
vtunnel [-d|-p] [-c|-n] [-b]
```
The client mode will listen on a local IP socket and once a connection is
accepted it will connect to the vtunnel server process using VSOCK. It will pass
the settings for destination address, destination port, and protocol. Once the
server process confirms that the destination of the tunnel is setup on the
server side, the client will forward begin to accept connections on a local IP
socket. The specific port to be bound locally with IP may very from that passed
by the user if the requested port is already bound. The client will continue to
accept additional connections until the process is killed.

If you need two tunnels at once, setup multiple vtunnel client instances.

The local and destination IP addresses will default to 127.0.0.1. The local and
destination ports will default to each other, and as such one of the two is
required.

When running on a guest, the destination machine's CID will be set to the
value for a host, which is 2. When running on a host, the CID must be either
passed as an argument or else the name argument becomes mandatory as it will be
used to look the CID using esxcli or vmrun.exe depoending upon the host's
operating system. On qemu it seems that the host is set as 0 instead on 2, so
on those systems, we set the host CID to 0 instead of 2.

It will output one line indicating the address and procotol on which it bound

# On ESXi
The installation tarball contains a firewall configuration for vtunnel. It will
open 1000 ports on the ESXi host, 10000-10999. Use a port within this range if
you wish to connect to the vtunnel entrance via an external system, such as the
EMMET VM running ansible.
Example:
```
/bin/vtunnel-esx -l 172.16.1.120 -p 10010 -d 22 -n blue.centos6.test -b
```

# On Windows as a service
This executable can run as a Windows service. To install the service:
```
sc create vtunnel binpath= "c:\program files\exercise\vtunnel-x86_64-w64-mingw32.exe --server --background" start= auto
sc failure vtunnel reset= 1 actions= restart/1000
sc start vtunnel
```
or, to create a tunnel outbound from the Windows guest:
```
sc create vtunnel binpath= "c:\program files\exercise\vtunnel-x86_64-w64-mingw32
.exe --dst-port 22 --background" start= auto
sc failure vtunnel reset= 1 actions= restart/1000
sc start vtunnel
```

# Threads on Windows
When running as either a client or a server, there is one main thread that will
always be running. A total of three additional threads per tunnel are used for
processing of the tunnel data. One thread is created to handle the connection
and that thread creates two additonal threads, one for each socket, that will
block on `recv()` until the socket can be read closes. The thread that detects
a closed socket will close the other socket, causing the other thread to exit.
When both of these socket-handing threads exit, the tunnel thread will close.

```
Tunnels to threads:
0	1
1	4
2	7
3	10
```

# Threads on Linux
When running as either a client or a server, there is one main thread that will
always be running. One additional thread is created for each tunnel. This thread
will use `select()` to monitor the sockets for data that can be read. When one
of the sockets closes, the thread will close the other socket and exit.

```
Tunnels to threads:
0	1
1	2
2	3
3	4
```

# Other notes
vtunnel has been tested with one server handling simultaneous connections from
two vtunnel clients on the same guest: port 80 out to a web server and port 3389
to localhost on the host.

# Use Cases
* Host-Guest: Configure systems with ansible via winrm/ssh
* Guest-Host: Log student activity to a central location
* Guest-Host: Allow third party monitoring agents to communicate with server
* Guest-Host: Allow control of NPC solutions such as Ghosts

# TODO
* Adding support for logging to a file
* Implement/test UDP tunneling

Copyright 2018 Carnegie Mellon University. See LICENSE.txt for terms.

