/*
 * vtunnel
 *
 * Copyright 2018 Carnegie Mellon University. All Rights Reserved.
 *
 * NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 *
 * Released under a BSD (SEI)-style license, please see license.txt or contact permission@sei.cmu.edu for full terms.
 *
 * [DISTRIBUTION STATEMENT A] This material has been approved for public release and unlimited distribution.  Please see Copyright notice for non-US Government use and distribution.
 *
 * Carnegie Mellon® and CERT® are registered in the U.S. Patent and Trademark Office by Carnegie Mellon University.
 *
 * This Software includes and/or makes use of the following Third-Party Software subject to its own license:
 * 1. open-vm-tools (https://github.com/vmware/open-vm-tools/blob/9369f1d77fdd90f50b60b44f1ba8c8da00ef55ca/open-vm-tools/LICENSE) Copyright 2018 VMware, Inc..
 * 
 * DM18-1221
 */

#include "vtunnel.h"

/** for running as a daemon */
int daemonize;
/** verbose mode */
int verbose;
/** for tracking whether we are on esx */
int esx;
/** for tracking whether we are on windows */
int windows;
/** for accepting vsock connections */
int vsock_port;
/** for af vsock */
int af;
/** for udp or tcp tunnel */
int udp;
/** my local cid */
int my_cid;
/** for stopping loops and threads */
int running;
/** for authentiation of client connections */
char auth_string[32];
/** for tunnel destination IP */
char dst_address[INET_ADDRSTRLEN];
/** for acceptable tunnel destinations */
char accept_string[19];
/** for tunnel destination port */
int dst_port;
/** for destination machine CID */
unsigned int dst_cid;
/** allowed network address */
unsigned int netip;
/** allowed network mask */
unsigned int netmask;
/** whether to run in server mode */
int server;
/** local address to which client will bind */
char local_address[INET_ADDRSTRLEN];
/** local port to which client will bind */
int local_port;
/** name of destination machine */
char dst_name[256];
/** control socket for vsock ioctl */
int vsock_dev_fd;

/**
 *      @brief Prints the CLI help
 *      @param exval - exit code
 *      @return - void, calls exit()
 */
void print_help(int exval)
{
	printf("vtunnel - vsock tunnel\n\n");

	printf("Usage:  vtunnel [--server] [--auth <string>] [--verbose]\n");
	printf("\tvtunnel [--name <vm_name>|--cid <#>] [--tcp|--udp] [--local-ip <ip>]\n");
	printf("\t\t<--local-port <port>|--dst-port <port>> [--dst-ip <ip>]\n");
	printf("\t\t[--auth <string>] [--background]\n\n");

	printf("Options:\n");
	printf("  -h, --help		print this help and exit\n");
	printf("  -v, --verbose		print verbose output\n");
	printf("  -V, --version		print version and exit\n");
	printf("  -s, --server		program will run in server mode\n");
	printf("  -t, --tcp		tunnel will be TCP (default\n");
	printf("  -u, --udp 		tunnel will be UDP\n");
	printf("  -b, --background	daemonize and return bound port\n");
	printf("  -l, --local-ip	local bind address for the tunnel\n");
	printf("  -p, --local-port	local bind port for the tunnel\n");
	printf("  -n, --name		name of destination machine\n");
	printf("  -c, --cid		cid of destination machine\n");
	printf("  -r, --dst-ip		tunnel destination address\n");
	printf("  -d, --dst-port	tunnel destination port\n");
	printf("  -a, --auth		authentication string\n");
	printf("  -A, --accept		acceptable destination\n\n");


	printf("Copyright 2018 Carnegie Mellon University. All Rights Reserved.\n\n");
	printf("NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING INSTITUTE MATERIAL IS FURNISHED ON AN \"AS-IS\" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.\n\n");
	printf("Released under a BSD (SEI)-style license, please see LICENSE.txt or contact permission@sei.cmu.edu for full terms.\n\n");
        printf("Report bugs to <arwelle@cert.org>\n\n");

	_exit(exval);
}

/*
 *      @brief Parse the vmx file for cid
 *      @param vmx - pointer to path of vmx file
 *      @param name - pointer to machine name
 *      @return unsigned int cid
 */
unsigned int parse_vmx(char *vmx, char *name)
{
	FILE *fp;
	unsigned int cid;
	int value;
	char cid_string[16];
	char line[LINE_BUF];
	int line_len;
	char *path;
	int found;
	char *filename;
	char tempname[256];
	char *space;
	char *beg;
	char *end;
	char vmx_file[1024];

	cid = 1;
	value = 0;
	line_len = 0;
	memset(line, 0, LINE_BUF);
	memset(cid_string, 0, 16);
	memset(vmx_file, 0, 1024 );
	found = 0;

	filename = malloc(strnlen(vmx, 1024));
	if (filename == NULL) {
		perror("vtunnel: malloc");
		_exit(EXIT_FAILURE);
	}
	strncpy(filename, vmx, strnlen(vmx, 1024));

	if (verbose)
		printf("vtunnel: checking vmx %s\n", vmx);

	/* test if this vm name is a dup with number like (1) */
	space = strchr(name, ' ');
	if (space) {
		strncpy(tempname, name, space - name);
	} else {
		strncpy(tempname, name, strnlen(name, 256));
	}

	/* we only checks for dups on esx, not windows */
#ifndef _WIN32
	path = strtok(filename, "/");
	while (path) {
		/* use temp name without dup number to test vmx filename */
		if (strncmp(path, tempname, strnlen(tempname, 256)) == 0) {
			found = 1;
			break;
		} else if (verbose) {
			printf("%s %s\n", path, tempname);
		}
		path = strtok(NULL, "/");
	}

	/* this may not be the correct for dups yet... */
	if (!found)
		return cid;
#endif

	/* reset found for proper display name check in file */
	found = 0;

	/* remove quotes from vmx file name (windows issue) */
	int vmx_size;
	int pos;
	int newpos;

	vmx_size = strnlen(vmx, 1024);
	newpos = 0;

	for (pos = 0; pos < vmx_size; pos++) {
		if (vmx[pos] == '\"') {
			continue;
		}
		vmx_file[newpos] = vmx[pos];
		newpos++;
	}

	fp = fopen(vmx_file, "r");

	if (!fp) {
		perror("vtunnel: fopen");
		printf("vtunnel: could not open %s\n", vmx_file);
		return cid;
	} else if (verbose) {
		printf("vtunnel: parsing %s\n", vmx);
	}

	while (fgets(line, LINE_BUF, fp)) {
		if (strncmp(line, "displayName = ", 14) == 0 ) {
			beg = strchr(line, '"') + 1;
			end = strrchr(line, '"');
			line_len = strnlen(line, LINE_BUF);
			if (strncmp(beg, name, end - beg) == 0) {
				found = 1;
			} else {
				cid = 1;
				break;
			}
		} else if (strncmp(line, "vmci0.id = ", 11) == 0) {
			line_len = strnlen(line, LINE_BUF);
			strncpy(cid_string, line + 12, line_len - 14);
			if (sscanf(cid_string, "%d", &value) == 1) {
				cid = (unsigned int)value;
			} else {
				printf("vtunnel: unknown error parsing cid\n");
				cid = 1;
			}
			if (found)
				break;
		}
	}
	/* this may not be the correct for dups yet... */
	if (!found)
		return cid;

	/* check if fp is still valid before close? */
	if (!fp)
		printf("vtunnel: something happened to fp\n");
	else
		if (fclose(fp) == EOF)
			perror("vtunnel: fclose");
	return cid;
}

