#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/aes.h>

typedef unsigned long ip_address;
typedef unsigned short uint16;

static const int BUFFER_SIZE = 16*1024;

// temporary keys for encryption
unsigned char ckey[] =  "thiskeyisverybad";
unsigned char ivec[] = "dontusethisinput";

bool do_crypt = false;

static void UsageExit()
{
   printf("Usage:  Nitro (send/receive) filename [host:Port]\n");
   exit(10);
}

static ip_address GetHostByName(const char * name)
{
   struct hostent * he = gethostbyname(name);
   return ntohl(he ? *((ip_address*)he->h_addr) : 0);
}

// Function for en/decrypting the file. https://tinyurl.com/ywpxj9up
static FILE * doCrypt(bool encrypt, const char * fileName, FILE * fpIn)
{
   int bytes_read, bytes_written;
   unsigned char indata[AES_BLOCK_SIZE];
   unsigned char outdata[AES_BLOCK_SIZE];

   int len = strlen(fileName);
   char outFile[len+4] = "";
   strcat(outFile, "aes_");
   strcat(outFile, fileName);
   
   FILE * fpOut = fopen(outFile, "w");

   /* data structure that contains the key itself */
   AES_KEY key;

   /* set the encryption key */
   AES_set_encrypt_key(ckey, 128, &key);

   /* set where on the 128 bit encrypted block to begin encryption*/
   int num = 0;

   while (1) {
      bytes_read = fread(indata, 1, AES_BLOCK_SIZE, fpIn);

      if (encrypt) // user wants to encypt file
         AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num, AES_ENCRYPT);
      else // user wants to decrypt
         AES_cfb128_encrypt(indata, outdata, bytes_read, &key, ivec, &num, AES_DECRYPT);

      bytes_written = fwrite(outdata, 1, bytes_read, fpOut);
      if (bytes_read < AES_BLOCK_SIZE)
         break;
   }
   if (encrypt)
      printf("Encrypted %d bytes.\n", bytes_written);
   else
      printf("Decrypted %d bytes.\n", bytes_written);
   return fpOut;
}

static void SendFile(ip_address ipAddress, uint16 port, const char * fileName)
{
   FILE * fpIn = fopen(fileName, "r");

   if (do_crypt)
   {
      // encrypt file and then move pointer to the beginning of it to prepare for transfer.
      fpIn = doCrypt(true, fileName, fpIn);
      
      // close and reopen the file to reset the pointer.
      fclose(fpIn);
      fpIn = fopen(fileName, "r");
   }

   if (fpIn)
   {
      int s = socket(AF_INET, SOCK_STREAM, 0);
      if (s >= 0)
      {
         struct sockaddr_in saAddr; memset(&saAddr, 0, sizeof(saAddr));
         saAddr.sin_family      = AF_INET;
         saAddr.sin_addr.s_addr = htonl(ipAddress);
         saAddr.sin_port        = htons(port);

         if (connect(s, (struct sockaddr *) &saAddr, sizeof(saAddr)) == 0)
         {
            printf("Connected to remote host, sending file [%s]\n", fileName);

            char buf[BUFFER_SIZE];
            while(1)
            {
               ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
               if (bytesRead <= 0) break;  // EOF

               printf("Read %i bytes from file, sending them to network...\n", (int)bytesRead);
               if (send(s, buf, bytesRead, 0) != bytesRead)
               {
                  perror("send");
                  break;
               }
            }
         }
         else perror("connect");

         close(s);
      }
      else perror("socket");

      fclose(fpIn);
   }
   else printf("Error, couldn't open file [%s] to send!\n", fileName);
}

static void ReceiveFile(uint16 port, const char * fileName)
{
   int s = socket(AF_INET, SOCK_STREAM, 0);
   if (s >= 0)
   {
#ifndef WIN32
      // (Not necessary under windows -- it has the behaviour we want by default)
      const int trueValue = 1;
      (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
#endif

      struct sockaddr_in saAddr; memset(&saAddr, 0, sizeof(saAddr));
      saAddr.sin_family      = AF_INET;
      saAddr.sin_addr.s_addr = INADDR_ANY;  // (INADDR_ANY) Bind to all interfaces instead of only localhost.
      // saAddr.sin_addr.s_addr = htonl(0);  // (IPADDR_ANY) typo?
      saAddr.sin_port        = htons(port);

      if ((bind(s, (struct sockaddr *) &saAddr, sizeof(saAddr)) == 0)&&(listen(s, 10) == 0))
      {
         memset(&saAddr, 0, sizeof(saAddr));
         socklen_t len = sizeof(saAddr);
         printf("Waiting for incoming TCP connection on port %u, to write file [%s]\n", port, fileName);
         int connectSocket = accept(s, (struct sockaddr *) &saAddr, &len);
         if (connectSocket >= 0)
         {
            FILE * fpIn = fopen(fileName, "w");
            if (fpIn)
            {
               char buf[BUFFER_SIZE];
               while(1)
               {
                  ssize_t bytesReceived = recv(connectSocket, buf, sizeof(buf), 0);
                  if (bytesReceived < 0) perror("recv");  // network error?
                  if (bytesReceived == 0) break;   // sender closed connection, must be end of file

                  printf("Received %i bytes from network, writing them to file...\n", (int) bytesReceived);
                  if (fwrite(buf, 1, bytesReceived, fpIn) != (size_t) bytesReceived)
                  {
                     perror("fwrite");
                     break;
                  }
               }

               if (do_crypt)
               {
                  // reset file pointer
                  fclose(fpIn);
                  fpIn = fopen(fileName, "r");

                  // decrypt file
                  fpIn = doCrypt(false, fileName, fpIn);
               }

               fclose(fpIn);
            }
            else printf("Error, couldn't open file [%s] to receive!\n", fileName);

            close(connectSocket);
         }
      }
      else perror("bind");

      close(s);
   }
   else perror("socket");
}

int main(int argc, char ** argv)
{
   if (argc < 4) UsageExit();

   // the last two arguments are used for encryption purposes.
   if (argc == 6)
   {
      // copy arguments to keys
      memcpy(ckey, argv[4], sizeof ckey);
      memcpy(ivec, argv[5], sizeof ivec);
      do_crypt = true;
   }

#ifdef WIN32
   // Windows requires this to start up the networking API
   WORD versionWanted = MAKEWORD(1, 1);
   WSADATA wsaData;
   (void) WSAStartup(versionWanted, &wsaData);
#else 
   // avoid SIGPIPE signals caused by sending on a closed socket
   signal(SIGPIPE, SIG_IGN);
#endif

   bool isSend = false;
        if (strcmp(argv[1], "send")    == 0) isSend = true;
   else if (strcmp(argv[1], "receive") != 0) UsageExit();

   const char * fileName = argv[2];

   char * token;
   ip_address ip;
   uint16 port = 0;
   if (isSend)
   {
      token = strtok(argv[3], ":");
      ip = GetHostByName(token);
      token = strtok(NULL, "");
      port = atoi(token);
   }
   else
   {
      port = atoi(argv[3]);
   }
   // ip_address ip = isSend ? GetHostByName(argv[3]) : 0;
   // uint16 port = 0;
   // const char * portColon = strchr(argv[3], ':');
   // port = portColon ? atoi(portColon+1) : 0;
   if (port == 0) port = 9999;

   if (isSend) SendFile(ip, port, fileName);
          else ReceiveFile(port, fileName);

   printf("Exiting, bye!\n");
   return 0;
}