#include "ftp.h"

int loginSuccess = 0;
int data_sock;//data socket which is used to transfer data
int level;

int main(int argc, char *argv[])
{
    struct sockaddr_in remote;//Client
    socklen_t len = sizeof(struct sockaddr_in);
    int sock;
    int count = 0;//Count the client access number
    int listen_sock;

    if((listen_sock = startup(21)) < 0){//Initialize connection at port 21
        return 0;
    }
    
    for(;;){
        struct FtpClient* currClt = (struct FtpClient*) malloc(
				sizeof(struct FtpClient));//Current client
        int count_cmd = 0;//Count the number of commands from client

        int reuse = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,(char *) &reuse, sizeof(reuse));//For resuing socket

        //Accept the connection request
        if((sock = accept(listen_sock,(struct sockaddr*)&remote, &len)) < 0){
            perror(ACCEPT_ERROR);
            exit(0);
        }
        //Initialize client
        currClt->_client_socket = sock;
        currClt->_data_socket = -1;
        currClt->_dataport = -1;
        //currClt->*_ip = {0};
            
        printf(ACC_CLT);

        for(;;){
            if(!count_cmd)//First connected, send 220 to client
                send_msg(sock,"220 please enter username\r\n");
            if(handle_command(currClt)){//Handle incoming commands
                //Close connection socket
                if(close(sock) < 0){
                    perror(CLOSE_ERROR);
                    exit(0);
                }
                break;
            }
            count_cmd++;
        }
    }

    //Close listening socket
    if(close(listen_sock) < 0){
        perror(CLOSE_ERROR);
    }

    return 0;
}

//Start up function to configure the server to boot
//In TCP flow gram in lecture notes, socket() to listen()
int startup(int port){
    int sock; /* Socket */
    int reuse = 1;
    struct sockaddr_in servAddr; /* Local address */
    unsigned short servPort; /* Server port */
    servPort = port; /* First arg: local port */

    /* Create socket for sending/receiving commands */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror(SOCKET_ERROR1);
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    /* Bind to the local address */
    if ((bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr))) < 0){
        perror(BIND_ERROR);
        return -2;
    }

    /*listening is always in passive mode*/
    if((listen(sock,5)) < 0){//Only listen to one user
        perror(LISTEN_ERROR);
        return -3;
    }else{
        printf("listening now\r\n");
    }

    return sock;
}

//Handle commands from client
int handle_command(struct FtpClient* client){
    //Buffer for string composed of command and arguments
    char *buffer = (char*) malloc(sizeof(char) * 1000); 
    char *cmd = NULL;//Ftp control command
    char *arg = NULL;//Arguments input by user
    int sock = client->_client_socket;
    char oldname[MAX];

    recv_msg(sock, &buffer, &cmd ,&arg);//Receive user input and command
    printf("User control command :%s",buffer);
    //Handle different control commands made by user
    if(strcmp("USER", cmd) == 0) {//USER
		handle_USER(sock, arg);
	}else if(strcmp("PASS", cmd) == 0) {//PASS
		loginSuccess = handle_PASS(sock, arg);
	}else if(strcmp("SYST", cmd) == 0){//SYST
        send_msg(sock, "215 UNIX Type: L8\r\n");//Online resources referenced
        printf("Sent 215 to client.\r\n");
    }else if(strcmp("PWD", cmd) == 0){//PWD
        if(loginSuccess == 1)
            handle_PWD(sock);
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("CWD", cmd) == 0){//CWD
        if(loginSuccess == 1)
            handle_CWD(sock,arg);
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("MKD", cmd) == 0){//MKD
        if(loginSuccess == 1)
            handle_MKD(sock,arg);
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("DELE", cmd) == 0){//DELE
        if(loginSuccess == 1 && level >= 3)
            handle_DELE(sock,arg);
        else if(level < 3)
            send_msg(sock,"550 You have no autority!\r\n");//Send 530
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("RNFR", cmd) == 0){//RNFR
        if(loginSuccess == 1){
            strcpy(oldname,arg);
            handle_RNFR(sock,arg);
        }else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("RNTO", cmd) == 0){//RNTO
        if(loginSuccess == 1)
            handle_RNTO(sock,oldname,arg);
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("LIST", cmd) == 0){
        if(loginSuccess == 1)
            handle_LIST(sock);    
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("PORT", cmd) == 0){//PORT
        handle_PORT(client, arg);
    }else if(strcmp("TYPE", cmd) == 0){//TYPE
        handle_TYPE(sock, arg);
    }else if(strcmp("RETR", cmd) == 0){
        if(loginSuccess == 1){
            if(handle_RETR(sock,arg) == 0){
                send_msg(sock,"502 Get data failed.\r\n");
                printf("Sent 502 to client\r\n");
            }else{
                send_msg(sock,"226 Transfer complete.\r\n");
                printf("Sent 226 to client\r\n");
            }
        }
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("STOR", cmd) == 0){//PUT Level 3 authority funciton
        if(loginSuccess == 1 && level >= 3){
            handle_PUT(sock,arg,data_sock);
        }else if(level < 3)
            send_msg(sock,"550 You have no autority!\r\n");//Send 530
        else
            send_msg(sock,"530 Please login with USER and PASS!\r\n");//Send 530
    }else if(strcmp("QUIT", cmd) == 0){//QUIT
        send_msg(sock,"221 Goodbye.\r\n");
        return 1;
    }else if(cmd == NULL){
        printf(NO_ARG);
        send_msg(sock, "500 Not legal usage.\r\n");
    }else{//Invaild input
        printf("Invalid command %s\r\n",cmd);
        send_msg(sock, "500 Invalid command.\r\n");
    }
    return 0;
}

