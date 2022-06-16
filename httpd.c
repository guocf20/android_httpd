/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include<sys/epoll.h>

#include "log.h"


#define MAX_CLIENTS 1024
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2


#define min(x, y) (x) > (y)? (y):(x)

typedef struct http_body_
{
	uint8_t data[4096];
	int body_len;
	int body_writed;
	int body_offset;
}http_body;

typedef struct http_header_
{
	char *origin;
	char *method;
	char *path;
	char *query;
	char *boundary;
	int content_len;
}http_header;
void accept_request(void *arg);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, http_header header, http_body body, int conent_left);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(short *);
void unimplemented(int);

extern FILE *logger_fd;
extern int logger;






int  parse_header(const char* line, int line_len, http_header *entry, int *header_len, int *body_len)
{
	const char *ptr = line;
	const char *key_value_ptr = NULL;
	const char *header_end = NULL;
        if (line == NULL)
        {
                return 1;
        }
        ptr = strchr(line, ' ');
        if (ptr == NULL)
        {
                return 1;
        }

        const char *start_of_path = ptr + 1;

        entry->method = strndup(line, start_of_path - line + 1);
        entry->method[start_of_path - line - 1] = '\0';
//no query string;
        ptr = strchr(start_of_path, '?');

        if (ptr == NULL)
        {
                const char *start_of_query = strchr(start_of_path, ' ') + 1;
                entry->path = strndup(start_of_path, start_of_query - start_of_path + 1);
                entry->path[start_of_query - start_of_path - 1] = '\0';
		goto parse_left;
        }

        const char *start_of_query = ptr + 1;
        entry->path = strndup(start_of_path, start_of_query - start_of_path + 1);
        entry->path[start_of_query - start_of_path - 1] = '\0';

        ptr = strchr(start_of_query, ' ');
        if (ptr == NULL)
        {
                return 1;
        }

        const char *end_of_query = ptr + 1;
        entry->query = strndup(start_of_query, end_of_query - start_of_query + 1);
        entry->query[end_of_query - start_of_query -1 ] = '\0';
	

parse_left:
	 
	 ptr = line;
         key_value_ptr = line;
 	while(key_value_ptr != NULL)
	{
		key_value_ptr = strstr(ptr, "\r\n");
		if (key_value_ptr && (ptr-line) < line_len)
		{

			char *str = strndup(ptr, key_value_ptr - ptr +1);
			str[key_value_ptr - ptr ] = '\0';
			if (strlen(str)  > 2)
			{
				printf("str key value = %s\n", str);
				if (strcasestr(str, "Content-Length") != NULL)
				{
					printf( "this i content leng = %d\n", atoi(&str[strlen("Content-Length") + 1]));
					entry->content_len = atoi(&str[strlen("Content-Length") + 1]);
				}
				if (strcasestr(str, "Content-Type") != NULL)
				{

					printf( "this i content info = %s\n", &str[strlen("Content-Type") + 1]);
					char *boundary_begin = strstr(&str[strlen("Content-Type") + 1],"boundary=");
					entry->boundary =strdup(boundary_begin+strlen("boundary="));
					printf("boundary %s\n", entry->boundary);
				}
			}
			ptr = key_value_ptr + 2;
			if (*(ptr+0) == '\r' && *(ptr+1) == '\n')
			{
				printf("this is end\n");
				ptr+=2;
				break;
			}
		}
		else
		{
			break;
		}
		
	}
	printf("body in header left %d  total =%d  header_len = %d\n", line_len - (ptr-line), line_len, ptr-line);
	*header_len = ptr-line;
	*body_len = line_len - (ptr-line);
        return 0;
}


void dump_header(http_header h)
{
        if (h.method)
        {
                printf("%s\n", h.method);
        }
        if (h.path)
        {
                printf("%s*\n", h.path);
        }
        if (h.query)
        {
                printf("%s\n", h.query);
        }

}

void free_header(http_header *h)
{

        if (h->method)
        {
                free(h->method);
		h->method = NULL;
        }
        if (h->path)
        {
                free(h->path);
		h->path = NULL;
        }
        if (h->query)
        {
               free(h->query);
	       h->query = NULL;
        }
}