/*
 *      @brief Get cid of machine with given name
 *      @param name - name of vm
 *      @return unsigned int cid
 *	TODO: implement a lookup when using qemu
 */
unsigned int get_dst_cid(char *name)
{
	unsigned int cid;
	FILE *pipe;
	char line[LINE_BUF];
	char vmx[LINE_BUF];
	int line_len;
	int ret;

	ret = 0;
	cid = 1;
	line_len = 0;
	memset(line, 0, LINE_BUF);

#ifdef _WIN32
	/*
	 * the vmrun.exe command returns the list of running vms for the
	 * user which ran vmrun.exe. In my case, where I run wmasterd as the
	 * administrator and the VMs as a user, this fails. When running as the
	 * same user, an error is currently received when attempting to fopen
	 * the vms file produced by vmrun.exe
	 */

	pipe = popen("\"C:/Program Files (x86)/VMware/VMware Workstation/vmrun.exe\" list", "rt");

#else
	if (esx) {
		pipe = popen("/bin/esxcli vm process list", "r");
	} else {
		printf("vtunnel: only esx hosts are processed right now\n");
		return cid;
	}
#endif

	if (!pipe) {
		perror("vtunnel: pipe");
		return cid;
	}

	while (fgets(line, LINE_BUF, pipe)) {
#ifdef _WIN32
		/* continue if cid found or first line of output */
		if ((cid != 1) || (strncmp(line, "Total", 5) == 0))
			continue;
		line[strnlen(line, LINE_BUF) - 1] = '\0';
		snprintf(vmx, LINE_BUF, "\"%s\"", line);
		cid = parse_vmx(vmx, name);
		if (verbose)
			printf("vtunnel: parse_vmx returned\n");
#else
		/* process if cid not yet found and this is config file line */
		if ((cid == 1) &&
				(strncmp(line, "   Config File:", 15) == 0)) {
			line_len = strnlen(line, LINE_BUF);
			memset(vmx, 0, LINE_BUF);
			strncpy(vmx, line + 16, line_len - 17);
			cid = parse_vmx(vmx, name);
			if (cid != 1)
				break;
		}
#endif
	}

#ifdef _WIN32
	ret = _pclose(pipe);
#else
	ret = pclose(pipe);
#endif

	if (ret < 0)
		perror("vtunnel: pclose");

	return cid;
}

/**
 *	@brief closes socket and handles windows/linux
 */
int close_socket(int fd)
{
	int ret;
#ifdef _WIN32
	ret = closesocket(fd);
#else
	ret = close(fd);
#endif
	return ret;
}

#ifdef _WIN32
/**
 *	@brief process tunnels data
 *	relays data between two sockets
 */
void *process_tunnel_win(void *args)
{
	int *socks = (int *)args;
	int sock1 = *socks;
	int sock2 = socks[1];
	int bytes;
	char buf[8192];

	/* while read from vsock, write to ip socket */
	while (running) {
		bytes = recv(sock1, buf, 8192, 0);
		if (bytes < 0) {
			sock_error("vtunnel: recv");
			close_socket(sock2);
			return ((void *)0);
		} else if (bytes == 0) {
			printf("vtunnel: socket closed\n");
			close_socket(sock2);
			return ((void *)0);
		}
		if (verbose)
			printf("vtunnel: bytes read: %d\n", bytes);

		bytes = send(sock2, buf, bytes, 0);
		if (bytes < 0) {
			sock_error("vtunnel: send");
			close_socket(sock1);
			return ((void *)0);
		} else if (bytes == 0) {
			printf("vtunnel: socket closed\n");
			close_socket(sock1);
			return ((void *)0);
		}
		if (verbose) {
			printf("vtunnel: bytes sent: %d\n", bytes);
		}
	}

	return ((void *)0);
}
#else
/**
 *      @brief process tunnels data
 *      relays data between two sockets
 */