//Send message
void send_msg(int sock, char* msg) {
	if (strlen(msg) <= 0) {//Message empty
		printf(NO_MSG);
	}
    if(send(sock, msg, strlen(msg),0) < 0){//Send
		perror(SEND_ERROR);
    }
}

//Receive message and process it to command and arguments
void recv_msg(int sock, char** buf, char** cmd, char** argument) {
    memset(*buf, 0, sizeof(char)* MAX);
    int temp = recv(sock, *buf, MAX, 0);//Receive

    if(temp == 0){//Client leaving server
        printf(CLT_LFT);
        return;
    }else if(temp < 0){//Receving error
        perror(RECEIVE_ERROR);
        return;
    }else{//Receive normally
        int index = _find_first_of(*buf, ' ');//Find the first ' ' to seperate command and argument
        if (index < 0) {//If no ' ', all command part
            *cmd = _substring(*buf, 0, strlen(*buf) - 2);
        } else {
            *cmd = _substring(*buf, 0, index);//Command part
            *argument = _substring(*buf, index + 1, strlen(*buf) - index - 3);//Argument part
        }
    }
}

//Handle USER control command
//Valid username: test
//TODO: use map to add more users
void handle_USER(int sock, char* name) {
	if(!strcmp(name,"test")){
        printf("Recieved Name : %s\r\n",name);
        send_msg(sock,"331 please enter password\r\n");//Send 331 to client
        printf("Sent 331 to client.\r\n");
        level = 2;//Define access level of current user
    }else if(!strcmp(name,"super")){
        printf("Recieved Name : %s\r\n",name);
        send_msg(sock,"331 please enter password\r\n");//Send 331 to client
        printf("Sent 331 to client.\r\n");
        level = 3;
    }else{
        printf(NO_NAME);
        send_msg(sock,NO_NAME);//Send 530
        printf("Sent 530 to client.\r\n");
    }
}

//Handle PASS control command
//Valid password: 123
int handle_PASS(int sock, char* pwd) {
    if(!strcmp(pwd,"123")){//Compare with correct password
        printf("Recieved Password : %s\r\n",pwd);
        send_msg(sock,"230 Login successful!.\r\n");//Send 230 to client
        printf("Sent 230 to client.\r\n");
        return 1;
    }else{//Wrong password
        printf("Recieved Password : Wrong password!\r\n");
        send_msg(sock,"530 Wrong password!.\r\n");//Send 530 to client
        printf("Sent 530 to client.\r\n");
        return 0;
    }
}