/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(void *arg)
{
    int *int_ptr = (int *)arg;

    int client = *int_ptr;
    char buf[1024];
    size_t numchars = 0; 
    char method[255] = {0};
    char url[255];
    char path[512];
    struct stat st;
    int cgi = 0;      /* becomes true if server decides this is a CGI
                       * program */

    int body_len;
    int header_len;

    log_info(logger,"in acept request\n");

    http_header header;
    http_body body;

    char http_head[4096]={0};
    int read_total = read_http_header(client, http_head, 4096);
    memset(&header, '\0', sizeof(header));
    memset(&body, '\0', sizeof(body));
    parse_header(http_head, read_total, &header,&header_len, &body_len);
    if (body_len > 0)
    {
    	memcpy(body.data, &http_head[header_len], body_len);
	body.body_len  = body_len;
	printf("body info %s\n", body.data);
    }
    dump_header(header);
    sprintf(method, "%s", header.method);
    sprintf(url, "%s", header.path);
    if (header.query || strcmp(header.method,"POST") == 0)
    {
	cgi = 1;
    }


    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/' && strlen(url) == 1)
        strcat(path, "index.html");

    if (path[strlen(path) - 1] == '/')
    {
	    path[strlen(path) - 1] = '\0';
    }

    printf("path =%s\n", path);

    if (stat(path, &st) == -1) 
    {
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
	{
         //   strcat(path, "/index.html");
	}
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;
        if (!cgi)
	{
		printf("in serve\n");
            serve_file(client, path);
	}
	else
	{
	    printf("run cgi %s\n", path);
	    int content_left = header.content_len - body_len;
            execute_cgi(client, path, method, header, body, content_left);
	}
    }
    free_header(&header);
    close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{

    int fd = fileno(resource);
    struct stat attr;
    fstat(fd, &attr);

    log_info(logger, "file size = %d\n", attr.st_size);



    

   // size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
   //
    int read_once = 1024 * 200;
    int left = attr.st_size;
    unsigned char *ptr = (unsigned char *)malloc(read_once);
    while(left)
    {
	int read_size = fread(ptr, 1, min(read_once, left), resource);
	send(client, ptr, read_size, 0);
	left -=read_size;
    }
    free(ptr);
    return;
/*
    unsigned char *ptr = (unsigned char *)malloc(attr.st_size);
    fread(ptr, 1, attr.st_size, resource);
    send(client, ptr, attr.st_size, 0);
    
    free(ptr); */
    return ;

}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}


void delchar( char *str, char c ){
char *p = str;
while (*p) {
if (*p != c)
*str ++ = *p;
p ++;
    }
*str = '\0';
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path, const char *method,http_header header, http_body body, int content_left)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    printf("in execute cgi %s %s  %d %d\n", path, header.query,  body.body_len, content_left);
    
    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
    {
	    printf("execute_cgi get method\n");
    }
    else if (strcasecmp(method, "POST") == 0) /*POST*/
    {
	uint8_t *ptr = &body.data[content_left - 1];
	int len = 0;
	uint8_t tmpbuf[1024]={0};
	if (content_left == 0)
	{

		char *filename = memmem(body.data, body.body_len , "filename=", strlen("filename="));
		char *lineend = strstr(filename, "\r\n");
		char *tmp = memmem(body.data, body.body_len, "\r\n\r\n", 4);

		if (tmp != NULL)
		{
		   char *file = strndup(filename + strlen("filename="), lineend - (filename + strlen("filename=")) + 1);
		   int len = lineend- (filename + strlen("filename="));
		   printf("file len = %d\n", len);
		   file[lineend - (filename + strlen("filename=")) -1 ] = '\0'; 
		   printf("file = %s\n", file);
		   
		    char end_bound[256] = {0};
		    snprintf(end_bound, 255, "\r\n--%s", header.boundary);
		    char *boundary = memmem(tmp,  body.body_len - (tmp - (char *)body.data), end_bound, strlen(end_bound));
		    char path[256] = {0};
		    snprintf(path, 255, "htdocs/download/%s", file);
		    delchar(path, '"');
		    free(file);
		   FILE *fp = fopen(path, "a+");
		    fwrite(tmp + 4, 1, (boundary - (tmp +4)),fp);
		    fclose(fp);
		}


	}
	while (content_left > 0)
	{
		len = read(client, tmpbuf, 1024);
		if (len == 0 )
		{
			printf("read len = 0\n");
			break;
		}
		content_left-=len;
		if (len + body.body_len < sizeof(body.data ))
		{
			memcpy(ptr, tmpbuf, len);
			body.body_len +=len;
			ptr += len;
		}
		else
		{
			char *tmp = memmem(ptr, body.body_len, "\r\n\r\n", 4);
			if (tmp != NULL)
			{
				printf("this is end of data\n");
			}

		}
		

		printf("read = %d left = %d\n", len, content_left);

	}

    	sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Length:0\r\nConnection: close\r\n\r\n");
    	send(client, buf, strlen(buf), 0);


    }
    else/*HEAD or other*/
    {
    }


    if (pipe(cgi_output) < 0) {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }

    if ( (pid = fork()) < 0 ) {
        cannot_execute(client);
        return;
    }
    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", header.query);
            putenv(query_env);
        }
        else {   /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
	struct stat st;
	stat(path, &st);
	if ((st.st_mode & S_IFMT) == S_IFDIR)
	{
		execlp("./htdocs/ls.lua", "./htdocs/ls.lua",path, NULL);
	}
	else
	{
        	execl(path, NULL);
	}
        exit(0);
    } else {    /* parent */
	int content_length = 0;
	char content[4096] = {0};
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
	{
	    content[content_length++] = c;
            //send(client, &c, 1, 0);
	}

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);

    	sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Length:%d\r\nConnection: close\r\n\r\n", strlen(content));
    	send(client, buf, strlen(buf), 0);
	send(client, content, strlen(content), 0);
    }
}



