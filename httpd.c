#include "httpd.h"
#include "simple_net.h"

/**************************************************
 * Function: main
 *    Turns command line argument into an int, 
 *    which is the port to open. Set up signal
 *    handlers, open the service. Infinite loop
 *    in order to accept all connections. 
 *
 * Returns:
 *    0 on success; 
 *************************************************/

int main(int argc, char *argv[]){
   in_port_t port = strtol(argv[1], NULL, 10);
   int fd, new_fd;


   if((fd = create_service(port)) < 0){
      fprintf(stderr,"Error opening socket\n");
      exit(-1);
   }

   setup_handlers(CHILD);
   while(1){
      if((new_fd = accept_connection(fd)) < 0){
         fprintf(stderr,"Error opening connection\n");
         exit(1);
      }

      handle_request(new_fd);
      close(new_fd);
   }
   return 0;

}

/**************************************************
 * Function: child_handler
 *    Handles SIGCHLD by waiting for it.
 *************************************************/

void child_handler(){
   wait(NULL);
}

/**************************************************
 * Function: pipe_handler
 *    Handles SIGPIPE by exiting.
 *************************************************/

void pipe_handler(){
   exit(1);
}

/**************************************************
 * Function: setup_handlers
 *    Sets up signal handlers for SIGCHLD and 
 *    SIGPIPE. 
 *************************************************/

void setup_handlers(int sig){
   struct sigaction act;
   struct sigaction act_two;

   if(sig == CHILD){
      sigemptyset(&act.sa_mask);
      act.sa_handler = child_handler;
      act.sa_flags = 0;
      sigaction(SIGCHLD, &act, NULL); 
   }
   else{
      sigemptyset(&act_two.sa_mask);
      act_two.sa_handler = pipe_handler;
      act_two.sa_flags = 0;
      sigaction(SIGPIPE, &act_two, NULL); 
   }


}

/**************************************************
 * Function: handle_request
 *    Receives file descriptor for request's socket,
 *    forks a new process to handle it, then reads
 *    the request, then sends a reply.  
 *************************************************/
void handle_request(int fd){
   pid_t pid;
   char *req = NULL;
   char *reply = NULL;

   if((pid = fork()) < 0){
      reply = err_response("500");
      write(fd, reply, strlen(reply)+1);
      free(reply);
      exit(1);
   }
   else if(pid == 0){
      setup_handlers(PIPE);
      req = read_request(fd);
      if(req == NULL)
         exit(1);
      reply = proc_req(req);


      write(fd, reply, strlen(reply)+1);
      free(req);
      free(reply);
      exit(1);
   }
}


/**************************************************
 * Function: read_request
 *    Reads the request from the socket into a 
 *    buffer.
 * Returns: 
 *    Character pointer to the request string. 
 *************************************************/
char *read_request(int fd){
   int size = START_SIZE;
   char *buf = (char*)checked_malloc(size);
   char *start = buf;
   int pos = 0;
   start[0] = 0;

   while(read(fd,buf++,1) > 0){
      if(start[pos++] == '\n')
         break;
      if(pos >= size){
         size += 50;
         start = realloc(start, size);
         buf = start + pos;
      }
   }

   if(start[0] == 0){
      free(start);
      return NULL;
   }
   return start;
}

/**************************************************
 * Function: proc_req
 *    Parses the request into the type of request
 *    and the file that proceeds it. If it is a
 *    cgi-like command, it will run a different 
 *    function to handle that. Otherwise, it checks
 *    for .., then checks the type of request and
 *    returns an appropriate response. 
 * 
 * Returns: 
 *    Character pointer to the reply to client. 
 *************************************************/
char *proc_req(char *req){

   char *type = strtok(req," ");
   char *file = strtok(NULL, " ");
   char *file_copy = strdup(file);
   char *cgi_check = strtok(file_copy,"/");
   char *reply = NULL;

   if(strlen(file) <= 1){
      free(file_copy);
      return err_response("400");
   }
   file = add_dot_slash(file);

   if(strcmp(cgi_check, "cgi-like") == 0){
      if(strcmp(type, "GET") == 0)
         reply = cgi(file,GET); 
      else if(strcmp(type,"HEAD")==0){
         reply = cgi(file, HEAD);
      }
      else{
         reply =  err_response("501");
      }
   }
   else if(strstr(file, "/..") != 0){
      free(file);
      reply = err_response("403");
   }
   else if(strcmp(type, "GET")==0){
      reply = process_get(file);
   }
   else if(strcmp(type, "HEAD")==0){
      reply = get_header(file);
   }
   else{
      reply = err_response("501");
   }

   free(file);
   free(file_copy);

   return reply;

}

