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

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501  /* Windows XP. */
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include "vmci_sockets.h"
  #include "windows_error.h"
  SERVICE_STATUS          g_ServiceStatus = {0};
  SERVICE_STATUS_HANDLE   g_StatusHandle = NULL;
  HANDLE                  g_ServiceStopEvent = INVALID_HANDLE_VALUE;
  HANDLE hThread;
  #define SERVICE_NAME  "vtunnel"
  #define sock_error windows_error
#else
  #include <sys/ioctl.h>
  #include <sys/signal.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  /* vmci_sockets removed in favor of more generic vm_scockets */
  //#include "vmci_sockets.h"
  #include <linux/vm_sockets.h>
  #define IOCTL_VMCI_SOCKETS_GET_AF_VALUE    0x7b8
  #include <sys/utsname.h>
  /** ESXi 5.5 and 6.0 do not have 2.24 so we need to link older one */
  __asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
  #define sock_error perror
#endif

/** Buffer size for VMX config file */
#define LINE_BUF        1024

void print_help(int);
unsigned int parse_vmx(char *, char *);
unsigned int get_dst_cid(char *);
int close_socket(int);
void *process_tunnel_win(void *);
void process_tunnel(int, int);
void *client_handler(void *);
unsigned int ip2ui(char *);
char *ui2ip(unsigned int);
unsigned int createBitmask(const char *);
int check_address(char *);
int compare_address(char *);
void *server_handler(void *);
void *run_as_server(void *);
void *run_as_client(void *);
void kill_handler(void);