void process_tunnel(int inet_sock, int vsock_sock)
{
	fd_set rfds;
	struct timeval tv;
	int nfds;
	int retval;
	int bytes;
	char buf[8192];

	if (inet_sock > vsock_sock)
		nfds = inet_sock + 1;
	else
		nfds = vsock_sock + 1;

	/* while read from vsock, write to ip socket */
	while (running) {
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&rfds);
		FD_SET(inet_sock, &rfds);
		FD_SET(vsock_sock, &rfds);

		retval = select(nfds, &rfds, NULL, NULL, &tv);
		if (retval < 0) {
			perror("vtunnel: select");
			continue;
		}

		if (FD_ISSET(inet_sock, &rfds)) {
			bytes = recv(inet_sock, buf, 8192, 0);
			if (bytes < 0) {
				sock_error("vtunnel: recv");
				return;
			} else if (bytes == 0) {
				if (verbose)
					printf("vtunnel: inet_sock closed\n");
				return;
			}
			if (verbose)
				printf("vtunnel: inet_sock  bytes read: %d\n",
					bytes);
			bytes = send(vsock_sock, buf, bytes, 0);
			if (bytes < 0) {
				sock_error("vtunnel: recv");
				return;
			} else if (bytes == 0) {
				if (verbose)
					printf("vtunnel: vsock_sock closed\n");
				return;
			}
			if (verbose) {
				printf("vtunnel: vsock_sock bytes sent: %d\n",
					bytes);
			}
		}
		if (FD_ISSET(vsock_sock, &rfds)) {
			bytes = recv(vsock_sock, buf, 8192, 0);
			if (bytes < 0) {
				sock_error("vtunnel: recv");
				return;
			} else if (bytes == 0) {
				if (verbose)
					printf("vtunnel: vsock_sock closed\n");
				return;
			}
			if (verbose)
				printf("vtunnel: vsock_sock bytes read: %d\n",
					bytes);
			bytes = send(inet_sock, buf, bytes, 0);
			if (bytes < 0) {
				sock_error("vtunnel: recv");
				return;
			} else if (bytes == 0) {
				if (verbose)
					printf("vtunnel: inet_sock closed\n");
				return;
			}
			if (verbose) {
				printf("vtunnel: inet_sock  bytes sent: %d\n",
					bytes);
			}
		}
	}
	if (verbose)
		printf("vtunnel: process tunnel exiting\n");
}
#endif

int connect_vsock(int vsock_client_fd, struct sockaddr_vm server_vm,
	char *dst_address, int dst_port)
{
	char buf[1024];
	int bytes;
	int ret;

	/* connect to vsock server (remote machine) on vsock client fd */
	ret = connect(vsock_client_fd, (struct sockaddr *)&server_vm,
		sizeof(struct sockaddr));
	if (ret < 0) {
		printf("vtunnel: could not connect to vsock server\n");
		sock_error("vtunnel: connect");
		return -1;
	} else if (verbose) {
		printf("vtunnel: connect success to vsock server\n");
	}

	memset(buf, 0, 1024);

	if (udp) {
		snprintf(buf, 1024, "%s %s %d udp",
			auth_string, dst_address, dst_port);
	} else {
		snprintf(buf, 1024, "%s %s %d tcp",
			auth_string, dst_address, dst_port);
	}

	bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);
	if (bytes < 0) {
		sock_error("vtunnel: send");
		return -1;
	}
	if (verbose) {
		printf("vtunnel: sent %d bytes destination line: '%s'\n",
			bytes, buf);
	}

	memset(buf, 0, 1024);

	/* this blocks until the server replies back */
	bytes = recv(vsock_client_fd, buf, 1024, 0);
	if (bytes < 0) {
		sock_error("vtunnel: recv");
		return -1;
	} else if (bytes == 5)  {
		if (strncmp(buf, "ready", 5) != 0) {
			printf("vtunnel: remote connect failed\n");
			return -1;
		}
	} else if (bytes == 4) {
		if (strncmp(buf, "addr", 4) == 0) {
			printf("vtunnel: remote addr failed\n");
			return -1;
		} else if (strncmp(buf, "auth", 4) == 0) {
			printf("vtunnel: remote auth failed\n");
			return -1;
		} else if (strncmp(buf, "fail", 4) == 0) {
			printf("vtunnel: remote destination line failed to parse\n");
			return -1;
		}
	} else {
		printf("vtunnel: invalid notification size %d\n", bytes);
		return -1;
	}

	if (verbose)
		printf("vtunnel: read %d byte notification '%s'\n", bytes, buf);

	/* TODO: determine how long we should allow this to hang */
	return 0;
}


/**
 *	@brief handle ip connections that initiate tunnels
 */
void *client_handler(void *arg)
{

	/** client vsock fd that connects to vsock server */
	int vsock_client_fd;
	/** sockaddr_vm for vsock server */
	struct sockaddr_vm server_vm;
	/** connection from local ip client */
	int local_client_fd;

	/* set vsock server address */
	memset(&server_vm, 0, sizeof(server_vm));
	server_vm.svm_cid = dst_cid;
	server_vm.svm_port = vsock_port;
	server_vm.svm_family = af;

	local_client_fd  = *(int *)arg;

	if (local_client_fd < 0)
	{
		printf("vtunnel: client_handler was passed invalid local_client_fd\n");
		return ((void *)0);
	}

	/* create vsock */
	vsock_client_fd = socket(af, SOCK_STREAM, 0);
	if (vsock_client_fd < 0) {
		sock_error("vtunnel: socket");
		printf("vtunnel: could not create vsock client socket\n");
		close_socket(local_client_fd);
		return ((void *)0);
	}

	if (connect_vsock(vsock_client_fd, server_vm,
		dst_address, dst_port) != 0) {
		close_socket(local_client_fd);
		close_socket(vsock_client_fd);
		printf("vtunnel: could not setup tunnel\n");
		return ((void *)0);
	}

	/* foward from ip client to vsock server */

#ifdef _WIN32
	pthread_t inet_thread;
	pthread_t vsock_thread;
	int ret;
	int inet_args[2];
	int vsock_args[2];
	inet_args[0] = local_client_fd;
	inet_args[1] = vsock_client_fd;
	ret = pthread_create(&inet_thread, NULL,
		process_tunnel_win, (void *)&inet_args);
	if (ret < 0) {
		perror("vtunnel: pthread_create");
		/* close client connection */
		close_socket(vsock_client_fd);
		close_socket(local_client_fd);
		return ((void *)0);
	}
	vsock_args[0] = vsock_client_fd;
	vsock_args[1] = local_client_fd;
	ret = pthread_create(&vsock_thread, NULL,
		process_tunnel_win, (void *)&vsock_args);
	if (ret < 0) {
		perror("vtunnel: pthread_create");
		/* close client connection */
		close_socket(vsock_client_fd);
		close_socket(local_client_fd);
		return ((void *)0);
	}

	// wait for child threads to exist
	pthread_join(inet_thread, NULL);
	pthread_join(vsock_thread, NULL);
#else
	process_tunnel(local_client_fd, vsock_client_fd);

	close_socket(local_client_fd);
	close_socket(vsock_client_fd);

#endif

	printf("vtunnel: finished processing tunnel, thread exiting\n");

	return ((void *)0);
}


