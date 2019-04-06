#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


// common errors
#define ARGUMENTS_ERROR 1

void generateHelp(){
    printf("this is a help section\n TBD \n \n");
}

int checkAllowedPortNumbers(char *port){
    char *ptr;
    int num = 0;
    num = (int)strtol(port, &ptr, 10);

    if (*ptr == '\0') {
        // ok, it is a number
        if(num > 0 && num < 65536){
            return num;
        } else return 0;

    }
    else {
        return 0;
    }

}

int validateAndExtractPortNumbers(char *ports, int **portsToScan){


    char actualPort[6];
    int actualPortPosition = 0;
    int numberOfPorts = 0;
    int numberOfPorts2 = 0;


    // first try
    for(int i = 0; i < (int)strlen(ports); i++){
        if(ports[i] >= '0' && ports[i] <= '9'){
            // nothing
        } else {
            if(i == 0 || i == (int)(strlen(ports)-1)){
                // wrong syntax
                return 0;
            }
            if(ports[i] == ','){
                numberOfPorts++;
            } else if(ports[i] == '-'){
                numberOfPorts2++;
            } else {
                return 0;
            }
        }
    }


    if(numberOfPorts == 0 && numberOfPorts2 == 0){
        // only one port
        int *result = malloc(1*sizeof(int));
        result[0] = checkAllowedPortNumbers(ports);
        if(result[0] == 0){
            return 0;
        }
        *portsToScan = result;
        free(result);
        return 1;
    } else if(numberOfPorts > 0 && numberOfPorts2 == 0){
        int pSize = numberOfPorts+1;
        int *result = malloc((pSize)*sizeof(int));
        // divided by ,
        int counter = 0;
        for(int i = 0; i < (int)strlen(ports); i++){
            if(ports[i] >= '0' && ports[i] <= '9'){
                // number
                actualPort[actualPortPosition] = ports[i];
                actualPortPosition++;
                if(i+1 == (int)strlen(ports)){
                    actualPort[actualPortPosition] = '\0';
                    result[counter] = checkAllowedPortNumbers(actualPort);
                    if(result[counter] == 0){
                        return 0;
                    }
                    actualPortPosition = 0;
                    actualPort[0] = '\0';
                    counter++;
                }
            } else {
                actualPort[actualPortPosition] = '\0';
                result[counter] = checkAllowedPortNumbers(actualPort);
                if(result[counter] == 0){
                    return 0;
                }
                actualPortPosition = 0;
                actualPort[0] = '\0';
                counter++;
            }
        }
        *portsToScan = result;
        free(result);
        return pSize;
    } else if(numberOfPorts2 == 1 && numberOfPorts == 0){
        // range
        int num1 = 0;
        int num2 = 0;
        for(int i = 0; i < (int)strlen(ports); i++){
            if(ports[i] >= '0' && ports[i] <= '9'){
                // number
                actualPort[actualPortPosition] = ports[i];
                actualPortPosition++;
                if(i+1 == (int)strlen(ports)){
                    actualPort[actualPortPosition] = '\0';
                    if(num1 == 0){
                        num1 = checkAllowedPortNumbers(actualPort);
                        if(num1 == 0){
                            return 0;
                        }
                    } else {
                        num2 = checkAllowedPortNumbers(actualPort);
                        if(num2 == 0){
                            return 0;
                        }
                    }
                }
            } else {
                actualPort[actualPortPosition] = '\0';
                if(num1 == 0){
                    num1 = checkAllowedPortNumbers(actualPort);
                    if(num1 == 0){
                        return 0;
                    }
                } else {
                    num2 = checkAllowedPortNumbers(actualPort);
                    if(num2 == 0){
                        return 0;
                    }
                }

                actualPort[0] = '\0';
                actualPortPosition = 0;
            }

        }

        if(num2 > num1){
            int pSize = num2-num1+1;
            int *result = malloc((pSize)*sizeof(int));
            int counter = 0;
            for(int i = num1; i <= num2; i++){
                result[counter] = i;
                counter++;
            }
            *portsToScan = result;
            free(result);
            return pSize;
        } else {
            printf("Error in range. You have to use range a-b, where a < b! ");
            return 0;
        }

    } else {
        return 0;
    }


    return 0;


}

struct sockaddr* getAddress(char *hostname){
    struct addrinfo* result;
    int error;


    error = getaddrinfo(hostname, NULL, NULL, &result);
    if (error != 0) {
        if (error == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }
        return NULL;
    }


