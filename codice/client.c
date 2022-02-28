#include "greenpass.c"


//  $ ./client <center-address> <center-port> <code>
int main(int argc, char** argv) {

  /* Variable definitions */
  int clientSocket, nwrite, nread;
  struct sockaddr_in covidCenterAddress;
  struct packet response;
  char cardString[1024];
  char buffer[INET6_ADDRSTRLEN];
  char **alias;
  struct hostent *data;

  if (argc != 4) {
    printf("Usage: %s <center-ip> <center-port> <fiscal_code>\n", argv[0]);
    exit(1);
  } else if (strlen(argv[3]) > MAXIMUM_FISCAL_CODE_LENGTH) {
    printf("Usage: %s <center-ip> <center-port> <fiscal_code>\n", argv[0]);
    printf("<fiscal_code> too long.. maximum fiscal_code length: %d\n", MAXIMUM_FISCAL_CODE_LENGTH);
    exit(-1);
  }

  /* setup TCP client socket */
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0) {
    perror("SOCKET: error\n");
    exit(1);
  }

  /* fill covidCenterAddress struct */
  covidCenterAddress.sin_port = htons(atoi(argv[2]));
  covidCenterAddress.sin_family = AF_INET;

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
  if (inet_pton(AF_INET, buffer, &covidCenterAddress.sin_addr) <= 0) {
    perror("INET_PTON: error\n");
    printf("inet_pton error: closing process...\n");
    exit(1);
  }


  /* connect to covidCenterAddress */
  if (connect(clientSocket, (struct sockaddr *) & covidCenterAddress, sizeof(covidCenterAddress)) < 0) {
      perror("CONNECT: error\n");
      exit(1);
  }
  printf("[+] Client connected to (%s:%d)\n", inet_ntoa(covidCenterAddress.sin_addr), (int) ntohs(covidCenterAddress.sin_port));


  /*
    Il client invia una stringa alfanumerica (codice della tessera sanitaria) al centro vaccinale,
    attende la risposta del centro vaccinale.

  */


  /* build client request */
  struct packet *clientRequest = createPacket(argv[3], REGISTER);
  showPacketInfo(*clientRequest);

  /* send client request to centroVaccinale  */
  nwrite = FullWrite(clientSocket, clientRequest, sizeof(struct packet));
  if (nwrite) {
    perror("FULLWRITE: error\n");
    exit(1);
  }
  printf("[+] Request sent to Centro Vaccinale.\n[+] Waiting response...\n");

  /* wait response from centroVaccinale */
  nread = read(clientSocket, &response, sizeof(struct packet));
  if (nread < 0) {
    perror("READ: error\n");
    exit(1);

  }
  printf("[+] Response from Centro Vaccinale received.\n");

  //show green pass information
  printf("[+] Showing information...\n");
  showGreenPass(response);

  //close socket
  close(clientSocket);
  exit(0);
}