/**
 *      @brief check for valid ip address string
 *	this will also set the netip and netmask
 *      @param address_string - string from user
 *      @return success of failure
 */
int check_address(char *address_string)
{
	char *ip;
	char *cidr;
	int prefix;

	ip = strtok(address_string, "/");
	cidr = strtok(NULL, "\0");

	if ((!ip) || (!cidr)) {
		printf("vtunnel: could not parse address string\n");
		return -1;
	}
#ifndef _WIN32
	if (!inet_pton(AF_INET, ip, &netip)) {
		printf("vtunnel: invalid ip given\n");
		return -1;
	}
#else
	struct sockaddr_in winip;
	int size = sizeof(winip);
	if (WSAStringToAddress(ip, AF_INET, NULL, (struct sockaddr *)&winip, &size)) {
		printf("vtunnel: invalid ip given\n");
		sock_error("vtunnel: WSAStringToAddress");
		return -1;
	}
	printf("%d\n", winip.sin_addr.s_addr);
	netip = winip.sin_addr.s_addr;
#endif
	prefix = atoi(cidr);
	if ((prefix < 0) || (prefix > 32)) {
		printf("vtunnel: invalid prefix given\n");
		return -1;
	}

	netmask = htonl(~((1 << (32 - prefix)) - 1));

	return 0;
}

/**
 *      @brief check whether requested destination is in acceptable range
 *      @param address - string from client
 *      @return good or bad
 */
int compare_address(char *address)
{
	unsigned int ip;
#ifndef _WIN32
	if (!inet_pton(AF_INET, address, &ip)) {
		printf("vtunnel: invalid address passed\n");
		return -1;
	}
#else
	struct sockaddr_in winip;
	int size = sizeof(winip);
	if (WSAStringToAddress(address, AF_INET, NULL, (struct sockaddr *)&winip, &size)) {
		printf("vtunnel: invalid address passed\n");
		sock_error("vtunnel: WSAStringToAddress");
		return -1;
	}
	ip = winip.sin_addr.s_addr;
#endif
	if ((netip & netmask) == (ip & netmask))
		return 0;
	else
		return -1;
}

/**
 *	@brief handles vsock connections that request tunnels
 */
void *server_handler(void *arg)
{
	char buf[1024];
	int bytes;
	char dst_address[INET_ADDRSTRLEN];
	int dst_port;
	struct sockaddr_in dst_server;
	int dst_server_fd;
	int ret;
	int vsock_client_fd;
	char proto[4];
	char auth[32];

	vsock_client_fd = *(int *)arg;
	memset(buf, 0, 1024);
	memset(&dst_server, 0, sizeof(struct sockaddr_in));
	dst_server_fd = -1;

	if (vsock_client_fd < 0)
	{
		printf("vtunnel: socket passed invalid");
		_exit(EXIT_FAILURE);
	}

	/* read destination */
	bytes = recv(vsock_client_fd, buf, 1024, 0);
	if (bytes < 0) {
		sock_error("vtunnel: recv");
		close_socket(vsock_client_fd);
		return ((void *)0);
	} else if (bytes == 0) {
		if (verbose)
			printf("vtunnel: no bytes read from vsock\n");
		close_socket(vsock_client_fd);
		return ((void *)0);
	} else if (verbose) {
		printf("vtunnel: read %d bytes from vsock\n", bytes);
	}

	memset(proto, 0, 4);
	memset(auth, 0, 32);

	ret = sscanf(buf, "%s %s %d %s", auth, dst_address, &dst_port, proto);
	if (ret != 4) {
		printf("vtunnel: did not receive valid destination\n");

		memset(buf, 0, 1024);
		snprintf(buf, 1024, "fail");
		bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);

		if (bytes != 4) {
			printf("vtunnel: could not send notification\n");
		} else if (verbose) {
			printf("vtunnel: sent %d bytes notification\n", bytes);
		}
		close_socket(vsock_client_fd);
		return ((void *)0);
	}

	/* TODO: validate the desination IP is acceptable
	 * per config file or come command line arg
	 */
	if (compare_address(dst_address) != 0) {
		printf("vtunnel: destination address invalid or not allowed %s\n",
			dst_address);

		memset(buf, 0, 1024);
		snprintf(buf, 1024, "addr");
		bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);

		if (bytes != 4) {
			printf("vtunnel: could not send notification\n");
			close_socket(vsock_client_fd);
			return ((void *)0);
		} else if (verbose) {
			printf("vtunnel: sent %d bytes notification\n", bytes);
		}
		close_socket(vsock_client_fd);
		return ((void *)0);
	}

	if (strncmp(auth_string, auth, 32) != 0) {
		printf("vtunnel: invalid auth string %s\n", auth);

		memset(buf, 0, 1024);
		snprintf(buf, 1024, "auth");
		bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);

		if (bytes != 4) {
			printf("vtunnel: could not send notification\n");
			close_socket(vsock_client_fd);
			return ((void *)0);
		} else if (verbose) {
			printf("vtunnel: sent %d bytes notification\n", bytes);
		}
		close_socket(vsock_client_fd);
		return ((void *)0);
	}

	if (strncmp(proto, "udp", 3) == 0)
		udp = 1;
	else
		udp = 0;

	printf("vtunnel: setting up tunnel to %s:%d %s\n",
		dst_address, dst_port, proto);

	dst_server.sin_family = AF_INET;
	dst_server.sin_addr.s_addr = inet_addr(dst_address);
	dst_server.sin_port = htons(dst_port);

	/* setup ip socket */
	if (udp)
		dst_server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	else
		dst_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (dst_server_fd < 0) {
		sock_error("vtunnel: socket");
		printf("vtunnel: could not create ip client socket\n");
		close_socket(vsock_client_fd);
		return ((void *)0);
	}

	/* connect to destination */
	ret = connect(dst_server_fd, (struct sockaddr *)&dst_server,
		sizeof(dst_server));
	if (ret < 0) {
		sock_error("vtunnel: connect");
		close_socket(dst_server_fd);

		// inform client of failure
		memset(buf, 0, 1024);
		snprintf(buf, 1024, "error");
		bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);
		if (bytes != 5) {
			printf("vtunnel: could not send error\n");
		} else if (verbose) {
			printf("vtunnel: sent %d bytes notification\n", bytes);
		}

		close_socket(vsock_client_fd);
		return ((void *)0);
	}
	if (verbose) {
		printf("vtunnel: connection to tunnel destination succeeded\n");
	}

	memset(buf, 0, 1024);
	snprintf(buf, 1024, "ready");

	bytes = send(vsock_client_fd, buf, strnlen(buf, 1024), 0);

	if (bytes != 5) {
		printf("vtunnel: could not send confirmation\n");
		close_socket(vsock_client_fd);
		close_socket(dst_server_fd);
		return ((void *)0);
	} else if (verbose) {
		printf("vtunnel: sent %d bytes notification\n", bytes);
	}

	if (verbose) {
		printf("vtunnel: connection success notification succeeded\n");
	}

	/* forward data from vsock server to destination ip socket */
