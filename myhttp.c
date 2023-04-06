#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
//#include<winsock2.h>
#include<ctype.h>
#include<arpa/inet.h>//用于互联网地址的定义，包括一些转换函数


int get_line(int sock, char* buff, int size);
void do_http_request(int client_sock);

void do_http_request(int client_sock){
    int len;
    char buff[256];
    /*读取客户端发送的http请求*/
    //1.读取请求行
    len = get_line(client_sock, buff, sizeof(buff));
}

/*读取请求信息
*@para sock:套接字描述符
*@para buff:数据缓冲区
*@para size:缓冲区大小
*return -1表示出错，>0表示读取的数据长度。
*/
int get_line(int sock, char* buff, int size){
    int count = 0;
    int len = 0;
    char ch = '\0';
    while((count<size-1) && ch != '\n'){
        len = read(sock, &ch, 1);
        //读取成功
        if(len == 1){
            //处理特殊字符
            if(ch == '\r'){//不存储回车符
                continue;
            }
            else if(ch == '\n'){//换行结束
                buff[count] = '\0';
                break;
            }
            //处理一般字符
            buff[count] = ch;
            ++count;
        }
        else if(len < 0){//读取出错
            perror("read failed");
            count = -1;
            break;
        }
        else{//返回0，客户端链接关闭
            fprintf(stderr, "client connect close.\n");
            count = -1;
            break;
        }
    }
    return count;
}

#define SERVER_PORT 8003

int main(void){
    int sock;//
    struct sockaddr_in server_addr;

    //1.创建信箱
    sock = socket(AF_INET, SOCK_STREAM, 0);
    //2.清空标签，写上地址和端口号
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;//选择协议族IP4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//监听本地所有IP地址
    server_addr.sin_port = htons(SERVER_PORT);//绑定端口号

    //3.实现标签贴到收信箱
    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    //4.把信箱挂置到传达室
    listen(sock, 128);
    //5.等待接受来信
    printf("waitting for client message.\n");
    
    int done = 1;
    while(done){
        struct sockaddr_in client;
        int client_sock, len, i;
        char client_ip[64];
        char buff[256];
        socklen_t client_addr_len;
        client_addr_len = sizeof(client);
        client_sock = accept(sock, (struct sockaddr*)& client, &client_addr_len);

        //打印客户端IP和port
        printf("client ip : %s\t port: %d\t",
        inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, sizeof(client_ip)),
        ntohs(client.sin_port));
        //处理http请求，读取客户端发送的信息
        do_http_request(client_sock);
        close(client_sock);
    }
    close(sock);
    return 0;
}