//Find the first of char in a string
//Online resources referenced
int _find_first_of(char* str, char ch)
{
	int len = strlen(str);
	int i = 0;
	for(; i < len; i ++)
	{	
		if(str[i] == ch)
		{
			return i;
		}
	}
	return -1;
}

//Substring method
//Online resources referenced
char* _substring(char* buf, int i, int len)
{
	if(i < 0|| len <= 0)
	{
		return NULL;
	}
	int j = 0;
	int l = strlen(buf);
	int length = l - i > len?len:l - i;
	char* dest = (char*) malloc(sizeof(char) * (length + 10));
	memset(dest, 0, length + 10);
	for(; j < length; j ++)
	{
		dest[j]  = buf[i + j]; 
	}

	return dest;
}

//Handle PWD control command
//Print working Directory
void handle_PWD(int sock){
    char buf[300];
    char cur_path[100];
    strcpy(buf, "257 \"");
    getcwd(cur_path, sizeof(cur_path));//get absolute path of current directory
    strcat(buf,cur_path);
    strcat(buf, "\"\r\n");
    printf("PWD prints %s",buf);
    send_msg(sock,buf);
    printf("Sent 257 to client.\r\n");
}

//Handle CWD control command
//Change Working Directory
void handle_CWD(int sock,char* dir){
    if(chdir(dir) != 0){
        send_msg(sock,"550 Failed to change directory.\r\n");
        printf("Failed to change, sent 550 to client.\r\n");
    }else{
        send_msg(sock,"250 Directory successfully changed.\r\n");
        printf("Successful to change, sent 250 to client.\r\n");
    }
}

//Handle PORT control command
int handle_PORT(struct FtpClient* client,char* arg){
    if (data_sock > 0) {//If there is previous connection
		close(data_sock);
	}
    
    int ip1,ip2,ip3,ip4,port1,port2;
    //char ip[MAX] = {0};

	sscanf(arg, "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2);//Interprete argument
    client->_dataport = port1 * 256 + port2;//Calculate port number of data transfer port
    snprintf(client->_ip,50,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
    
    if((data_sock = start_data_port(client->_dataport, client->_ip)) < 0){//Start TCP connection
        send_msg(client->_client_socket,"530 TCP connection failed!\r\n");//Sent 530 to client
        printf("Sent 530 to client.\r\n");
        return -1;
    }else{
        send_msg(client->_client_socket,"200 PORT command successful.\r\n");
        printf("Created new data port :%d, client ip :%s.\r\n",client->_dataport,client->_ip);
        printf("Sent 200 to client.\r\n");
        return 1;
    }
}

/* calculate time duration between two time points. */
int timeDiff(struct timeval* tv1, struct timeval* tv2) {
    return (int) (abs(tv2->tv_sec - tv1->tv_sec) * 1000000 + tv2->tv_usec - tv1->tv_usec);
}

void sleep_ms(unsigned int msecs) {
    struct timeval tval;
    tval.tv_sec = msecs/1000;
    tval.tv_usec = (msecs*1000)%1000000;
    select(0, NULL, NULL, NULL, &tval);
}

//Handle RETR
int handle_RETR(int sock,char* arg){
    char file_name[MAX];//File name
    char temp[MAX];//IF there is a second argument of user, use temp to store
    memset(file_name,'\0',sizeof(file_name));
    sscanf(arg,"%s %s",file_name,temp);//Interprete

    send_msg(sock,"150 Opening BINARY mode data connection.\r\n");//Sent 150 to client
    printf("Sent 150 to client.\r\n");

    FILE *fp;
    fp = fopen(file_name, "r");//Open file in read mode
    struct timeval start;
    struct timeval end;
    int pkt_num = 0;
    int cst_spd = 10;//constrain to 10 kB/s

    if(fp == NULL){
        printf("The file %s do not exist.\r\n", file_name);
		close(data_sock);
        return 0;
    }else{
        //Get size of the file
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);

        fseek(fp, 0, SEEK_SET);
		char buf[MAX];
		memset(buf,'\0',MAX);
        
        gettimeofday(&start, NULL); //Start timer

        while(fread(buf, MAX, 1, fp)==1){//Read file
            send(data_sock,buf,MAX,0);
            memset(buf,'\0',MAX);
            pkt_num++;
        }
        pkt_num++;

        int tt_tfc = strlen(buf) + (pkt_num - 1) * 255;//get total traffic of the tranfer for all paktets
        sleep_ms(1 + tt_tfc/cst_spd);//Sleep for a while
        send(data_sock,buf,strlen(buf),0);

        gettimeofday(&end, NULL);//End timer
        int tm_df = timeDiff(&start, &end);
        double speed = (double) tt_tfc / 1024.0 * 1000000.0 / tm_df;
        printf("total traffic: %d Bytes sent in %d pakets\r\nsending speed :%f kB/s\r\n",tt_tfc,pkt_num,speed);
        
        fclose(fp);
    	close(data_sock);
        return 1;
	}
}

//Start TCP connection at server port 20, client port variable port
int start_data_port(int port, char* ip){
    if(data_sock > 0){
        close(data_sock);
    }
    int sock;
    
    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror(SOCKET_ERROR1);
        return -1;
    }else{
        struct sockaddr_in cltAddr; /* Local address */
        struct sockaddr_in servAddr; /* Local address */
        int reuse = 1;
        if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) != 0){//socket reuse setting
            printf(REUSE_ERR);
            return -2;
        }

        /* Construct client address structure */
        memset(&cltAddr, 0, sizeof(cltAddr));
        cltAddr.sin_family = AF_INET;
        cltAddr.sin_addr.s_addr = inet_addr(ip);
        cltAddr.sin_port = htons(port);

        /* Construct server address structure */
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servAddr.sin_port = htons(20);

        /* Bind to the local address */
        if ((bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr))) < 0){
            perror(BIND_ERROR);
            return -3;
        }

        //Use connect function to connect client actively
        if(connect(sock,(struct sockaddr*)&cltAddr, sizeof(cltAddr)) < 0){
            close(sock);
            perror(CONNECT_ERROR);
            return -4;
        }else{
            printf("Started up TCP connection with client at port 20.\r\n");
            return sock;
        }
    }
}