#ifdef _WIN32
	pthread_t inet_thread;
	pthread_t vsock_thread;
	int inet_args[2];
	int vsock_args[2];
	inet_args[0] = dst_server_fd;
	inet_args[1] = vsock_client_fd;
	ret = pthread_create(&inet_thread, NULL,
		process_tunnel_win, (void *)&inet_args);
	if (ret < 0) {
		perror("vtunnel: pthread_create");
		/* close client connection */
		close_socket(vsock_client_fd);
	}
	vsock_args[0] = vsock_client_fd;
	vsock_args[1] = dst_server_fd;
	ret = pthread_create(&vsock_thread, NULL,
		process_tunnel_win, (void *)&vsock_args);
	if (ret < 0) {
		perror("vtunnel: pthread_create");
		/* close client connection */
		close_socket(vsock_client_fd);
	}

	pthread_join(inet_thread, NULL);
	pthread_join(vsock_thread, NULL);
#else
	process_tunnel(dst_server_fd, vsock_client_fd);

	close_socket(dst_server_fd);
	close_socket(vsock_client_fd);
#endif

	printf("vtunnel: finished processing tunnel, thread exiting\n");

	return ((void *)0);
}

/**
 *	@brief act as a server
 *	the program will listen for a vsock connection that will provide
 *	the destination ip/port for the tunnel's remote endpoint. this will
 *	be used on the guest to process tunnel requests from the host, or on
 *	the host to process tunnel requests from the guests. as such, it must
 *	be able to process multiple requests at once.
 */
void *run_as_server(void *arg)
{
	/** for error checking */
	int ret;
	/** sockaddr_vm for vmci server started here */
	struct sockaddr_vm server_vm;
	/** fd of this vsock server */
	int vsock_server_fd;
	/** needed for accept */
	socklen_t client_len;

	/* setup vsock server */
	vsock_server_fd = socket(af, SOCK_STREAM, 0);
	if (vsock_server_fd < 0) {
		sock_error("vtunnel: socket");
		printf("vtunnel: could not create vsock server socket\n");
		_exit(EXIT_FAILURE);
	}

	/* set our vsock server address */
	memset(&server_vm, 0, sizeof(server_vm));
	server_vm.svm_cid = VMADDR_CID_ANY;
	server_vm.svm_port = vsock_port;
	server_vm.svm_family = af;

	/* bind our vsock server */
	ret = bind(vsock_server_fd, (struct sockaddr *)&server_vm,
		sizeof(struct sockaddr));
	if (ret < 0) {
		sock_error("vtunnel: bind");
		close_socket(vsock_server_fd);
		_exit(EXIT_FAILURE);
	}

	/* listen on vsock server for up backlog of 10 connections */
	ret = listen(vsock_server_fd, 10);
	if (ret < 0) {
		sock_error("vtunnel: listen");
		close_socket(vsock_server_fd);
		_exit(EXIT_FAILURE);
	}

	printf("vtunnel: listening on vsock %u:%d\n",
		my_cid, vsock_port);
	fflush(stdout);

	/* daemonize server */
#ifndef _WIN32
	if (daemonize) {
		pid_t child = fork();
		if (child < 0) {
			perror("vtunnel: fork");
			_exit(EXIT_FAILURE);
		} else if (child > 0) {
			_exit(EXIT_SUCCESS);
		}
		/* now executing in child */
		if (chdir("/") < 0)
			perror("vtunnel: chdir");
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_WRONLY, NULL);
		open("/dev/null", O_WRONLY, NULL);
		open("/dev/null", O_WRONLY, NULL);
	}
#endif

	/* accept loop */
	while (running) {
		fd_set rfds;
		struct timeval tv;
		int nfds;

		nfds = vsock_server_fd + 1;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&rfds);
		FD_SET(vsock_server_fd, &rfds);
		ret = select(nfds, &rfds, NULL, NULL, &tv);

		if (ret < 0) {
			perror("vtunnel: select");
			continue;
		} else if (ret > 0) {
			if (!FD_ISSET(vsock_server_fd, &rfds)) {
				continue;
			} else if (verbose) {
				printf("vtunnel: connection attempt\n");
			}
		} else {
			continue;
		}

		int vsock_client_fd;
		/** sockaddr_vm for vmci client connecting to here */
		struct sockaddr_vm client_vm;

		client_len = sizeof(client_vm);

		/* clear/reset client address information */
		memset(&client_vm, 0, sizeof(client_vm));
		vsock_client_fd = -1;

		/* accept vsock connection */
		vsock_client_fd = accept(vsock_server_fd,
			(struct sockaddr *)&client_vm, &client_len);

		if (vsock_client_fd < 0) {
			printf("vtunnel: vsock accept failed\n");
			sock_error("vtunnel: accept");
			continue;
		} else if (verbose) {
			printf("vtunnel: vsock connection accepted\n");
		}

		/* pass client socket off to handler */
		pthread_t tunnel_thread;
		pthread_attr_t tattr;
		pthread_attr_init(&tattr);
		pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&tunnel_thread, &tattr,
			server_handler, &vsock_client_fd);
 		if (ret < 0) {
 			perror("vtunnel: pthread_create");
			/* close client connection */
			close_socket(vsock_client_fd);
 		}
	}
	if (verbose)
		printf("vtunnel: vsock accept loop closed\n");

	close_socket(vsock_server_fd);

	return ((void *)0);
}