int read_http_header(int sock, char *buf, int buf_size)
{
	//avoid \r\n\r\n
	char *ptr = buf ;
	int size = 0;
	int http_header_end = 0;
	int total_read = 0;
	read(sock, ptr, 4);
	total_read +=4;
	ptr+=4;

	while ( (size = read(sock, ptr, 1024) ) > 0 && (ptr-buf) < buf_size  )
	{
		total_read +=size;
		if (strstr( (ptr), "\r\n\r\n")!= NULL)
		{

			break;
		}
		else
		{
			ptr+=size;
		}
	}
	return total_read;
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    struct stat entry;
    stat(filename, &entry);
    printf("header size = %d\n", entry.st_size);


    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    printf("file name = %s\n", filename);

    snprintf(buf, 1023, "Content-Length: %d\r\n", entry.st_size);
    send(client, buf, strlen(buf), 0);


    snprintf(buf, 1023, "Accept-Range: none\r\n");
    send(client, buf, strlen(buf), 0);

    printf("filename = %s\n", filename);
    if (strstr(filename, ".jpg") != NULL)
    {
	    log_info(logger, "this is pic\n");
	    sprintf(buf, "Content-Type: image/jpeg\r\n");
    }
    else if (strstr(filename, ".webp") != NULL)
    {
	    sprintf(buf, "Content-Type: image/webp\r\n");
    }
    else if (strstr(filename, ".zip") != NULL || strstr(filename, ".tar") || strstr(filename, "*.gz") != NULL)
    {

	    log_info(logger, "this is compress file\n");
	    sprintf(buf, "Content-Type: application/octet-stream\r\n");
    }
    else if (strstr(filename, ".mp4") != NULL)
    {

	    sprintf(buf, "Content-Type: video/mp4\r\n");
    }
    else if (strstr(filename, ".mp3") != NULL)
    {
	    sprintf(buf, "Content-Type: audio/mp3\r\n");
    }
    else if (strstr(filename, "webm") != NULL)
    {
		sprintf(buf, "Content-Type: video/webm\r\n");
    }
    else
    {
	    log_info(logger, "this is text\n");
    	    sprintf(buf, "Content-Type: text/html\r\n");
    }
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    /*
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';

    while ((numchars > 0) && strcmp("\n", buf)) 
        numchars = get_line(client, buf, sizeof(buf));
*/
    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

/*
int init_server(char *ip, short port)
{
	int server = 0;
	int enable = 1;
	server = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = hton(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

    	if ((setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) < 0)  
    	{  
        	error_die("setsockopt failed");
    	}
    	if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		error_dir("bind failed");
	}
	if (listen(server, MAX_CLIENTS) < 0)
	{
		error_dir("listen bind");
	}

	int epfd = epoll_create(MAX_CLIENTS);
	
	struct epoll_event event;
	event.data.fd = server;

	return server;
}
*/

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(short *port)
{
    int httpd = 0;
    int on = 1;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  
    {  
        error_die("setsockopt failed");
    }
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    if (*port == 0)  /* if dynamically allocating a port */
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 20) < 0)
        error_die("listen");
    return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


/**********************************************************************/

int main(void)
{
 //   daemon(1, 0);
    signal(SIGPIPE, SIG_IGN);

    log_init("log.txt", LOG_LEVEL_ALL);	
    int server_sock = -1;
    short port = 4000;
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t  client_name_len = sizeof(client_name);
    pthread_t newthread;

    server_sock = startup(&port);
    log_err(logger, "httpd running on port %d\n", port);
    

    while (1)
    {
        client_sock = accept(server_sock,
                (struct sockaddr *)&client_name,
                &client_name_len);
	log_info(logger, "new accepting...\n");
        if (client_sock == -1)
	{
            error_die("accept");
	}
        //accept_request(client_sock); 
        if (pthread_create(&newthread , NULL, (void *)accept_request, (void *)&client_sock) != 0)
            perror("pthread_create");

	pthread_detach(newthread);
    }

    close(server_sock);

    log_close();
    return(0);
}
