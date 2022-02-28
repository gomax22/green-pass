#include "greenpass.c"


//  $ ./clientT <serverG-address> <serverG-port> <code> <operation>
int main(int argc, char** argv) {

  /* Variable definitions */
  int clientT_Socket, nwrite, nread;
  struct sockaddr_in serverG_Address;
  struct packet response;
  char cardString[1024];
  char buffer[INET6_ADDRSTRLEN];
  char **alias;
  struct hostent *data;
  int greenPassOperation = -2;

  /* input check */
  if (argc != 5) {
    printf("Usage: %s <serverG-ip> <serverG-port> <fiscal_code> <operation>\n", argv[0]);
    exit(1);
  } else if (strlen(argv[3]) > MAXIMUM_FISCAL_CODE_LENGTH) {
    printf("Usage: %s <serverG-ip> <serverG-port> <fiscal_code> <operation>\n", argv[0]);
    printf("<fiscal_code> too long.. maximum fiscal_code length: %d\n", MAXIMUM_FISCAL_CODE_LENGTH);
  } else if ((greenPassOperation = computeOperation(argv[4])) == -2) {
    printf("Usage: %s <serverG-ip> <serverG-port> <fiscal_code> <operation>\n", argv[0]);
    printf("[-] Requested an invalid operation. Aborting process...\n");
    exit(-1);
  }

  /* setup TCP client socket */
  clientT_Socket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientT_Socket < 0) {
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
  if (connect(clientT_Socket, (struct sockaddr *) & serverG_Address, sizeof(serverG_Address)) < 0) {
      perror("CONNECT: error\n");
      exit(1);
  }
  printf("[+] Client connected to (%s:%d)\n", inet_ntoa(serverG_Address.sin_addr), (int) ntohs(serverG_Address.sin_port));

  /*
    Il clientT ripristina o annulla la validità di un greenpass a seguito di un contagio od una guarigione

  */

  /* build client request */
  struct packet *clientRequest = createPacket(argv[3], computeOperation(argv[4]));
  showPacketInfo(*clientRequest);

  /* send client request to serverG */
  nwrite = FullWrite(clientT_Socket, clientRequest, sizeof(struct packet));
  if (nwrite) {
    perror("FULLWRITE: error\n");
    exit(1);
  }
  printf("[+] Request sent to serverG.\n[+] Waiting response...\n");

  //wait response
  nread = read(clientT_Socket, &response, sizeof(struct packet));
  if (nread < 0) {
    perror("READ: error\n");
    exit(1);

  }
  printf("[+] Response from serverG received.\n");

  //show green pass information
  printf("[+] Showing information...\n");
  showGreenPass(response);

  //close socket
  close(clientT_Socket);
  exit(0);
}