/*
 *	this function is the thread ran by the windows service control manager
 *	and also as a normal function if not running as a windows server.
 *	it also runs as a normal function when using linux, both in forground
 *	and daemonized.
 */
void *run_as_client(void *arg) {
	int ret;
	int bound_local_port;

	/** local ip server */
	struct sockaddr_in local_server;
	/** local ip socket */
	int local_server_fd;
	/* soocket options */
	int sock_opts;

	sock_opts = 1;
	ret = 0;
	bound_local_port = -1;

	/* set local ip socket information */
	memset(&local_server, 0, sizeof(struct sockaddr_in));
	local_server.sin_addr.s_addr = inet_addr(local_address);
	local_server.sin_port = htons(local_port);
	local_server.sin_family = AF_INET;

	/* create local ip socket */
	if (udp)
		local_server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	else
		local_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (local_server_fd < 0) {
		sock_error("vtunnel: socket");
		printf("vtunnel: could not create ip server socket\n");
		_exit(EXIT_FAILURE);
	}

	ret = setsockopt(local_server_fd, SOL_SOCKET, SO_REUSEADDR,
		(const char *)&sock_opts, sizeof(int));
	if (ret < 0) {
		sock_error("vtunnel: setsockopt");
		_exit(EXIT_FAILURE);
	}

	/* bind local ip socket */
	/* TODO: consider a commandline arg to set the max or not doing this */
	while (bound_local_port < 0) {
		ret = bind(local_server_fd, (struct sockaddr *)&local_server,
			sizeof(struct sockaddr));
		if (ret < 0) {
			if (verbose)
				printf("vtunnel: failed to bind %d\n",
					local_port);
			local_port++;
			local_server.sin_port = htons(local_port);
		} else {
			bound_local_port = local_port;
		}
	}

	printf("vtunnel: pid %d listening on tcp %s:%d\n",
		getpid(), local_address, bound_local_port);

	fflush(stdout);

	/* daemonize client */
#ifndef _WIN32
	if (daemonize) {
		if (verbose) {
			printf("vtunnel: does not daemonize when verbose\n"),
			daemonize = 0;
		}

		pid_t child = fork();
		if (child < 0) {
			perror("vtunnel: fork");
			_exit(EXIT_FAILURE);
		} else if (child > 0) {
			_exit(EXIT_SUCCESS);
		}
		/* now executing in child */
		if (chdir("/") < 0)
			perror("vtunnel: chdir");
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_WRONLY, NULL);
		open("/dev/null", O_WRONLY, NULL);
		open("/dev/null", O_WRONLY, NULL);
	}
#endif

	/* listen */
	if (!udp) {
		/* begin to listen on local ip socket */
		ret = listen(local_server_fd, 10);
		if (ret < 0) {
			sock_error("vtunnel: listen");
			_exit(EXIT_FAILURE);
		}

		/* loop for accept */
		while (running) {
			fd_set rfds;
			struct timeval tv;
			int nfds;

			nfds = local_server_fd + 1;
			tv.tv_sec = 10;
			tv.tv_usec = 0;

			FD_ZERO(&rfds);
			FD_SET(local_server_fd, &rfds);

			ret = select(nfds, &rfds, NULL, NULL, &tv);

			if (ret < 0) {
				perror("vtunnel: select");
				continue;
			} else if (ret > 0) {
				if (!FD_ISSET(local_server_fd, &rfds)) {
					continue;
				} else if (verbose) {
					printf("vtunnel: connection attempt\n");
				}
			} else {
				continue;
			}

			/** connection from local ip client */
			int local_client_fd;
			/** local ip client address */
			struct sockaddr_in local_client;
			/** length of local ip client address information */
			socklen_t local_client_len;

			local_client_len = sizeof(local_client);

			/* clear/reset local ip client address information */
			memset(&local_client, 0, sizeof(struct sockaddr_in));
			local_client_fd = -1;

			/* accept client connection on local ip socket */
			local_client_fd = accept(local_server_fd,
				(struct sockaddr *)&local_client,
				&local_client_len);
			if (local_client_fd < 0) {
				sock_error("vtunnel: accept");
				printf("vtunnel: error accepting client connection\n");
				continue;
			}

			printf("vtunnel: received connection on %s:%d tcp\n",
				local_address, local_port);

			pthread_t tunnel_thread;
			pthread_attr_t tattr;
			pthread_attr_init(&tattr);
			pthread_attr_setdetachstate(&tattr,
				PTHREAD_CREATE_DETACHED);
			ret = pthread_create(&tunnel_thread, &tattr,
				client_handler, &local_client_fd);
			if (ret < 0) {
				perror("vtunnel: pthread_create");
				/* close client connection */
				close_socket(local_client_fd);
			}
		}
	} else {
		printf("vtunnel: udp tunnels not yet implemented\n");
	}
	if (verbose)
		printf("vtunnel: tcp accept loop closed\n");

	close_socket(vsock_dev_fd);

	return ((void *)0);
}

#ifndef _WIN32
/**
 *      @brief Signal handler which causes program to exit
 *      @return void - calls exit()
 */
void kill_handler(void)
{
	if (verbose)
		printf("vtunnel: ctrl-c received...\n");

	running = 0;
}
#else
/**
 *	receives and handles requests from the windows service control manager
 */
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode) {
	case SERVICE_CONTROL_STOP :
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/* Perform tasks necessary to stop the service here  */
		running = 0;

		DWORD dwExitCode;
		TerminateThread(hThread, dwExitCode);

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
			OutputDebugString(
				"vtunnel: ServiceCtrlHandler: SetServiceStatus returned error");
		}

		SetEvent(g_ServiceStopEvent);
		break;
	default:
		break;
	}
}

/**
 *	main windows service function
 *	calls run_as_client or run_as_server
 */
VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;
	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
		ServiceCtrlHandler);
	DWORD lpExitCode;

	if (g_StatusHandle == NULL)  {
		goto EXIT;
	}

	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
		OutputDebugString(
			"vtunnel: ServiceMain: SetServiceStatus returned error");
	}

	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (g_ServiceStopEvent == NULL) {
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
			OutputDebugString(
				"vtunnel: ServiceMain: SetServiceStatus returned error");
		}
		goto EXIT;
	}

	/* start thread before listing service as started */
	if (server)
		hThread = CreateThread(NULL, 0, (LPVOID) run_as_server, NULL, 0, NULL);
	else
		hThread = CreateThread(NULL, 0, (LPVOID) run_as_client, NULL, 0, NULL);

	/* if thread started, service is running, wait for termination */
	if (hThread) {
		g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
		g_ServiceStatus.dwCheckPoint = 0;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
			OutputDebugString(
				"vtunnel: ServiceMain: SetServiceStatus returned error");
		}

		/* worker thread exits signaling that the service needs to stop */
		WaitForSingleObject(hThread, INFINITE);

		/* get exit code from main thread */
		GetExitCodeThread(hThread, &lpExitCode);
	} else {
		lpExitCode = 1;
	}

	/* Perform any cleanup tasks */

	CloseHandle(g_ServiceStopEvent);

	/* Tell the service controller we are stopped */
    	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = lpExitCode;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
		OutputDebugString(
			"vtunnel: ServiceMain: SetServiceStatus returned error");
	}

EXIT:
	return;
}
#endif



/**
 *      @brief main function
 */
int main(int argc, char *argv[])
{
	int opt;
	int long_index;
	int host;
	char proto[4];
	int auth;
	int opt_t;
	int opt_n;
	int opt_c;
	int opt_l;
	int opt_p;
	int opt_r;
	int opt_d;
	int opt_A;
	int virtio;

#ifdef _WIN32
	SERVICE_TABLE_ENTRY ServiceTable[] =  {
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
		{NULL, NULL}
	};
#else
	struct utsname uts_buf;
#endif

	static struct option long_options[] = {
		{"help",	no_argument, 0, 'h'},
		{"version",	no_argument, 0, 'V'},
		{"verbose",	no_argument, 0, 'v'},
		{"server",	no_argument, 0, 's'},
		{"tcp",		no_argument, 0, 't'},
		{"udp",		no_argument, 0, 'u'},
		{"background",	no_argument, 0, 'b'},
		{"local-ip",	required_argument, 0, 'l'},
		{"local-port",	required_argument, 0, 'p'},
		{"name",	required_argument, 0, 'n'},
		{"cid",		required_argument, 0, 'c'},
		{"dst-ip",	required_argument, 0, 'r'},
		{"dst-port",	required_argument, 0, 'd'},
		{"auth",	required_argument, 0, 'a'},
		{"accept",	required_argument, 0, 'A'},
		{0, 0, 0, 0}
	};

	daemonize = 0;
	opt_n = 0;
	opt_c = 0;
	opt_t = 0;
	opt_l = 0;
	opt_p = 0;
	opt_r = 0;
	opt_d = 0;
	opt_A = 0;
	auth = 0;
	running = 1;
	long_index = 0;
	verbose = 0;
	udp = 0;
	server = 0;
	my_cid = 1;
	af = -1;
	dst_cid = 1;
	esx = 0;
	virtio = 0;
	host = 0;
	memset(&local_address, 0, INET_ADDRSTRLEN);
	memset(&dst_name, 0, 256);
	memset(&dst_address, 0, INET_ADDRSTRLEN);
	local_port = 0;
	dst_port = 0;
	vsock_port = 6666;
	strncpy(local_address, "127.0.0.1", INET_ADDRSTRLEN);
	strncpy(dst_address, "127.0.0.1", INET_ADDRSTRLEN);
	strncpy(accept_string, "0.0.0.0/0", 19);
	memset(proto, 9, 4);

#ifdef _WIN32
	windows = 1;
#else
	windows = 0;
#endif

	/* deterime if we are a guest machine */
#ifndef _WIN32
	/* use ioctl to get our CID */
	vsock_dev_fd = open("/dev/vsock", O_RDONLY);
	if (vsock_dev_fd < 0) {
		perror("vtunnel: open");
		printf("vtunnel: could not open /dev/vsock\n");
		_exit(EXIT_FAILURE);
	}
	if (ioctl(vsock_dev_fd, IOCTL_VM_SOCKETS_GET_LOCAL_CID, &my_cid) < 0)
		perror("vtunnel: ioctl IOCTL_VM_SOCKETS_GET_LOCAL_CID");
	if (ioctl(vsock_dev_fd, IOCTL_VMCI_SOCKETS_GET_AF_VALUE, &af) < 0)
		perror("vtunnel: ioctl IOCTL_VMCI_SOCKETS_GET_AF_VALUE");

	struct stat buf_stat;
	if (stat("/sys/module/vhost_vsock", &buf_stat) == 0) {
		/* when qemu is used */
		virtio = 1;
	}

	uname(&uts_buf);

	if (strncmp(uts_buf.sysname, "VMkernel", 8) == 0)
		esx = 1;

	if (af == -1) {
		if (verbose)
			printf("vtunnel: could not get af_vsock via ioctl\n");

		/* take a guess */
		if (esx)
			af = 53;
		else
			af = 40;
	}

#else
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(1,1), &wsa_data);
	af = VMCISock_GetAFValue();
	if (af == -1) {
		printf("vtunnel: VMCISock_GetAFValue() fail %d\n", af);
	}
	my_cid = VMCISock_GetLocalCID();
	if (af == -1) {
		printf("vtunnel: VMCISock_GetLocalCID() fail %d\n", my_cid);
	}
