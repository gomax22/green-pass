#include "greenpass.c"


//  $ ./centroVaccinale <center-address> <center-port> <server-address> <server-port>
int main(int argc, char** argv) {
  /* variable definitions */
  int centerSocket, serverV_Socket, i, n, max_fd, enable = 1;
  int len, nwrite, nread, fd;
  char fd_open[FD_SETSIZE];
  fd_set fset;
  struct sockaddr_in centerAddress, serverV_Address, clientAddress;
  struct packet bufferPacket;
  struct hostent *addr, *data;
  char buff[4096];
  char buffer[INET6_ADDRSTRLEN];
  char **alias;


  /* input check */
  if (argc != 5) {
    printf("Usage: %s <center-address> <center-port> <server-address> <server-port>\n", argv[0]);
    exit(1);
  }

  /* initialize address */
  memset((void *)&centerAddress, 0, sizeof(centerAddress));   /* clear center address */
  memset((void *) &serverV_Address, 0, sizeof(serverV_Address));


  /* setup TCP centerSocket configuration */
  centerSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (centerSocket < 0) {
    perror("SOCKET: error\n");
    exit(1);
  }
  printf("[+] Setup centerSocket configuration.\n");

  if (setsockopt(centerSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
    perror("SETSOCKOPT: error\n");
    exit(1);
  }

  /* fill centerAddress struct */
  centerAddress.sin_family = AF_INET;
  centerAddress.sin_port = htons(atoi(argv[2]));

  /* gethostbyname */
  if ((data = gethostbyname(argv[1])) == NULL) {
      herror("gethostbyname error\n");
      exit(1);
  }

  /* inet_ntop */
  alias = data->h_addr_list;
  if (inet_ntop(data->h_addrtype, *alias, buffer, sizeof(buffer)) == NULL) {
    fprintf(stderr, "inet_ntop error for %s\n", argv[1]);
    exit(1);
  }

  /* inet_pton for IP address */
  if (inet_pton(AF_INET, buffer, &centerAddress.sin_addr) <= 0) {
    perror("INET_PTON: error\n");
    printf("inet_pton error: closing process...\n");
    exit(1);
  }

  /* bind centerSocket to centerAddress */
  if (bind(centerSocket, (struct sockaddr *) &centerAddress, sizeof(centerAddress)) < 0) {
    perror("BIND: error\n");
    exit(1);
  }
  printf("[+] Bind centerSocket to centerAddress (%s:%u)\n", inet_ntoa(centerAddress.sin_addr), ntohs(centerAddress.sin_port));

  /* listen */
  if (listen(centerSocket, BACKLOG) < 0) {
    perror("LISTEN: error\n");
    exit(1);
  }
  printf("[+] Listening for new connections on port %s\n", argv[2]);

  /* setup TCP serverV_Socket configuration */
  serverV_Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverV_Socket < 0) {
    perror("SOCKET: error\n");
    exit(1);
  }
  printf("[+] Setup serverV_Socket configuration.\n");


  /* fill serverV_Address struct */
  serverV_Address.sin_family = AF_INET;
  serverV_Address.sin_port = htons(atoi(argv[4]));

  /* gethostbyname */
  if ((data = gethostbyname(argv[3])) == NULL) {
      herror("gethostbyname error\n");
      exit(1);
  }

  /* inet_ntop */
  alias = data->h_addr_list;
  if (inet_ntop(data->h_addrtype, *alias, buffer, sizeof(buffer)) == NULL) {
    fprintf(stderr, "inet_ntop error for %s\n", argv[1]);
    exit(1);
  }

  /* inet_pton for IP address */
  if (inet_pton(AF_INET, buffer, &serverV_Address.sin_addr) <= 0) {
    perror("INET_PTON: error\n");
    printf("inet_pton error: closing process...\n");
    exit(1);
  }


  /* connect serverV_Socket to serverV_Address */
  if (connect(serverV_Socket, (struct sockaddr *) &serverV_Address, sizeof(serverV_Address)) < 0) {
    perror("CONNECT: error\n");
    exit(1);
  }
  printf("[+] Connected to serverV (%s:%d)\n",inet_ntoa(serverV_Address.sin_addr), (int) ntohs(serverV_Address.sin_port));


  /* initialize all needed variables */
  memset(fd_open, 0, FD_SETSIZE);   /* clear array of open files */
  max_fd = max(serverV_Socket, centerSocket);                 /* maximum now is listening socket */
  fd_open[serverV_Socket] = 1;
  fd_open[centerSocket] = 1;

  /* main loop, wait for connection and data inside a select */
  while (1) {
  	FD_ZERO(&fset);		/* clear readset */

  	for (i = centerSocket; i <= max_fd; i++) { /* initialize fd_set */
  		if (fd_open[i] != 0)
  			FD_SET(i, &fset);
  	}

    /* monitoring file descriptors in fset */
		while ( ((n = select(max_fd+1, &fset, NULL, NULL, NULL)) < 0) && (errno == EINTR));     /* wait for data or connection */
  	if (n < 0) {                          /* on real error exit */
  		perror("select error");
			exit(1);
  	}

  	/* on activity */
  	if (FD_ISSET(centerSocket, &fset)) {       /* if new connection */
  		n--;                              /* decrement active */
  		len = sizeof(clientAddress);
  		if ((fd = accept(centerSocket, (struct sockaddr *) &clientAddress, &len)) < 0) {
  			perror("accept error");
  			exit(1);
  		}
      if ((addr = gethostbyaddr((const char*)&clientAddress.sin_addr, sizeof(clientAddress.sin_addr), clientAddress.sin_family)) == NULL) {
          herror("gethostbyaddr error\n");
          exit(1);
        }
      inet_ntop(AF_INET, &clientAddress.sin_addr, buff, sizeof(buff));

      printf("\t[+] Connection accepted from client (%s:%d).\n", addr->h_name, ntohs(clientAddress.sin_port));
  		fd_open[fd] = 1;                  /* set new connection socket */
  		if (max_fd < fd) max_fd = fd;     /* if needed set new maximum */
  	}

    /* loop on open connections */
    i = centerSocket;                  /* first socket to look */
    while (n != 0) {              /* loop until active */
    	i++;                      /* start after listening socket */
    	if (fd_open[i] == 0)
    		continue;   /* closed, go next */
    	if (FD_ISSET(i, &fset)) {
    		n--;                         /* decrease active */
    		nread = read(i, &bufferPacket, sizeof(bufferPacket));     /* read operations */
    		if (nread < 0) {
    			perror("Errore in lettura");
    			exit(1);
    		}
    		if (nread == 0) {            /* if closed connection */
          printf("\t[+] Closed connection from other end.\n");
    			close(i);  /*close active socket*/               /* close file */
    			fd_open[i] = 0;          /* mark as closed in table */
    			if (max_fd == i) {       /* if was the maximum */
    				while (fd_open[--i] == 0);    /* loop down */
    				max_fd = i;          /* set new maximum */
    				break;               /* and go back to select */
  			  }
      		  continue;                /* continue loop on open */
      	}


        if (bufferPacket.fd < 0) { /* packet received from a client */

          printf("[+] Computing green pass validity.\n");
          //compute validForMonths
          bufferPacket.clientRequest.greenPassRecord.validForMonths = 6;
          bufferPacket.fd = i;



          printf("[+] Sending client request to serverV.\n");

          //send request to server
          nwrite = FullWrite(serverV_Socket, &bufferPacket, sizeof(struct packet));
          if (nwrite) {
            perror("FULLWRITE: error\n");;
            exit(1);
          }
          printf("[+] Client request sent.\n[+] Waiting server response...\n");


        } else {  /*packet received from serverV */

          printf("[+] Server response received.\n");
          //send response to client
          nwrite = FullWrite(bufferPacket.fd, &bufferPacket, sizeof(struct packet));
          if (nwrite) {
            perror("FULLWRITE: error\n");
            exit(1);
          }
          printf("[+] Response sent to client.\n");
        }
      }
    }
  }
  /* normal exit, never reached */
	exit(0);
}