//Handle LIST control command
//Displays the contents of the working directory
void handle_LIST(int sock){
    if(data_sock < 0){
        send_msg(sock, "451 data socket closed.\r\n");
        exit(0);
        return;
    }
    send_msg(sock,"150 Here comes the directory listening.\r\n");
    printf("Prepare to list, sent 150 to client.\r\n");

    FILE *fp = NULL;
    if((fp = popen("ls -l", "r")) == NULL ){
        send_msg(sock, "451 the server had trouble reading the directory from disk.\r\n");
        printf("Failed to list, sent 451 to client.\r\n");
        return;
    }else{
	    char buf[MAX*10];
        memset(buf,'\0',sizeof(buf));
        fread(buf, MAX*10 - 1, 1, fp);
        send_msg(data_sock,buf);
        pclose(fp);
	}
    
    close(data_sock);
    send_msg(sock, "226 Directory send OK.\r\n");
    printf("Successful to list, sent 226 to client.\r\n");
    return;
}

//Handle MKD control command
//User to make a directory
void handle_MKD(int sock,char* name){
    char path[300];
    char buf[300]; 
    strcpy(buf, "257 \"");
    getcwd(path, sizeof(path));
    strcat(buf,path);
    strcat(buf,"/");
    strcat(buf,name);
    strcat(buf,"\" created.\r\n");
    if(mkdir(name) != 0){
        send_msg(sock,"550 Create directory operation failed.\r\n");
        printf("Failed to make directory, sent 550 to client.\r\n");
    }else{
        send_msg(sock,buf);
        printf("Successful to make directory, sent 257 to client.\r\n");
    }
}