    // return always first address
    return result->ai_addr;
    /*
    for(res = result; res != NULL; res = res->ai_next) {

        if(res->ai_family == AF_INET) {

            if( NULL == inet_ntop( AF_INET,
                                   &((struct sockaddr_in *)res->ai_addr)->sin_addr,
                                   _address,
                                   sizeof(_address) )
                    ) {
                perror("inet_ntop");
                //return EXIT_FAILURE;
            }

            //printf("%s\n", _address);
            return res;
        }
    }*/
}

void generateOutputHeader(char *destination, struct sockaddr* a){
    char _address[INET6_ADDRSTRLEN];

    if( NULL == inet_ntop( AF_INET,
                           &((struct sockaddr_in *)a)->sin_addr,
                           _address,
                           sizeof(_address) )
            ) {
        perror("inet_ntop");

    }

    printf("Interesting ports on ");
    printf("%s", destination);
    if(strcmp(_address, destination) != 0){
        printf(" (%s)", _address);
    }
    printf(":\n");
    printf("PORT\t\tSTATE\n");

}


int main(int argc, char *argv[]) {

    // arguments parsing
    int c;
    char *pu, *pt, *destination;


    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
                {"pu",  required_argument, 0,  'u' },
                {"pt",  required_argument, 0,  't' },
                {"help",  no_argument, 0,  'h' },
                {0,     0,                 0,  0 }
        };

        c = getopt_long_only(argc, argv, ":t:u:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            //case 'p':
            case 'u':
                //printf("option pu with value '%s'\n", optarg);
                pu = optarg;
                break;

            case 't':
                //printf("option pt with value '%s'\n", optarg);
                pt = optarg;
                break;

            case 'h':
                if(argc != 2){
                    printf("This combination of arguments is not allowed. Use ./ipk-scan -help for understanding.\n");
                    return ARGUMENTS_ERROR;
                } else {
                    generateHelp();
                    return EXIT_SUCCESS;
                }


            case '?':
                printf("This arguments are not allowed. Use ./ipk-scan -help for understanding.\n");
                return ARGUMENTS_ERROR;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    if(optind < argc) {
        //printf("%d \n", optind);
        if(optind < 5){
            printf("You have to specify domain-name or IP address. Use ./ipk-scan -help for understanding.\n");
            return ARGUMENTS_ERROR;
        } else if(optind > 5){
            printf("Too many arguments. Use ./ipk-scan -help for understanding.\n");
            return ARGUMENTS_ERROR;
        } else {
            destination = argv[optind];
            //printf("moje zkouska: %s\n", argv[optind]);
        }

    }

    int *ptArray;
    //printf("jsem se nejak dostal sem \n");
    printf("pt: %s\n", pt);
    int ptArraySize = validateAndExtractPortNumbers(pt, &ptArray);
    if(ptArraySize == 0){
        printf("This specification of ports is not allowed. Use ./ipk-scan -help for understanding.\n");
    }
    //printf("a sem \n");


    printf("pt arr size: %d\n", ptArraySize);
    printf("pu: %s\n", pu);
    int *puArray;
    int puArraySize = validateAndExtractPortNumbers(pu, &puArray);
    if(puArraySize == 0){
        printf("This specification of ports is not allowed. Use ./ipk-scan -help for understanding.\n");
    }

    for(int i = 0; i < ptArraySize; i++){
        printf("pt %d: \n", ptArray[i]);
    }

    printf("pu arr size: %d\n", puArraySize);

    for(int j = 0; j < puArraySize; j++){
        printf("pu %d. %d\n", j, puArray[j]);
    }


    //printf("a sem \n");

    struct sockaddr* a = getAddress(destination);


    if(a == NULL){
        printf("vyskytla se error pri prevodu hostname na ip adresu\n");
    }

    // vypis, ktery se presunul do GenerateOutput
    /*char _address[INET6_ADDRSTRLEN];

    if( NULL == inet_ntop( AF_INET,
                           &((struct sockaddr_in *)a)->sin_addr,
                           _address,
                           sizeof(_address) )
            ) {
        perror("inet_ntop");

    }

    printf("%s\n", _address);*/

    generateOutputHeader(destination, a);



    /*struct addrinfo* result;
    struct addrinfo* res;
    int error;
    char _address[INET6_ADDRSTRLEN];


    error = getaddrinfo(destination, NULL, NULL, &result);
    if (error != 0) {
        if (error == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }
        exit(EXIT_FAILURE);
    }

    for(res = result; res != NULL; res = res->ai_next) {

        if(res->ai_family == AF_INET) {

            if( NULL == inet_ntop( AF_INET,
                                   &((struct sockaddr_in *)res->ai_addr)->sin_addr,
                                   _address,
                                   sizeof(_address) )
                    ) {
                perror("inet_ntop");
                return EXIT_FAILURE;
            }

            printf("%s\n", _address);
        }
    }*/

    return EXIT_SUCCESS;
}