#endif

	/* check for host */
	if (esx)
		host = 1;
	else if (virtio && (my_cid == 0))
		host = 1;
	else if (my_cid == 2)
		host = 1;
	else
		host = 0;

	while ((opt = getopt_long(argc, argv, "hVvstubl:p:n:c:r:d:a:A:",
			long_options, &long_index)) != -1) {
		switch (opt) {
		case 'h':
			print_help(EXIT_SUCCESS);
			break;
		case 'V':
			printf("vtunnel: version %s\n", VERSION_STR);
			_exit(EXIT_SUCCESS);
			break;
		case 'v':
			verbose = 1;
			break;
		case 's':
			server = 1;
			break;
		case 't':
			opt_t = 1;
			break;
		case 'u':
			udp = 1;
			break;
		case 'l':
			opt_l = 1;
			strncpy(local_address, optarg, INET_ADDRSTRLEN);
			local_address[15] = '\0';
			break;
		case 'b':
			if (windows) {
				printf("vtunnel: running as service\n");
			}
			daemonize = 1;
			break;
		case 'p':
			opt_p = 1;
			local_port = atoi(optarg);
			break;
		case 'n':
			opt_n = 1;
			strncpy(dst_name, optarg, 256);
			dst_name[255] = '\0';
			break;
		case 'c':
			opt_c = 1;
			dst_cid = atoi(optarg);
			if (dst_cid == 1) {
				printf("vtunnel: cannot set dst_cid to reserved value 1\n");
				_exit(EXIT_FAILURE);
			} else if (host && (dst_cid == 0)) {
				printf("vtunnel: host cannot connect to reserved value 0\n");
				_exit(EXIT_FAILURE);
			}
			break;
		case 'r':
			opt_r = 1;
			strncpy(dst_address, optarg, INET_ADDRSTRLEN);
			dst_address[15] = '\0';
			break;
		case 'd':
			opt_d = 1;
			dst_port = atoi(optarg);
			break;
		case 'a':
			auth = 1;
			strncpy(auth_string, optarg, 32);
			auth_string[31] = '\0';
			break;
		case 'A':
			opt_A = 1;
			strncpy(accept_string, optarg, 19);
			accept_string[18] = '\0';
			if (check_address(accept_string) < 0) {
				printf("vtunnel: invalid address for accept\n");
				print_help(EXIT_FAILURE);
			}
			break;
		}
	}

	if (optind < argc)
		print_help(EXIT_FAILURE);

	if (!host && (opt_n || opt_c)) {
		printf("vtunnel: guest cannot set destination cid\n");
		printf("vtunnel: my cid: %d\n", my_cid);
		print_help(EXIT_FAILURE);
	}

	/* set auth to default value if not set by user */
	if (!auth) {
		strncpy(auth_string, "VTUNNEL", 32);
	}

	if (verbose)
		printf("vtunnel: using auth string %s\n", auth_string);

	/*Handle kill signals*/
#ifndef _WIN32
	signal(SIGINT, (void *)kill_handler);
	signal(SIGTERM, (void *)kill_handler);
	signal(SIGQUIT, (void *)kill_handler);
	signal(SIGHUP, SIG_IGN);
#endif

	/* if server mode, bind vsock and loop until exit */
	if (server) {
		/* validate options */
		if (udp || opt_t || opt_l || opt_p || opt_r || opt_d ||
				opt_n || opt_c) {
			printf("vtunnel: invalid option set for server mode\n");
			print_help(EXIT_FAILURE);
		}

		if (daemonize && verbose && !windows) {
			printf("vtunnel: does not daemonize when verbose\n"),
			daemonize = 0;
		}

#ifdef _WIN32
		if (daemonize) {
			if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
				printf("StartServiceCtrlDispatcher error %ld",
					GetLastError());
				_exit(EXIT_FAILURE);
			}
		} else {
			run_as_server(NULL);
		}
#else
		run_as_server(NULL);
#endif

		if (verbose)
			printf("vtunnel: now exiting server\n");
		_exit(EXIT_SUCCESS);
	} else {
		/* validate options */
		if (opt_A) {
			printf("vtunnel: invalid option set for client mode\n");
			print_help(EXIT_FAILURE);
		}
	}

	/* make sure we get required args */
	if ((!dst_port) && (!local_port)) {
		printf("vtunnel: neither local nor destination port set\n");
		print_help(EXIT_FAILURE);
	} else if (dst_port && (!local_port)) {
		local_port = dst_port;
	} else if (local_port && (!dst_port)) {
		dst_port = local_port;
	}

	if (!host) {
		if (verbose)
			printf("vtunnel: running as a client\n");

		/* determine if we are using vmci or virtio */
#ifndef _WIN32
		if (virtio == 1)
			/* when qemu is used */
			dst_cid = 0;
		else
			/* when vmci is used */
			dst_cid = 2;
#else
		dst_cid = 2;
#endif
	}

	if ((dst_cid == 1) && (strnlen(dst_name, 256) > 0)) {
		if (verbose)
			printf("vtunnel: dst name %s\n", dst_name);
		/* lookup cid (only when on host) */
		dst_cid = get_dst_cid(dst_name);
		if (dst_cid == 1) {
			printf("vtunnel: could not determine destination\n");
			_exit(EXIT_FAILURE);
		}
	} else if ((dst_cid == 1) && (strnlen(dst_name, 256) <= 0)) {
		printf("vtunnel: could not determine destination machine\n");
		print_help(EXIT_FAILURE);
	}

	if (verbose) {
		printf("vtunnel: connection settings:\n");
		printf("vtunnel:\tlocal_address\t%s\n", local_address);
		printf("vtunnel:\tlocal_port\t%d\n", local_port);
		printf("vtunnel:\tdst_address\t%s\n", dst_address);
		printf("vtunnel:\tdst_port\t%d\n", dst_port);
		printf("vtunnel:\tlocal cid\t%d\n", my_cid);
		printf("vtunnel:\tdst_cid\t\t%d\n", dst_cid);
		printf("vtunnel:\tvsock_port\t%d\n", vsock_port);
		if (udp)
			printf("vtunnel:\tprotocol:\tcp\n");
		else
			printf("vtunnel:\tprotocol:\ttcp\n");
	}


#ifdef _WIN32
	if (daemonize) {
		/* Connects the main thread of a service process to the
		 * service control manager, which causes the thread to be
		 * the service control dispatcher thread for the calling
		 * process.
		 */
		if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
			printf("StartServiceCtrlDispatcher error %ld",
				GetLastError());
			_exit(EXIT_FAILURE);
		}
	} else {
		run_as_client(NULL);
	}
#else
	run_as_client(NULL);
#endif

	if (verbose)
		printf("vtunnel: now exiting client\n");

}