/**************************************************
 * Function: process_get
 *    Gets the header, checks for errors, then
 *    adds the contents onto the reply string. 
 * Returns: 
 *    Character pointer to the reply. 
 *************************************************/

char *process_get(char *file){
   char *reply;
   char *header = NULL;
   char *perm = NULL;
   char *cont = NULL;

   header = get_header(file);
   /*Return errors if they exist*/
   if(header[9] != '2')
      reply = header;
   else if((perm = check_perm(file)) != NULL){
      free(file);
      reply = perm;
   }

   else{
      cont = get_contents(file);
      reply = checked_malloc(strlen(cont)
            + strlen(header)+1);
      strcpy(reply, header);
      strcat(reply, cont);
      free(header);
      free(cont);
   }

   return reply;
}

/**************************************************
 * Function: get_header
 *    Checks that the file exists, stores the 
 *    length of the file in a string. Builds the
 *    header line by line with concatenation.
 * Returns: 
 *    Character pointer to start of header. 
 *************************************************/

char *get_header(char *file){
   struct stat fs;
   char *header = checked_malloc(250);
   char len[10];

   if(check_perm(file) != NULL){
      free(header);
      return err_response(check_perm(file));
   }

   stat(file, &fs);

   sprintf(len, "%d", (int)fs.st_size);
    
   strcpy(header, "HTTP/1.0 200 OK\r\n");
   strcat(header, "Content-Type: text/html\r\n");
   strcat(header, "Content-Length: ");
   strcat(header, len);
   strcat(header, "\r\n\r\n");


   return header;


}

/**********************************************
 * Function: check_perm
 *    Checks for the existence of the file and 
 *    if it is readable. 
 * Returns: 
 *    String of the error number, or NULL upon
 *    success; 
 **********************************************/

char *check_perm(char *file){
   struct stat fs;

   stat(file, &fs);
   if(!S_ISREG(fs.st_mode))
      return "400";
   if(access(file, F_OK) == -1)
      return "404";
   if(access(file, R_OK) == -1)
      return "403";

   return NULL;
}


/**************************************************
 * Function: get_contents
 *    Opens the file and uses fseek to find the 
 *    size. Uses that size to create a buffer.
 *    Read the contents from the file pointer,
 *    stores it in buffer.
 * Returns:
 *    Buffer containing string of contents.
 *************************************************/

char *get_contents(char *file){
   FILE *fp;
   char *contents;
   long file_size;


   file = add_dot_slash(file);
   
   fp = fopen(file, "r");
   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   fseek(fp,0, SEEK_SET);

   
   contents = checked_malloc(file_size + 1);
   fread(contents, file_size, 1, fp);

   fclose(fp);
   contents[file_size] = 0;

   free(file);
   return contents;

}

/**************************************************
 * Function: cgi
 *    Gets called if cgi-like command is requested.
 *    Eats up the slashes and dots preceding the
 *    file, then uses strtok to build array of 
 *    strings to be used by exec. Passes the 
 *    command to exec_cgi.
 * Returns:
 *    Response from exec_cgi, the complete reply.
 *************************************************/

char *cgi(char *file, int req){
   int i = 0;
   int size = (sizeof(char*) * 25) + 1;
   char **command = checked_malloc(size);
   char *response;
   char *temp;
   
   while((file)[0] != 'c'){
      file++;
   }
   temp = strtok(NULL, "? ");
   command[0] = add_dot_slash(temp);

   while(command[i] != NULL){
      command[++i] =  strtok(NULL, "&");
   }

   response = exec_cgi(command, req);

   free(command[0]);
   free(command);

   return response;
}

/**************************************************
 * Function: exec_cgi
 *    Forks a new process in order to exec the 
 *    desired program. Child redirects stdout and
 *    stderr into a file, which can be read to 
 *    return the result. Parent waits, then checks
 *    for errors, then determines what to return.
 * Returns:
 *    Either header or header with contents. Both
 *    character pointers.
 *************************************************/