//Handle DELE control command
//Delete file or directory
void handle_DELE(int sock,char* name){
    if(remove(name) != 0){
        send_msg(sock,"550 Delete operation failed.\r\n");
        printf("Failed to delete, sent 550 to client.\r\n");
    }else{
        send_msg(sock,"250 Delete opertaion successful.\r\n");
        printf("Successful to delete, sent 250 to client.\r\n");
    }
}

//Handle RNFR control command
void handle_RNFR(int sock, char* oldname){
    FILE *fp = NULL;
    if((fp = fopen(oldname,"r")) == NULL){//Check if the name exists
        send_msg(sock,"550 Failed to rename.\r\n");
        printf("Failed for RNFR, sent 550 to client.\r\n");
    }else{
        send_msg(sock,"350 Ready for RNTO.\r\n");
        printf("Success for RNFR, sent 350 to client.\r\n");
    }
}

//Handle RNTO control command
void handle_RNTO(int sock, char* oldname, char* newname){
    FILE *ck;
    if((ck = fopen(newname,"r")) != NULL){//Check if the name exists
        send_msg(sock,"550 Files with the same name exist.\r\n");
        printf("Failed for RNTO, sent 550 to client.\r\n");
        return;
    }
    if(rename(oldname,newname) == 0){
        send_msg(sock,"250 Rename successful.\r\n");
        printf("Successful to rename, sent 250 to client.\r\n");
    }else{
        send_msg(sock,"550 RNTO command failed.\r\n");
        printf("Failed to RNTO, sent 550 to client.\r\n");
    }
}

//Handle STOR control command
void handle_PUT(int sock, char* name, int data_sock){
    FILE *fp;
    unsigned char databuf[1024];
    struct stat buf;
    int bytes = 0;
    int pkt_num = 0;
    struct timeval start;
    struct timeval end;
    int tt_tfc = 0;
    int up_sp = 5;

    FILE *ck;
    if((ck = fopen(name,"r")) != NULL){//Check if the name exists
        send_msg(sock,"450 Files with the same name exist.\r\n");
        printf("Failed for STOR, sent 450 to client.\r\n");
        return;
    }

    if(stat(name, &buf) == -1){//Check for duplicate name files 
        printf("File doesn't existd, try to create a new one.\r\n");
    }
    fp = fopen(name,"w");//Open the file as "write"
    if(fp == NULL){
        perror("fopen() error");
        send_msg(sock,"450 Cannot create the file.\r\n");
        printf("Failed to put, please chech.\r\n");
        close(data_sock);
        return;
    }else{
        send_msg(sock,"150 Ok to send data.\r\n");
        printf("Sent 150 to client.\r\n");
    }
    
    //Write data through I/O stream
    gettimeofday(&start, NULL);
    while((bytes = read(data_sock,databuf,MAX)) > 0 ){
        write(fileno(fp),databuf,bytes);
        pkt_num++;
        tt_tfc += bytes;
    }
    sleep_ms(1 + tt_tfc/5);
    gettimeofday(&end, NULL);
    tt_tfc += bytes;
    pkt_num++;

    int tm_df = timeDiff(&start, &end);
    double speed = (double) tt_tfc / 1024.0 * 1000000.0 / tm_df;
    printf("total traffic: %d Bytes recieved in %d pakets\r\nrecieving speed :%f kB/s\r\n",tt_tfc,pkt_num,speed);
    
    fclose(fp);
    close(data_sock);
    send_msg(sock,"226 Transfer complete.\r\n");
    printf("Successful to put, send 226 to client.\r\n");
    return;
}

//Handle TYPE control command
void handle_TYPE(int sock, char* mode){
    switch(mode[0])//判断传输模式 默认是二进制
    {
        case 'A':
            send_msg(sock, "200 Switching to ASCII mode.\r\n");
            printf("Set type to ASCII\r\n");
            break;
        case 'I':
            send_msg(sock, "200 Switching to Binary mode.\r\n");
            printf("Set type to Binary\r\n");
            break;
        default:
            send_msg(sock, "200 Switching to Binary mode.\r\n");
            printf("Set type to Binary\r\n");
            break;
    }
    return;
}