#include "greenpass.c"


//  $ ./clientS <serverG-address> <serverG-port> <code>
int main(int argc, char** argv) {

  /* Variable definitions */
  int clientS_Socket, nwrite, nread;
  struct sockaddr_in serverG_Address;
  struct packet response;
  char cardString[1024];
  char buffer[INET6_ADDRSTRLEN];
  char **alias;
  struct hostent *data;

  /* input check */
  if (argc != 4) {
    printf("Usage: %s <serverG-ip> <serverG-port> <fiscal_code>\n", argv[0]);
    exit(1);
  }  else if (strlen(argv[3]) > MAXIMUM_FISCAL_CODE_LENGTH) {
    printf("Usage: %s <serverG-ip> <serverG-port> <fiscal_code>\n", argv[0]);
    printf("<fiscal_code> too long.. maximum fiscal_code length: %d\n", MAXIMUM_FISCAL_CODE_LENGTH);
    exit(-1);
  }

  /* setup TCP client socket */
  clientS_Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientS_Socket < 0) {
    perror("SOCKET: error\n");
    exit(1);
  }

  /* fill serverG_Address struct */
  serverG_Address.sin_port = htons(atoi(argv[2]));
  serverG_Address.sin_family = AF_INET;

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
  if (inet_pton(AF_INET, buffer, &serverG_Address.sin_addr) <= 0) {
    perror("INET_PTON: error\n");
    printf("inet_pton error: closing process...\n");
    exit(1);
  }

  /* connect to serverG_Address */
  if (connect(clientS_Socket, (struct sockaddr *) & serverG_Address, sizeof(serverG_Address)) < 0) {
      perror("CONNECT: error\n");
      exit(1);
  }
  printf("[+] Client connected to (%s:%d)\n", inet_ntoa(serverG_Address.sin_addr), (int) ntohs(serverG_Address.sin_port));

  /*
    Il clientS deve verificare se un greenpass è valido

  */

  /* build client request */
  struct packet *clientRequest = createPacket(argv[3], CHECK);
  showPacketInfo(*clientRequest);

  /* send client request to serverG */
  nwrite = FullWrite(clientS_Socket, clientRequest, sizeof(struct packet));
  if (nwrite) {
    perror("FULLWRITE: error\n");
    exit(1);
  }
  printf("[+] Request sent to serverG.\n[+] Waiting response...\n");

  /* wait response from serverG */
  nread = read(clientS_Socket, &response, sizeof(struct packet));
  if (nread < 0) {
    perror("READ: error\n");
    exit(1);

  }
  printf("[+] Response from serverG received.\n");

  //show green pass information
  printf("[+] Showing information...\n");
  showGreenPass(response);

  //close socket
  close(clientS_Socket);
  exit(0);
}
