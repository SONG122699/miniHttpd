#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<string.h>
//#include<winsock2.h>
#include<ctype.h>
#include<errno.h>
#include<arpa/inet.h>//用于互联网地址的定义，包括一些转换函数

#define _debug 1

#define SERVER_PORT 8085


int get_line(int sock, char* buff, int size);
void do_http_request(int client_sock);
void do_http_response(int client_sock);
void not_found(int client_sock);

/*
*解析客户端请求
*@para client_sock 客户端socket编号
*/
void do_http_request(int client_sock){
    int len;
    char buff[256];
    char method[64];
    char url[128];
    char path[256];
    struct stat st;
    /*读取客户端发送的http请求*/
    //1.读取请求行
    len = get_line(client_sock, buff, sizeof(buff));
    if(len > 0){
        int i = 0, j = 0;
        while(!isspace(buff[i]) && i<sizeof(method)-1){
            method[i] = buff[i];
            i++;
            j++;
        }
        method[i] = '\0';
        if(_debug) printf("request method: %s\n", method);

        if(strncasecmp(method, "GET", strlen(method)) == 0){//处理get请求
            if(_debug) printf("method = GET\n");
            //获取url
            while(isspace(buff[j])) j++;//跳过空白符
            i = 0;
            while(!isspace(buff[j]) && i<sizeof(url)-1){
                url[i] = buff[j];
                i++;
                j++;
            }
            url[i] = '\0';
            if(_debug) printf("url: %s\n",url);

            //继续读取http头部
            do{
                len = get_line(client_sock, buff, sizeof(buff));
                if(_debug) printf("read: %s\n",buff);
            }while(len > 0);

            //***定位服务器本地的html文件***
            //有参数的情况，处理url中的?，得到真实的url
            {
                char *pos = strchr(url, '?');//返回?第一次出现的地址
                if(pos){
                    *pos = '\0';//将出现的?改为结束符
                    printf("real url : %s\n",url);
                }
            }

            sprintf(path, "var/www/html%s", url);
            if(_debug) printf("html path: %s\n", path);

            //执行响应
            //如果请求的文件存在，则返回相应的文件；如果不存在，响应404 NOT FOUND
            if(stat(path, &st) < 0){//文件不存在或者出错
            not_found(client_sock);
            }
            else{
                do_http_response(client_sock);
            }

        }else{//非get请求,响应501 metho
            fprintf(stderr, "warning! other method [%s]\n", method);
        //继续读取http头部
            do{
                len = get_line(client_sock, buff, sizeof(buff));
                if(_debug) printf("read: %s\n",buff);
            }while(len > 0);
            //unimplemented(client_sock);//
        }
    }else{//请求格式有问题，出错处理
        //bad_request(client_sock);
    }
}

void do_http_response(int client_sock){
    const char* main_header = "\
    HTTP/1.0 200 OK\r\n\
    Server: Song Server\r\n\
    Content-Type: text/html\r\n\
    Connection: Close\r\n\
    ";
    const char* welcome_html = "\
    <!DOCTYPE html>\n\
    <html>\n\
    <head>\n\
	    <meta charset=\"UTF-8\">\n\
	    <title>登录页面</title>\n\
    </head>\n\
    <body align=center height=\"500px\" >\n\
    <div>\n\
    <h1>登录页面</h1>\n\
    <form>\n\
        <label for=\"email\">邮箱：</label><br>\n\
        <input type=\"text\" id=\"email\" name=\"email\" placeholder=\"请输入邮箱\"><br>\n\
        <label for=\"password\">密码：</label><br>\n\
        <input type=\"password\" id=\"password\" name=\"password\" placeholder=\"请输入密码\"><br>\n\
        <input type=\"submit\" value=\"登录\">\n\
    </form>\n\
    </div>\n\
    </body>\n\
    </html>\n\
    ";
    //1.发送main_header
    int len  = write(client_sock, main_header, strlen(main_header));
    if(_debug) fprintf(stdout, "... send main_header ...\n");
    if(_debug) fprintf(stdout, "write[%d] : %s", len, main_header);
    //2.生成Conten-Length行并发送
    char send_buff[64];
    int wc_len = strlen(welcome_html);
    len = snprintf(send_buff, 64, "Content=Length: %d\r\n\r\n", wc_len);
    len = write(client_sock, send_buff, len);
    if(_debug) fprintf(stdout, "write Content=Length[%d]: %s", len, send_buff);
    //3.发送HTML文件内容
    len = write(client_sock, welcome_html, wc_len);
    if(_debug) fprintf(stdout, "write html[%d]: %s",len, welcome_html);
}

void not_found(int client_sock){
    const char* main_header = "\
    HTTP/1.0 404 NOT FOUND\r\n\
    Server: Song Server\r\n\
    Content-Type: text/html\r\n\
    \r\n\
    ";
    const char* reply = "\
    <!DOCTYPE html>\n\
    <html>\n\
    <head>\n\
	<title>404 Not Found</title>\n\
    </head>\n\
    <body>\n\
	    <h1>404 Not Found</h1>\n\
	    <p>The page you are looking for could not be found.</p>\n\
    </body>\n\
    </html>\n\
    ";

    int len  = write(client_sock, main_header, strlen(main_header));
    if(_debug) fprintf(stdout, "... send main_header ...\n");
    if(_debug) fprintf(stdout, "write[%d] : %s", len, main_header);

    len = write(client_sock, reply, strlen(reply));
    if(len <= 0){
        fprintf(stderr, "send reply failed, reason: %s\n", strerror(errno));
    }


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