char *exec_cgi(char **cmd, int req){
   pid_t pid; 
   int out;
   char pid_int[5];
   char *pid_int2;
   char *header;
   char *cont;
   char *reply;
   int status;

   pid = fork();
   if(pid < 0){
      header = err_response("500");
   }
   else if(pid == 0){
      setup_handlers(PIPE);
      sprintf(pid_int, "%d", getpid());
      out = open(pid_int, O_CREAT | O_RDWR, S_IRUSR 
               | S_IWUSR);
      chdir("cgi-like");
      dup2(out, 1);
      dup2(out, 2);
      execvp(cmd[0], cmd);

      if(strcmp(check_perm(cmd[0]),"404") == 0)
         exit(4);
      if(strcmp(check_perm(cmd[0]),"403") == 0)
         exit(3);
      else
         exit(2);
   }
   else{
      waitpid(pid, &status, 0);
      sprintf(pid_int, "%d", pid);
      pid_int2 = add_dot_slash(pid_int);

      if((header=check_exit(status)) != NULL){}
      else if(req == HEAD){
         header = get_header(pid_int2);
      }
      else{
         header = get_header(pid_int2);
         cont = get_contents(pid_int2);
         reply = checked_malloc(strlen(header)
                        +strlen(cont) + 2);

         strcpy(reply, header);
         strcat(reply, cont);
         free(header);
         header = reply;
         free(cont);
      }
      remove(pid_int2);
      free(pid_int2);

   }

   return header;

}

/**************************************************
 * Function: check_exit
 *    Checks the exit status of the child process,
 *    If the child exits abnormally, the proper 
 *    error response is given.
 * Returns:
 *    String of the error response, or null if
 *    child exited normally. 
 *************************************************/

char *check_exit(int status){
   int exit_status;
   exit_status = WEXITSTATUS(status);
   if(exit_status != 0){
      if(exit_status == 4){
         return err_response("404");
      }
      if(exit_status == 3){
         return err_response("403");
      }
      if(exit_status == 2){
         return err_response("500");
      }

   }
   return NULL;
}

/**************************************************
 * Function: add_dot_slash
 *    Adds a './' to the beginning of the string. 
 * Returns: 
 *    Character pointer to new space with ./file.
 *************************************************/

char *add_dot_slash(char *file){
   char *new = checked_malloc(strlen(file) + 3);
   strcpy(new, "./");
   strcat(new, file);

   return new;
}


/**************************************************
 * Function: err_reponse
 *    Takes a string for the type of error, then
 *    builds proper header. The proper html files
 *    that I created will be opened and read for
 *    the contents of the error reply. 
 * Returns:
 *    Error response to be sent to the client. 
 *************************************************/

char *err_response(char *type){
   char *error_msg;
   char *resp = checked_malloc(550);
   int err = strtol(type, NULL, 10);


   strcpy(resp, "HTTP/1.0 ");

   switch(err){
      case 400: 
         strcat(resp, "400 Bad Request");
         error_msg = get_contents("Err_400.html");
         break;
      case 403:
         strcat(resp, "403 Permission Denied");
         error_msg = get_contents("Err_403.html");
         break;
      case 404: 
         strcat(resp, "404 Not Found");
         error_msg = get_contents("Err_404.html");
         break;
      case 500: 
         strcat(resp, "Internal Error");
         error_msg = get_contents("Err_500.html");
         break;
      case 501:
         strcat(resp, "Not Implemented");
         error_msg = get_contents("Err_501.html");
         break;
   }


   strcat(resp, "\r\nContent-Type: text/html\r\n");
   strcat(resp, "Content-Length: 110\r\n\r\n");
   strcat(resp,error_msg);
   free(error_msg);


   return resp; 


}


/**************************************************
 * Function: checked_malloc
 *    Mallocs some space, checks for error.
 * Returns: 
 *    Void pointer to freshly malloc'd space.
 *************************************************/
void *checked_malloc(size_t size){
   void *spot = malloc(size);
   if(spot == NULL){
     perror("Malloc");
     exit(1);
   }
  return spot; 
}
